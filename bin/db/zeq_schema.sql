
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

-- The material fields look like 'how not to design a database 101',
-- but we really have no reason to split them into their own table... 
-- only used for char select, anyway.
CREATE TABLE character (
    character_id            INTEGER PRIMARY KEY,
    account_id              INT,
    name                    TEXT,
    surname                 TEXT,
    level                   INT     DEFAULT 1,
    class                   INT     DEFAULT 1,
    race                    INT     DEFAULT 1,
    zone_id                 INT     DEFAULT 1,
    instance_id             INT     DEFAULT 0,
    gender                  INT     DEFAULT 0,
    face                    INT     DEFAULT 0,
    deity                   INT     DEFAULT 396,
    x                       REAL    DEFAULT 0,
    y                       REAL    DEFAULT 0,
    z                       REAL    DEFAULT 0,
    heading                 REAL    DEFAULT 0,
    current_hp              INT     DEFAULT 10,
    current_mana            INT     DEFAULT 0,
    current_endurance       INT     DEFAULT 0,
    experience              INT     DEFAULT 0,
    training_points         INT     DEFAULT 5,
    base_str                INT     DEFAULT 0,
    base_sta                INT     DEFAULT 0,
    base_dex                INT     DEFAULT 0,
    base_agi                INT     DEFAULT 0,
    base_int                INT     DEFAULT 0,
    base_wis                INT     DEFAULT 0,
    base_cha                INT     DEFAULT 0,
    guild_id                INT     DEFAULT 0,
    guild_rank              INT     DEFAULT 0,
    harmtouch_timestamp     INT     DEFAULT 0,
    discipline_timestamp    INT     DEFAULT 0,
    pp                      INT     DEFAULT 0,
    gp                      INT     DEFAULT 0,
    sp                      INT     DEFAULT 0,
    cp                      INT     DEFAULT 0,
    pp_cursor               INT     DEFAULT 0,
    gp_cursor               INT     DEFAULT 0,
    sp_cursor               INT     DEFAULT 0,
    cp_cursor               INT     DEFAULT 0,
    pp_bank                 INT     DEFAULT 0,
    gp_bank                 INT     DEFAULT 0,
    sp_bank                 INT     DEFAULT 0,
    cp_bank                 INT     DEFAULT 0,
    hunger                  INT     DEFAULT 4500,
    thirst                  INT     DEFAULT 4500,
    is_gm                   BOOLEAN DEFAULT 0,
    anon                    INT     DEFAULT 0,
    drunkeness              INT     DEFAULT 0,
    creation_time           INT,
    material0               INT     DEFAULT 0,
    material1               INT     DEFAULT 0,
    material2               INT     DEFAULT 0,
    material3               INT     DEFAULT 0,
    material4               INT     DEFAULT 0,
    material5               INT     DEFAULT 0,
    material6               INT     DEFAULT 0,
    material7               INT     DEFAULT 0,
    material8               INT     DEFAULT 0,
    tint0                   INT     DEFAULT 0,
    tint1                   INT     DEFAULT 0,
    tint2                   INT     DEFAULT 0,
    tint3                   INT     DEFAULT 0,
    tint4                   INT     DEFAULT 0,
    tint5                   INT     DEFAULT 0,
    tint6                   INT     DEFAULT 0
);

CREATE INDEX idx_character_account_id ON character (account_id);
CREATE INDEX idx_character_name ON character (name);

CREATE TABLE guild (
    guild_id    INTEGER PRIMARY KEY,
    name        TEXT
);
