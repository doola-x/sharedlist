// Exercises Sharedlist::apply / sync / invert against an in-memory sqlite db.
// build + run (from repo root, same flags as etc/buildcmds.txt, minus the server):
//   g++ -std=c++20 -I src/include -o /tmp/test_actions tests/test_actions.cpp \
//       src/sharedlist.cpp src/actions.cpp src/dal.cpp src/http_client.cpp \
//       src/include/sqlite3.o -lcurl -lcrypto && /tmp/test_actions database/schema.sql
#include <fstream>
#include <sstream>
#include "../src/include/sharedlist.hpp"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { cerr << "FAIL line " << __LINE__ << ": " #cond << endl; failures++; } } while (0)

static string join(const vector<string>& v) {
	string s;
	for (auto& x : v) s += (s.empty() ? "" : ",") + x;
	return s;
}

int main(int argc, char** argv) {
	if (argc < 2) { cerr << "usage: test_actions schema.sql" << endl; return 2; }
	ifstream f(argv[1]);
	stringstream schema;
	schema << f.rdbuf();

	Database db(":memory:");
	db.open();
	db.execute(schema.str());
	db.execute("insert into users (id, username, hashword, salt) values (1,'alice','',''),(2,'bob','',''),(3,'carol','','')");
	db.execute("insert into tracks (origin_id, name) values ('t1','one'),('t2','two'),('t3','three'),('t4','four')");

	HttpClient http;
	Sharedlist sl(&db, &http);
	const int ALICE = 1, BOB = 2, CAROL = 3;

	auto order = [&](int id) {
		vector<string> v;
		for (auto& r : db.query<StringModel>(
				"select origin_id from sharedlist_tracks where sharedlist_id = ? order by position", {id}))
			v.push_back(r.value);
		return join(v);
	};
	auto act = [](ActionKind k, const string& track = "", int pos = -1) {
		Action a; a.kind = k; a.track_id = track; a.position = pos; return a;
	};
	auto member = [](ActionKind k, int user, const string& role = "") {
		Action a; a.kind = k; a.user_id = user; a.new_value = role; return a;
	};
	auto lastStatus = [](const Sharedlist::ApplyResult& r) { return r.results.empty() ? string("-") : r.results.back().status; };

	// create + sync: owner membership, dedupe, origin order
	int id = sl.createSharedlist(ALICE, "spotify", "pl1");
	CHECK(id > 0);
	CHECK(sl.roleOf(id, ALICE) == "owner");
	CHECK(sl.createSharedlist(ALICE, "spotify", "pl1") == id); // idempotent, no second owner row
	CHECK(sl.getMembers(id).size() == 1);

	vector<SharedlistTrackModel> origin(4);
	origin[0].id = "t1"; origin[1].id = "t2"; origin[2].id = "t3"; origin[3].id = "t1"; // dup
	CHECK(sl.syncSharedlistTracks(origin, id, ALICE) == 0);
	CHECK(order(id) == "t1,t2,t3");
	CHECK(sl.syncSharedlistTracks(origin, id, ALICE) == 0); // no diff, nothing logged
	long long since = sl.history(id, 0, 1000).back().id;

	// permissions: non-member and viewer-to-be are refused, whole batch, nothing written
	auto r = sl.apply(id, BOB, {act(ActionKind::AddTrack, "t4")});
	CHECK(!r.ok && r.http == 403);
	r = sl.apply(id, ALICE, {member(ActionKind::AddMember, BOB, "editor")});
	CHECK(r.ok && lastStatus(r) == "applied");
	r = sl.apply(id, ALICE, {member(ActionKind::AddMember, BOB, "editor")});
	CHECK(lastStatus(r) == "rejected"); // already a member
	r = sl.apply(id, ALICE, {member(ActionKind::AddMember, CAROL, "owner")});
	CHECK(lastStatus(r) == "rejected"); // owners can't be minted

	// editor: tracks + meta yes, membership no
	r = sl.apply(id, BOB, {act(ActionKind::AddTrack, "t4", 0), act(ActionKind::RemoveTrack, "t9")});
	CHECK(r.ok && r.results.size() == 2 && r.results[0].status == "applied" && r.results[1].status == "rejected");
	CHECK(r.results[0].id == r.batch_id); // batch named after first action
	CHECK(order(id) == "t4,t1,t2,t3");
	r = sl.apply(id, BOB, {member(ActionKind::AddMember, CAROL, "viewer")});
	CHECK(!r.ok && r.http == 403);
	r = sl.apply(id, BOB, {act(ActionKind::AddTrack, "t4")});
	CHECK(lastStatus(r) == "rejected"); // already in list
	r = sl.apply(id, BOB, {act(ActionKind::AddTrack, "nope")});
	CHECK(lastStatus(r) == "rejected"); // unknown track

	// move + meta
	r = sl.apply(id, ALICE, {act(ActionKind::MoveTrack, "t3", 0)});
	CHECK(lastStatus(r) == "applied" && order(id) == "t3,t4,t1,t2");
	Action meta; meta.kind = ActionKind::ChangeMeta; meta.field = "name"; meta.new_value = "mix";
	CHECK(lastStatus(sl.apply(id, BOB, {meta})) == "applied");
	meta.field = "owner_id";
	CHECK(lastStatus(sl.apply(id, BOB, {meta})) == "rejected"); // whitelist only

	// role changes + owner is untouchable; viewer is read-only; anyone can leave
	CHECK(lastStatus(sl.apply(id, ALICE, {member(ActionKind::ChangeRole, BOB, "viewer")})) == "applied");
	CHECK(sl.apply(id, BOB, {act(ActionKind::AddTrack, "t4")}).http == 403);
	CHECK(lastStatus(sl.apply(id, ALICE, {member(ActionKind::RemoveMember, ALICE)})) == "rejected");
	CHECK(lastStatus(sl.apply(id, ALICE, {member(ActionKind::ChangeRole, ALICE, "viewer")})) == "rejected");
	CHECK(lastStatus(sl.apply(id, ALICE, {member(ActionKind::AddMember, CAROL, "viewer")})) == "applied");
	CHECK(lastStatus(sl.apply(id, CAROL, {member(ActionKind::RemoveMember, CAROL)})) == "applied");
	CHECK(sl.roleOf(id, CAROL).empty());
	CHECK(sl.apply(id, CAROL, {act(ActionKind::AddTrack, "t4")}).http == 403);

	// undo everything since the sync by inverting the log in reverse
	auto log = sl.history(id, since, 1000);
	int undone = 0;
	for (auto it = log.rbegin(); it != log.rend(); ++it) {
		if (it->status != "applied") continue;
		auto a = Action::fromRecord(it->kind, it->payload);
		CHECK(a.has_value());
		auto res = sl.apply(id, ALICE, {invert(*a)});
		CHECK(res.ok && lastStatus(res) == "applied");
		undone++;
	}
	CHECK(undone > 0);
	CHECK(order(id) == "t1,t2,t3");
	CHECK(sl.roleOf(id, BOB).empty() && sl.getMembers(id).size() == 1);
	SharedlistInfoModel info;
	CHECK(sl.getSharedlistInfo(id, info) && info.name == "");

	// sync diff: origin drops t2, adds t4
	origin.assign(3, SharedlistTrackModel());
	origin[0].id = "t1"; origin[1].id = "t3"; origin[2].id = "t4";
	CHECK(sl.syncSharedlistTracks(origin, id, ALICE) == 0);
	CHECK(order(id) == "t1,t3,t4");

	cout << (failures ? "FAILED" : "all passed") << endl;
	return failures ? 1 : 0;
}
