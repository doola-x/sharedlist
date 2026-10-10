#pragma once
#include "dal.hpp"
#include "http_client.hpp"
#include "models.hpp"

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
	int syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks, int sharedlist_id) const;
	int createSharedlist(int user_id, const string& origin_type, const string& origin_id) const;
	bool isOwner(int sharedlist_id, int user_id) const;
	vector<TrackModel> getSharedlistTracks(int sharedlist_id, int offset, int limit) const;
};
