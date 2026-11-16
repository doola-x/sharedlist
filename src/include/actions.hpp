#pragma once
#include "crow.h"
#include <optional>
#include <string>
using namespace std;

// Everything that can happen to a sharedlist. A "diff" is just a vector<Action>
// applied as one batch. Fields marked (filled) are recorded by apply() at the
// moment the action lands, so the log entry can be inverted later.
enum class ActionKind { AddTrack, RemoveTrack, MoveTrack, ChangeMeta, AddMember, RemoveMember, ChangeRole };

struct Action {
	ActionKind kind = ActionKind::AddTrack;
	string track_id;       // add/remove/move_track
	int position = -1;     // add: insert at (-1 = end) | remove: old position (filled) | move: destination
	int from = -1;         // move: old position (filled)
	int user_id = 0;       // add/remove_member, change_role: the target user
	string field;          // change_meta: name | description | image_url
	string old_value;      // change_meta / change_role / remove_member(role) (filled)
	string new_value;      // change_meta / change_role / add_member(role)

	string toJson() const;
	// client submission: only the input fields are read, (filled) ones are ignored
	static optional<Action> fromKindAndJson(const string& kind, const crow::json::rvalue& body);
	// stored log entry: keeps the (filled) fields
	static optional<Action> fromRecord(const string& kind, const string& payload);
};

const char* kindName(ActionKind k);
optional<ActionKind> kindFromName(const string& name);

// undo of an already-applied action (uses the (filled) fields)
Action invert(const Action& a);

// role: "owner" | "editor" | "viewer" | "" (not a member)
bool canPerform(const string& role, const Action& a, int actor_id);
bool isAssignableRole(const string& role); // editor | viewer; owner is never assignable
