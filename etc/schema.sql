CREATE TABLE IF NOT EXISTS "users" (
	id integer primary key autoincrement,
	username text,
	email text null,
	hashword text, 
	salt text
);
CREATE TABLE IF NOT EXISTS "sessions" (
	id integer primary key autoincrement,
	session_id text not null,
	user_id integer,
	foreign key (user_id) references users(id)
);
CREATE TABLE IF NOT EXISTS "tokens" (
	id integer primary key autoincrement,
	user_id integer not null,
	access_token text not null,
	refresh_token text not null,
	created_at datetime current_timestamp,
	foreign key (user_id) references users(id) on delete cascade
);
CREATE TABLE IF NOT EXISTS "spotify_state" (
	id integer primary key autoincrement,
	user_id integer not null,
	state text not null,
	created_at datetime default current_timestamp,
	valid int default 0
);
CREATE TABLE IF NOT EXISTS "sharedlists" (
	id integer primary key autoincrement,
	owner_id integer not null,
	origin_type text not null,
	origin_id text not null,
	spotify_id text,
	apple_id text,
	created_at datetime default current_timestamp,
	unique(owner_id, origin_id)
);
CREATE TABLE IF NOT EXISTS "tracks" (
	id integer primary key,
	origin_id text unique,
	spotify_id text,
	apple_id text,
	name text,
	artists text,
	album text
);
CREATE TABLE IF NOT EXISTS "sharedlist_tracks" (
	id integer primary key autoincrement,
	sharedlist_id integer not null,
	origin_id text not null,
	unique(sharedlist_id, origin_id)
);
