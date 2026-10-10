-- usernames must be unique; drop old sessions (they were never enforced anyway)
DELETE FROM sessions;
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE UNIQUE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);
