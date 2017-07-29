
CREATE TABLE login (
    username    TEXT PRIMARY KEY,
    password    BLOB,
    salt        BLOB
);

-- If we are connected to two separate login servers, we need to make sure that
-- an account from login server A with an id of 1 and name "abc" is treated as
-- separate from an account from login server B with an id of 1 and name "xyz".
-- To do this, we make sure to use both the id number AND the account name,
-- rather than just the id number.
CREATE TABLE account_id (
    id      INT,
    name    TEXT,
    PRIMARY KEY (id, name)
);

CREATE TABLE account (
    id                  INT     PRIMARY KEY,
    recent_char_name    TEXT,
    recent_ip_address   TEXT,
    status              INT     DEFAULT 0,
    gm_speed            BOOLEAN DEFAULT 0,
    gm_hide             BOOLEAN DEFAULT 0,
    suspended_until     INT     DEFAULT 0,
    creation_time       DATE    DEFAULT 0
);

CREATE TRIGGER trigger_account_creation_time AFTER INSERT ON account
BEGIN
    UPDATE account SET creation_time = datetime('now') WHERE id = new.id;
END;
