#pragma once
#include "dal.hpp"
#include "http_client.hpp"
#include "models.hpp"
#include "actions.hpp"

using namespace std;

class Sharedlist {
public:
	const Database* db;
	const HttpClient* http;
    
    Sharedlist();
	Sharedlist(const Database* _db, const HttpClient* _http);
	~Sharedlist();

	vector<SharedlistTrackModel> fetchSpotifyTracks(const string& user_token, const string& origin_id) const;
	int addSharedlistTracks(const vector<SharedlistTrackModel>& tracks) const;
	// diffs the origin's tracks against the list and applies the result as one batch
	int syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks, int sharedlist_id, int actor_id) const;
	int createSharedlist(int user_id, const string& origin_type, const string& origin_id) const;

	// membership: "owner" | "editor" | "viewer", or "" if user isn't a member
	string roleOf(int sharedlist_id, int user_id) const;
	vector<MemberModel> getMembers(int sharedlist_id) const;
	int findUserId(const string& username) const; // -1 if no such user

	// the only write path for list contents + membership. one transaction per batch.
	// permission failure rejects the whole batch (nothing written); per-action state
	// conflicts (already in list, not a member, ...) are logged as 'rejected' and skipped.
	struct ActionResult { long long id = 0; ActionKind kind; string status; string reason; };
	struct ApplyResult {
		bool ok = false;
		int http = 200;      // suggested status when !ok
		string error;
		long long batch_id = 0;
		vector<ActionResult> results;
	};
	ApplyResult apply(int sharedlist_id, int actor_id, vector<Action> actions) const;
	vector<ActionRecordModel> history(int sharedlist_id, long long since_id, int limit) const;
	bool fetchSpotifyPlaylistMeta(const string& user_token, const string& origin_id, SharedlistMetaModel& meta) const;
	int saveSharedlistMeta(int sharedlist_id, const SharedlistMetaModel& meta) const;
	bool getSharedlistInfo(int sharedlist_id, SharedlistInfoModel& info) const;
	vector<TrackModel> getSharedlistTracks(int sharedlist_id, int offset, int limit) const;
};
