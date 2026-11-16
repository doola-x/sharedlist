CREATE TABLE IF NOT EXISTS "users" (
	id integer primary key autoincrement,
	username text,
	email text null,
	hashword text, 
	salt text
);
CREATE TABLE IF NOT EXISTS "sessions" (
	id integer primary key autoincrement,
	session_token text not null,
	user_id integer not null,
	expires date not null,
	foreign key (user_id) references users(id)
);
CREATE TABLE IF NOT EXISTS "tokens" (
	id integer primary key autoincrement,
	user_id integer not null unique,
	access_token text not null,
	refresh_token text not null,
	expires_at integer not null,
	created_at datetime default current_timestamp,
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
	name text,
	description text,
	image_url text,
	origin_owner text,
	snapshot_id text,
	unique(owner_id, origin_id)
);
CREATE TABLE IF NOT EXISTS "tracks" (
	id integer primary key,
	origin_id text unique,
	spotify_id text,
	apple_id text,
	name text,
	artists text,
	album text,
	album_id text,
	duration_ms integer,
	image_url text,
	explicit integer,
	release_date text
);
CREATE TABLE IF NOT EXISTS "spotify_profiles" (
	user_id integer primary key,
	spotify_id text not null,
	display_name text,
	image_url text,
	product text,
	country text,
	updated_at datetime default current_timestamp,
	foreign key (user_id) references users(id) on delete cascade
);
CREATE TABLE IF NOT EXISTS "sharedlist_tracks" (
	id integer primary key autoincrement,
	sharedlist_id integer not null,
	origin_id text not null,
	position integer not null default 0,
	unique(sharedlist_id, origin_id)
);
-- who can see/edit a list. exactly one 'owner' row per list (owner_id on sharedlists mirrors it)
CREATE TABLE IF NOT EXISTS "sharedlist_members" (
	sharedlist_id integer not null,
	user_id integer not null,
	role text not null check (role in ('owner', 'editor', 'viewer')),
	added_at datetime default current_timestamp,
	primary key (sharedlist_id, user_id),
	foreign key (sharedlist_id) references sharedlists(id) on delete cascade,
	foreign key (user_id) references users(id) on delete cascade
);
-- append-only log. id order == apply order. sharedlist_tracks / sharedlist_members are
-- projections of this log. payload carries old values so every action can be inverted.
CREATE TABLE IF NOT EXISTS "sharedlist_actions" (
	id integer primary key autoincrement,
	sharedlist_id integer not null,
	actor_id integer not null,
	kind text not null,
	payload text not null,
	batch_id integer,
	status text not null default 'applied' check (status in ('applied', 'pending', 'rejected')),
	reason text,
	created_at datetime default current_timestamp,
	foreign key (sharedlist_id) references sharedlists(id) on delete cascade
);
CREATE INDEX IF NOT EXISTS idx_actions_list ON sharedlist_actions(sharedlist_id, id);
CREATE INDEX IF NOT EXISTS idx_sharedlist_tracks_pos ON sharedlist_tracks(sharedlist_id, position);
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE UNIQUE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);
