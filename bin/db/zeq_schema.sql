
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
    inst_id                 INT     DEFAULT 0,
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
    autosplit               BOOLEAN DEFAULT 0,
    is_pvp                  BOOLEAN DEFAULT 0,
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

CREATE TABLE skill (
    character_id    INT,
    skill_id        INT,
    value           INT,
    
    PRIMARY KEY (character_id, skill_id)
);

CREATE TABLE spellbook (
    character_id    INT,
    slot_id         INT,
    spell_id        INT,
    
    PRIMARY KEY (character_id, slot_id)
);

CREATE TABLE memmed_spells (
    character_id        INT,
    slot_id             INT,
    spell_id            INT,
    recast_timestamp_ms INT,
    
    PRIMARY KEY (character_id, slot_id)
);

CREATE TABLE bind_point (
    character_id    INT,
    bind_id         INT,
    zone_id         INT,
    inst_id         INT DEFAULT 0,
    x               REAL,
    y               REAL,
    z               REAL,
    heading         REAL,
    
    PRIMARY KEY (character_id, bind_id)
);

CREATE TABLE guild (
    guild_id    INTEGER PRIMARY KEY,
    name        TEXT
);

CREATE TABLE item_proto (
    item_id     INTEGER PRIMARY KEY,
    path        TEXT    UNIQUE,
    mod_time    INT,
    name        TEXT,
    lore_text   TEXT,
    data        BLOB
);

CREATE TABLE inventory (
    character_id    INT,
    slot_id         INT,
    item_id         INT,
    stack_amount    INT DEFAULT 0,
    charges         INT DEFAULT 0,
    
    PRIMARY KEY (character_id, slot_id)
);

CREATE TABLE cursor_queue (
    character_id    INT,
    item_id         INT,
    stack_amount    INT DEFAULT 0,
    charges         INT DEFAULT 0
);

CREATE INDEX idx_cursor_queue ON cursor_queue (character_id);
