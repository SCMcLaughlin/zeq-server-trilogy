
CREATE TABLE account (
    id                  INTEGER PRIMARY KEY,
    name                TEXT,
    password_hash       BLOB,
    salt                BLOB,
    recent_char_name    TEXT,
    recent_ip_address   TEXT,
    status              INT     DEFAULT 0,
    gm_speed            BOOLEAN DEFAULT 0,
    gm_hide             BOOLEAN DEFAULT 0,
    suspended_until     INT     DEFAULT 0,
    creation_time       DATE
);

CREATE INDEX idx_account_name ON account (name);
