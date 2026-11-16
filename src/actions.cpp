#include "include/actions.hpp"

static const char* KIND_NAMES[] = {"add_track", "remove_track", "move_track", "change_meta",
	"add_member", "remove_member", "change_role"};

const char* kindName(ActionKind k) { return KIND_NAMES[static_cast<int>(k)]; }

optional<ActionKind> kindFromName(const string& name) {
	for (int i = 0; i < 7; i++) {
		if (name == KIND_NAMES[i]) return static_cast<ActionKind>(i);
	}
	return nullopt;
}

bool isAssignableRole(const string& role) { return role == "editor" || role == "viewer"; }

static string str(const crow::json::rvalue& b, const char* k) {
	return (b.has(k) && b[k].t() == crow::json::type::String) ? string(b[k].s()) : "";
}
static int num(const crow::json::rvalue& b, const char* k, int dflt) {
	return (b.has(k) && b[k].t() == crow::json::type::Number) ? static_cast<int>(b[k].i()) : dflt;
}

string Action::toJson() const {
	crow::json::wvalue j;
	switch (kind) {
	case ActionKind::AddTrack:
	case ActionKind::RemoveTrack:
		j["track_id"] = track_id; j["position"] = position; break;
	case ActionKind::MoveTrack:
		j["track_id"] = track_id; j["from"] = from; j["to"] = position; break;
	case ActionKind::ChangeMeta:
		j["field"] = field; j["old"] = old_value; j["new"] = new_value; break;
	case ActionKind::AddMember:
		j["user_id"] = user_id; j["role"] = new_value; break;
	case ActionKind::RemoveMember:
		j["user_id"] = user_id; j["role"] = old_value; break;
	case ActionKind::ChangeRole:
		j["user_id"] = user_id; j["old"] = old_value; j["new"] = new_value; break;
	}
	return j.dump();
}

optional<Action> Action::fromKindAndJson(const string& kind_name, const crow::json::rvalue& b) {
	auto k = kindFromName(kind_name);
	if (!k) return nullopt;
	Action a;
	a.kind = *k;
	switch (*k) {
	case ActionKind::AddTrack:
		a.track_id = str(b, "track_id"); a.position = num(b, "position", -1);
		if (a.track_id.empty()) return nullopt;
		break;
	case ActionKind::RemoveTrack:
		a.track_id = str(b, "track_id");
		if (a.track_id.empty()) return nullopt;
		break;
	case ActionKind::MoveTrack:
		a.track_id = str(b, "track_id"); a.position = num(b, "to", -1);
		if (a.track_id.empty() || a.position < 0) return nullopt;
		break;
	case ActionKind::ChangeMeta:
		a.field = str(b, "field"); a.new_value = str(b, "new");
		if (a.field.empty()) return nullopt;
		break;
	case ActionKind::AddMember:
		a.user_id = num(b, "user_id", 0); a.new_value = str(b, "role");
		if (a.user_id <= 0) return nullopt;
		break;
	case ActionKind::RemoveMember:
		a.user_id = num(b, "user_id", 0);
		if (a.user_id <= 0) return nullopt;
		break;
	case ActionKind::ChangeRole:
		a.user_id = num(b, "user_id", 0); a.new_value = str(b, "new");
		if (a.user_id <= 0) return nullopt;
		break;
	}
	return a;
}

optional<Action> Action::fromRecord(const string& kind_name, const string& payload) {
	auto b = crow::json::load(payload);
	auto k = kindFromName(kind_name);
	if (!b || !k) return nullopt;
	Action a;
	a.kind = *k;
	a.track_id = str(b, "track_id");
	a.user_id = num(b, "user_id", 0);
	a.field = str(b, "field");
	switch (*k) {
	case ActionKind::AddTrack:
	case ActionKind::RemoveTrack: a.position = num(b, "position", -1); break;
	case ActionKind::MoveTrack: a.from = num(b, "from", -1); a.position = num(b, "to", -1); break;
	case ActionKind::ChangeMeta:
	case ActionKind::ChangeRole: a.old_value = str(b, "old"); a.new_value = str(b, "new"); break;
	case ActionKind::AddMember: a.new_value = str(b, "role"); break;
	case ActionKind::RemoveMember: a.old_value = str(b, "role"); break;
	}
	return a;
}

Action invert(const Action& a) {
	Action r = a;
	switch (a.kind) {
	case ActionKind::AddTrack: r.kind = ActionKind::RemoveTrack; break;
	case ActionKind::RemoveTrack: r.kind = ActionKind::AddTrack; break;
	case ActionKind::MoveTrack: r.from = a.position; r.position = a.from; break;
	case ActionKind::ChangeMeta:
	case ActionKind::ChangeRole: r.old_value = a.new_value; r.new_value = a.old_value; break;
	case ActionKind::AddMember: r.kind = ActionKind::RemoveMember; r.old_value = a.new_value; r.new_value = ""; break;
	case ActionKind::RemoveMember: r.kind = ActionKind::AddMember; r.new_value = a.old_value; r.old_value = ""; break;
	}
	return r;
}

bool canPerform(const string& role, const Action& a, int actor_id) {
	if (role == "owner") return true;
	switch (a.kind) {
	case ActionKind::AddTrack:
	case ActionKind::RemoveTrack:
	case ActionKind::MoveTrack:
	case ActionKind::ChangeMeta:
		return role == "editor";
	case ActionKind::RemoveMember:
		return !role.empty() && a.user_id == actor_id; // anyone can leave
	default:
		return false; // add_member / change_role: owner only
	}
}
