#pragma once
#include "sqlite3.h"
#include "crow.h"
#include <string>
#include <vector>
using namespace std;

// string field of a json object, "" when absent or not a string (e.g. null)
inline string jsonString(const crow::json::rvalue& obj, const char* key) {
	if (!obj.has(key) || obj[key].t() != crow::json::type::String) return "";
	return obj[key].s();
}

// first image url of an `images` array, "" when there is none
inline string firstImageUrl(const crow::json::rvalue& obj) {
	if (!obj.has("images") || obj["images"].t() != crow::json::type::List || obj["images"].size() == 0) return "";
	return jsonString(obj["images"][0], "url");
}

struct UserModel {
	int id;
	string username;
	string salt;
	string hashword;
	
	static UserModel fromRow(sqlite3_stmt* stmt) {
		UserModel user;
		user.id = sqlite3_column_int(stmt, 0);
		user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		user.hashword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		return user;
	}
};

struct SessionModel {
	int id;
	int user_id;
	string session_token;

	static SessionModel fromRow(sqlite3_stmt* stmt) {
		SessionModel session;
		session.id = sqlite3_column_int(stmt, 0);
		session.session_token
		       	= reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		session.user_id = sqlite3_column_int(stmt, 2);
		return session;
	}

   bool operator==(const SessionModel& other) const {
       if (id != other.id) {
           return false;
       }
       if (user_id != other.user_id) {
           return false;
       }
       if (session_token != other.session_token) {
           return false;
       }
       return true;
   } 
};

struct SpotifyStateModel {
	int id;
	int user_id;
	string state;
	string created_at;
	int valid;

	static SpotifyStateModel fromRow(sqlite3_stmt* stmt){
		SpotifyStateModel state;
		state.id = sqlite3_column_int(stmt, 0);
		state.user_id = sqlite3_column_int(stmt, 1);
		state.state = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		state.valid = sqlite3_column_int(stmt, 3);
		return state;
	}
};

struct TokenModel {
	int id;
	int user_id;
	string access_token;
	string refresh_token;
	long expires_at = 0;

	static TokenModel fromRow(sqlite3_stmt* stmt) {
		TokenModel token;
		token.id = sqlite3_column_int(stmt, 0);
		token.user_id = sqlite3_column_int(stmt, 1);
		token.access_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		token.refresh_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		token.expires_at = sqlite3_column_int64(stmt, 4);
		return token;
	}
};

struct SharedlistModel {
	int id;
	int owner_id;
	string origin_type;
	string origin_id;
	string spotify_id;
	string apple_id;
	string created_at;

	static SharedlistModel fromRow(sqlite3_stmt* stmt) {
		SharedlistModel sharedlist;
		sharedlist.id = sqlite3_column_int(stmt, 0);
		sharedlist.owner_id = sqlite3_column_int(stmt, 1);
		sharedlist.origin_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		sharedlist.origin_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		sharedlist.spotify_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
		sharedlist.apple_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
		return sharedlist;
	}
};

struct SharedlistInfoModel {
	string owner;
	string created_at;
	string origin_type;
	string origin_id;
	int track_count = 0;
	long long total_ms = 0;
	string name;
	string description;
	string image_url;
	string origin_owner;

	static SharedlistInfoModel fromRow(sqlite3_stmt* stmt) {
		SharedlistInfoModel info;
		info.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		info.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		info.origin_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		info.origin_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		info.track_count = sqlite3_column_int(stmt, 4);
		info.total_ms = sqlite3_column_int64(stmt, 5);
		info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
		info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
		info.image_url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
		info.origin_owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
		return info;
	}
};

// what the origin service knows about a playlist
struct SharedlistMetaModel {
	string name;
	string description;
	string image_url;
	string owner;
	string snapshot_id;

	static SharedlistMetaModel fromJson(const crow::json::rvalue& body) {
		SharedlistMetaModel meta;
		meta.name = jsonString(body, "name");
		meta.description = jsonString(body, "description");
		meta.image_url = firstImageUrl(body);
		meta.snapshot_id = jsonString(body, "snapshot_id");
		if (body.has("owner")) meta.owner = jsonString(body["owner"], "display_name");
		return meta;
	}
};

struct TrackModel {
	int id;
	string spotify_id;
	string apple_id;
	string name;
	string artists;
	string album;
	long long duration_ms = 0;
	string image_url;
	bool explicit_ = false;

	static TrackModel fromRow(sqlite3_stmt* stmt) {
		TrackModel track;
		track.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		track.artists = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		track.album = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		track.spotify_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		track.duration_ms = sqlite3_column_int64(stmt, 4);
		track.image_url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
		track.explicit_ = sqlite3_column_int(stmt, 6) != 0;
		return track;
	}
};

struct SharedlistTrackModel {
    string id;
    string name;
    vector<string> artists;
    string album;
    string albumId;
    long long duration_ms = 0;
    string image_url;
    bool explicit_ = false;
    string release_date;

    static SharedlistTrackModel fromJson(const crow::json::rvalue& item) {
            SharedlistTrackModel track;
            track.id = item["id"].s();
            track.name = item["name"].s();
            track.album = item["album"]["name"].s();
            track.albumId = item["album"]["id"].s();
            track.image_url = firstImageUrl(item["album"]);
            track.release_date = jsonString(item["album"], "release_date");
            if (item.has("duration_ms")) track.duration_ms = item["duration_ms"].i();
            if (item.has("explicit")) track.explicit_ = item["explicit"].b();

            for (auto& artist : item["artists"]) {
                    track.artists.push_back(artist["name"].s());
            }

            return track;
    }
};


