-- one token row per user, with expiry. Keeps the newest row per user.
-- Existing rows get expires_at = 0 so they refresh on first use.
BEGIN;
CREATE TABLE tokens_new (
	id integer primary key autoincrement,
	user_id integer not null unique,
	access_token text not null,
	refresh_token text not null,
	expires_at integer not null,
	created_at datetime default current_timestamp,
	foreign key (user_id) references users(id) on delete cascade
);
INSERT INTO tokens_new (user_id, access_token, refresh_token, expires_at)
	SELECT user_id, access_token, refresh_token, 0 FROM tokens
	WHERE id IN (SELECT max(id) FROM tokens GROUP BY user_id);
DROP TABLE tokens;
ALTER TABLE tokens_new RENAME TO tokens;
COMMIT;
