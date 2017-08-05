
#include "char_select_thread.h"
#include "aligned.h"
#include "bit.h"
#include "class_id.h"
#include "char_select_client.h"
#include "db_thread.h"
#include "deity_id.h"
#include "gender_id.h"
#include "race_id.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_str.h"
#include "util_thread.h"
#include "zone_id.h"
#include "zpacket.h"
#include "char_select_packet_struct.h"
#include "player_profile_packet_struct.h"
#include "enum_char_select_opcode.h"
#include "enum_zop.h"
#include "enum2str.h"

#define CHAR_SELECT_THREAD_UDP_PORT 9000
#define CHAR_SELECT_THREAD_LOG_PATH "log/char_select_thread.txt"
#define CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS 10000
#define CHAR_SELECT_THREAD_WEARCHANGE_OP_SWITCH_CHAR 0x7fff

typedef struct {
    int64_t     accountId;
    char        sessionKey[16];
} CharSelectAuth;

struct CharSelectThread {
    uint32_t            clientCount;
    uint32_t            authsAwaitingClientCount;
    uint32_t            clientsAwaitingAuthCount;
    uint32_t            guildCount;
    int                 nextQueryId;
    int                 dbId;
    int                 logId;
    CharSelectClient**  clients;
    CharSelectAuth*     authsAwaitingClient;
    CharSelectClient**  clientsAwaitingAuth;
    uint64_t*           authAwaitingClientTimeouts;
    uint64_t*           clientAwaitingAuthTimeouts;
    Guild*              guildList;
    RingBuf*            csQueue;
    RingBuf*            mainQueue;
    RingBuf*            udpQueue;
    RingBuf*            dbQueue;
    RingBuf*            logQueue;
    /* Cached packets that are more or less static for all clients */
    TlgPacket*          packetLoginApproved;
    TlgPacket*          packetEnter;
    TlgPacket*          packetExpansionInfo;
    TlgPacket*          packetGuildList;
    TlgPacket*          packetEcho1;
    TlgPacket*          packetEcho2;
    TlgPacket*          packetEcho3;
    TlgPacket*          packetEcho4;
    TlgPacket*          packetEcho5;
};

static bool cs_thread_drop_client(CharSelectThread* cs, CharSelectClient* client)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = csc_get_ip_addr(client);
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(cs->udpQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_drop_client: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    
        return false;
    }
    
    return true;
}

static bool cs_thread_is_client_valid(CharSelectThread* cs, CharSelectClient* client)
{
    if (client)
    {
        CharSelectClient** clients = cs->clients;
        uint32_t n = cs->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == client)
                return true;
        }
    }
    
    return false;
}

static void cs_thread_handle_add_guild(CharSelectThread* cs, ZPacket* zpacket)
{
    Guild* guild = &zpacket->cs.zAddGuild.guild;

    if (guild->guildName)
    {
        uint32_t index = cs->guildCount;

        if (bit_is_pow2_or_zero(index))
        {
            uint32_t cap = (index == 0) ? 1 : index * 2;
            Guild* array = realloc_array_type(cs->guildList, cap, Guild);

            if (!array)
            {
                /* We can't take this guild, make sure to drop the name refcount */
                sbuf_drop(guild->guildName);
                goto skip;
            }

            cs->guildList = array;
        }

        cs->guildList[index] = *guild;
        cs->guildCount = index + 1;
    }

skip:
    /* Should we update our cached guild list packet? */
    if (zpacket->cs.zAddGuild.isLast)
    {
        TlgPacket* packet = packet_create_type(OP_CS_GuildList, PSCS_GuildList);
        Guild* guildList;
        uint32_t n, i;
        Aligned a;

        if (!packet) return;

        aligned_init(&a, packet_data(packet), packet_length(packet));

        /* PSCS_GuildList */
        /* count */
        aligned_write_uint32(&a, 0);

        n = cs->guildCount;
        guildList = cs->guildList;
        
        /* PSCS_Guild */
        for (i = 0; i < n; i++)
        {
            guild = &guildList[i];

            /* guildId */
            aligned_write_uint32(&a, guild->guildId);
            /* name */
            aligned_write_snprintf_and_advance_by(&a, CHAR_SELECT_MAX_GUILD_NAME_LENGTH, "%s", sbuf_str(guild->guildName));
            /* unknownA */
            aligned_write_uint32(&a, 0xffffffff);
            /* exists */
            aligned_write_uint32(&a, 1);
            /* unknownB */
            aligned_write_uint32(&a, 0);
            /* unknownC */
            aligned_write_uint32(&a, 0xffffffff);
            /* unknownD */
            aligned_write_uint32(&a, 0);
        }

        for (; i < CHAR_SELECT_MAX_GUILD_COUNT; i++)
        {
            /* guildId */
            aligned_write_uint32(&a, 0xffffffff);
            /* name */
            aligned_write_zeroes(&a, CHAR_SELECT_MAX_GUILD_NAME_LENGTH);
            /* unknownA */
            aligned_write_uint32(&a, 0xffffffff);
            /* exists */
            aligned_write_uint32(&a, 0);
            /* unknownB */
            aligned_write_uint32(&a, 0);
            /* unknownC */
            aligned_write_uint32(&a, 0xffffffff);
            /* unknownD */
            aligned_write_uint32(&a, 0);
        }

        cs->packetGuildList = packet_drop(cs->packetGuildList);
        cs->packetGuildList = packet;
        packet_grab(packet);
    }
}

static int cs_thread_check_db_error(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket, const char* funcName)
{
    if (zpacket->db.zResult.hadErrorUnprocessed)
    {
        log_writef(cs->logQueue, cs->logId, "WARNING: %s: database reported error, query unprocessed", funcName);
        return ERR_Invalid;
    }
    
    if (!cs_thread_is_client_valid(cs, client))
    {
        log_writef(cs->logQueue, cs->logId, "WARNING: %s: received database result for CharSelectClient that no longer exists", funcName);
        return ERR_Invalid;
    }
    
    if (zpacket->db.zResult.hadError)
    {
        log_writef(cs->logQueue, cs->logId, "WARNING: %s: database reported error", funcName);
        return ERR_Generic;
    }
    
    return ERR_None;
}

static int cs_thread_trigger_send_char_info(CharSelectThread* cs, CharSelectClient* client, int64_t accountId)
{
    ZPacket zpacket;

    zpacket.db.zQuery.qCSCharacterInfo.client = client;
    zpacket.db.zQuery.qCSCharacterInfo.accountId = accountId;

    return db_queue_query(cs->dbQueue, cs->csQueue, cs->dbId, cs->nextQueryId++, ZOP_DB_QueryCSCharacterInfo, &zpacket);
}

static int cs_thread_add_authed_client(CharSelectThread* cs, CharSelectClient* client, int64_t accountId, const char* sessionKey)
{
    RingBuf* udpQueue = cs->udpQueue;
    uint32_t index = cs->clientCount;
    int rc = ERR_OutOfMemory;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        CharSelectClient** clients = realloc_array_type(cs->clients, cap, CharSelectClient*);
        
        if (!clients) goto fail;
        
        cs->clients = clients;
    }
    
    cs->clients[index] = client;
    cs->clientCount = index + 1;
    
    /* Info the client that we have accepted them */
    rc = csc_schedule_packet(client, udpQueue, cs->packetLoginApproved);
    if (rc) goto fail;
    
    rc = csc_schedule_packet(client, udpQueue, cs->packetEnter);
    if (rc) goto fail;
    
    rc = csc_schedule_packet(client, udpQueue, cs->packetExpansionInfo);
    if (rc) goto fail;
    
    /* Query the DB for char select data */
    rc = cs_thread_trigger_send_char_info(cs, client, accountId);
    if (rc) goto fail;
    
    csc_set_auth_data(client, accountId, sessionKey);
    return ERR_None;
    
fail:
    cs_thread_drop_client(cs, client);
    return rc;
}

static bool cs_thread_is_auth_awaiting_client(CharSelectThread* cs, int64_t accountId, const char* sessionKey)
{
    CharSelectAuth* auths = cs->authsAwaitingClient;
    uint32_t n = cs->authsAwaitingClientCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        CharSelectAuth* auth = &auths[i];
        
        /*fixme: accountId may be limited to 7 digits, should match the truncating behavior for this check*/
        if (auth->accountId == accountId && memcmp(auth->sessionKey, sessionKey, sizeof(auth->sessionKey)) == 0)
        {
            /* Pop and swap */
            n--;
            auths[i] = auths[n];
            cs->authAwaitingClientTimeouts[i] = cs->authAwaitingClientTimeouts[n];
            cs->authsAwaitingClientCount = n;
            return true;
        }
    }
    
    return false;
}

static int cs_thread_add_client_awaiting_auth(CharSelectThread* cs, CharSelectClient* client)
{
    uint32_t index = cs->clientsAwaitingAuthCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        CharSelectClient** clients;
        uint64_t* timeouts;
        
        clients = realloc_array_type(cs->clientsAwaitingAuth, cap, CharSelectClient*);
        if (!clients) goto fail;
        cs->clientsAwaitingAuth = clients;
        
        timeouts = realloc_array_type(cs->clientAwaitingAuthTimeouts, cap, uint64_t);
        if (!timeouts) goto fail;
        cs->clientAwaitingAuthTimeouts = timeouts;
    }
    
    cs->clientsAwaitingAuth[index] = client;
    cs->clientAwaitingAuthTimeouts[index] = clock_milliseconds() + CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS;
    cs->clientsAwaitingAuthCount = index + 1;
    return ERR_None;
    
fail:
    return ERR_OutOfMemory;
}

static int cs_thread_handle_op_login_info(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    Aligned a;
    const char* acct;
    int64_t accountId;
    const char* sessionKey;
    
    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);
    
    if (aligned_remaining_data(&a) < 2)
        return ERR_Invalid;
    
    acct = aligned_current_type(&a, const char);
    
    if (aligned_advance_past_null_terminator(&a) < 4 || aligned_remaining_data(&a) < 16)
        return ERR_Invalid;
    
    accountId = strtol(acct + 3, NULL, 10); /* Skip "LS#", extract the digits */
    sessionKey = aligned_current_type(&a, const char);
    
    /* Check if this client is already authorized (highly likely) */
    if (cs_thread_is_auth_awaiting_client(cs, accountId, sessionKey))
        return cs_thread_add_authed_client(cs, client, accountId, sessionKey);
    
    /* Client wasn't authorized... add it to the clientsAwaitingAuth queue */
    return cs_thread_add_client_awaiting_auth(cs, client);
}

static TlgPacket* cs_thread_write_char_info_packet(CharSelectData* data, uint32_t count)
{
    TlgPacket* packet = packet_create_type(OP_CS_CharacterInfo, PSCS_CharacterInfo);
    Aligned a;
    uint32_t i;
    
    if (!packet) return NULL;
    
    aligned_init(&a, packet_data(packet), packet_length(packet));
    
    /* names */
    for (i = 0; i < count; i++)
    {
        aligned_write_snprintf_and_advance_by(&a, CHAR_SELECT_MAX_NAME_LENGTH, "%s", sbuf_str(data->name[i]));
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_snprintf_and_advance_by(&a, CHAR_SELECT_MAX_NAME_LENGTH, "<none>");
    }
    
    /* levels */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint8(&a, data->level[i]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint8(&a, 0);
    }
    
    /* classIds */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint8(&a, data->classId[i]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint8(&a, 0);
    }
    
    /* raceIds */
    for (i = 0; i < count; i++)
    {
        aligned_write_int16(&a, data->raceId[i]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_int16(&a, 0);
    }
    
    /* zoneShortNames */
    for (i = 0; i < count; i++)
    {
        aligned_write_snprintf_and_advance_by(&a, CHAR_SELECT_MAX_ZONE_SHORTNAME_LENGTH, "%s", zone_short_name_by_id(data->zoneId[i]));
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_snprintf_and_advance_by(&a, CHAR_SELECT_MAX_ZONE_SHORTNAME_LENGTH, "qeynos");
    }
    
    /* genderIds */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint8(&a, data->genderId[i]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint8(&a, 0);
    }
    
    /* faceIds */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint8(&a, data->faceId[i]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint8(&a, 0);
    }
    
    /* materialIds */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint8(&a, data->materialIds[i][0]);
        aligned_write_uint8(&a, data->materialIds[i][1]);
        aligned_write_uint8(&a, data->materialIds[i][2]);
        aligned_write_uint8(&a, data->materialIds[i][3]);
        aligned_write_uint8(&a, data->materialIds[i][4]);
        aligned_write_uint8(&a, data->materialIds[i][5]);
        aligned_write_uint8(&a, data->materialIds[i][6]);
        aligned_write_uint8(&a, data->materialIds[i][7]);
        aligned_write_uint8(&a, data->materialIds[i][8]);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_zeroes(&a, CHAR_SELECT_MATERIAL_COUNT);
    }
    
    /* unknownA */
    aligned_write_zeroes(&a, sizeof_field(PSCS_CharacterInfo, unknownA));
    
    /* materialTints */
    for (i = 0; i < count; i++)
    {
        aligned_write_uint32(&a, data->materialTints[i][0]);
        aligned_write_uint32(&a, data->materialTints[i][1]);
        aligned_write_uint32(&a, data->materialTints[i][2]);
        aligned_write_uint32(&a, data->materialTints[i][3]);
        aligned_write_uint32(&a, data->materialTints[i][4]);
        aligned_write_uint32(&a, data->materialTints[i][5]);
        aligned_write_uint32(&a, data->materialTints[i][6]);
        /* Weapon slots are never tinted */
        aligned_write_uint32(&a, 0);
        aligned_write_uint32(&a, 0);
    }
    
    for (; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_zeroes(&a, sizeof(uint32_t) * CHAR_SELECT_MATERIAL_COUNT);
    }
    
    /* unknownB */
    aligned_write_zeroes(&a, sizeof_field(PSCS_CharacterInfo, unknownB));
    
    /*
        The weird fields:
        We need to set these (probably not exactly as below, but close enough) to be able
        to tell which character slot is being referred to when the client asks for which
        weapon models to display in the characters' hands, via WearChange requests.
    */
    
    /* weirdA */
    for (i = 0; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint32(&a, i);
    }
    
    /* weirdB */
    for (i = 0; i < CHAR_SELECT_MAX_CHARS; i++)
    {
        aligned_write_uint32(&a, i);
    }
    
    /* unknownC */
    aligned_write_zeroes(&a, sizeof_field(PSCS_CharacterInfo, unknownC));
    
    return packet;
}

static void cs_thread_handle_db_character_info(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->db.zResult.rCSCharacterInfo.client;
    CharSelectData* data = zpacket->db.zResult.rCSCharacterInfo.data;
    uint32_t count = zpacket->db.zResult.rCSCharacterInfo.count;
    TlgPacket* packet;
    int rc;
    
    switch (cs_thread_check_db_error(cs, client, zpacket, FUNC_NAME))
    {
    case ERR_Invalid:
        goto free_data;
    
    case ERR_Generic:
        goto drop_client;
    
    case ERR_None:
        break;
    }
    
    if (!csc_is_authed(client))
        goto drop_client;

    csc_set_weapon_material_ids(client, data);
    
    packet = cs_thread_write_char_info_packet(data, count);
    if (!packet) goto drop_client;
    
    rc = csc_schedule_packet(client, cs->udpQueue, packet);
    if (rc) goto drop_client;
    
    goto free_data;
    
drop_client:
    cs_thread_drop_client(cs, client);
free_data:
    if (data)
    {
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            data->name[i] = sbuf_drop(data->name[i]);
        }
        
        free(data);
    }
}

static int cs_thread_handle_op_guild_list(CharSelectThread* cs, CharSelectClient* client)
{
    if (!csc_is_authed(client))
        return ERR_Invalid;

    return csc_schedule_packet(client, cs->udpQueue, cs->packetGuildList);
}

static int cs_thread_handle_op_name_approval(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    ZPacket query;
    Aligned a;
    const char* name;
    int len;
    int rc;

    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);

    if (!csc_is_authed(client) || aligned_remaining_data(&a) < sizeof(PSCS_NameApproval))
        return ERR_Invalid;

    name = aligned_current_type(&a, const char);
    len = str_len_bounded(name, (int)sizeof_field(PSCS_NameApproval, name));

    if (len == 0) return ERR_Invalid;

    query.db.zQuery.qCSCharacterNameAvailable.client = client;
    query.db.zQuery.qCSCharacterNameAvailable.name = sbuf_create(name, (int)len);
    sbuf_grab(query.db.zQuery.qCSCharacterNameAvailable.name);

    rc = db_queue_query(cs->dbQueue, cs->csQueue, cs->dbId, cs->nextQueryId++, ZOP_DB_QueryCSCharacterNameAvailable, &query);

    if (rc)
        sbuf_drop(query.db.zQuery.qCSCharacterNameAvailable.name);

    return rc;
}

static int cs_thread_send_name_approval(CharSelectThread* cs, CharSelectClient* client, bool isAvailable)
{
    TlgPacket* packet = packet_create(OP_CS_NameApproval, 1);
    if (!packet) return ERR_OutOfMemory;

    packet_data(packet)[0] = (uint8_t)isAvailable;

    return csc_schedule_packet(client, cs->udpQueue, packet);
}

static void cs_thread_handle_db_name_approval(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->db.zResult.rCSCharacterInfo.client;
    bool isAvailable = zpacket->db.zResult.rCSCharacterNameAvailable.isNameAvailable;
    int rc;

    switch (cs_thread_check_db_error(cs, client, zpacket, FUNC_NAME))
    {
    case ERR_Invalid:
    case ERR_Generic:
        goto drop_client;

    case ERR_None:
        break;
    }

    if (!csc_is_authed(client))
        goto drop_client;

    rc = cs_thread_send_name_approval(cs, client, isAvailable);
    if (rc) goto drop_client;

    csc_set_name_approved(client, isAvailable);
    return;

drop_client:
    cs_thread_drop_client(cs, client);
}

static bool cs_thread_create_character_is_valid(CharCreateData* data)
{
    uint32_t classIndex = data->classId - 1;
    uint32_t raceIndex = data->raceId - 1;
    uint32_t totalExpected, total, maxSpendableOneStat;
    int i;

#define classes_bitmap(war, clr, pal, rng, shd, dru, mnk, brd, rog, shm, nec, wiz, mag, enc)    \
    ((war << 0) | (clr << 1) | (pal << 2) | (rng << 3) | (shd << 4) | (dru << 5) | (mnk << 6) | (brd << 7) | (rog << 8) | (shm << 9) | (nec << 10) | (wiz << 11) | (mag << 12) | (enc << 13))
    static const uint16_t classesByRace[13] = {
        /*             WAR    CLR    PAL    RNG    SHD    DRU    MNK    BRD    ROG    SHM    NEC    WIZ    MAG    ENC */
        classes_bitmap(true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  true,  true,  true ),   /* Human */
        classes_bitmap(true,  false, false, false, false, false, false, false, true,  true,  false, false, false, false),   /* Barbarian */
        classes_bitmap(false, true,  true,  false, true,  false, false, false, false, false, true,  true,  true,  true ),   /* Erudite */
        classes_bitmap(true,  false, false, true,  false, true,  false, true,  true,  false, false, false, false, false),   /* Wood Elf */
        classes_bitmap(false, true,  true,  false, false, false, false, false, false, false, false, true,  true,  true ),   /* High Elf */
        classes_bitmap(true,  true,  false, false, true,  false, false, false, true,  false, true,  true,  true,  true ),   /* Dark Elf */
        classes_bitmap(true,  false, true,  true,  false, true,  false, true,  true,  false, false, false, false, false),   /* Half Elf */
        classes_bitmap(true,  true,  true,  false, false, false, false, false, true,  false, false, false, false, false),   /* Dwarf */
        classes_bitmap(true,  false, false, false, true,  false, false, false, false, true,  false, false, false, false),   /* Troll */
        classes_bitmap(true,  false, false, false, true,  false, false, false, false, true,  false, false, false, false),   /* Ogre */
        classes_bitmap(true,  true,  true,  true,  false, true,  false, false, true,  false, false, false, false, false),   /* Halfling */
        classes_bitmap(true,  true,  false, false, false, false, false, false, true,  false, true,  true,  true,  true ),   /* Gnome */
        classes_bitmap(true,  false, false, false, true,  false, true,  false, false, true,  true,  false, false, false)    /* Iksar */
    };
#undef classes_bitmap

    static const uint8_t baseStatsByRace[13][7] = {
        /*                 STR  STA  CHA  DEX  INT  AGI  WIS */
        /* Human     */ {  75,  75,  75,  75,  75,  75,  75 },
        /* Barbarian */ { 103,  95,  55,  70,  60,  82,  70 },
        /* Erudite   */ {  60,  70,  70,  70, 107,  70,  83 },
        /* Wood Elf  */ {  65,  65,  75,  80,  75,  95,  80 },
        /* High Elf  */ {  55,  65,  80,  70,  92,  85,  95 },
        /* Dark Elf  */ {  60,  65,  60,  75,  99,  90,  83 },
        /* Half Elf  */ {  70,  70,  75,  85,  75,  90,  60 },
        /* Dwarf     */ {  90,  90,  45,  90,  60,  70,  83 },
        /* Troll     */ { 108, 109,  40,  75,  52,  83,  60 },
        /* Ogre      */ { 130, 122,  37,  70,  60,  70,  67 },
        /* Halfling  */ {  70,  75,  50,  90,  67,  95,  80 },
        /* Gnome     */ {  60,  70,  60,  85,  98,  85,  67 },
        /* Iksar     */ {  70,  70,  55,  85,  75,  90,  80 }
    };

    static const uint8_t statBonusesByClass[14][7] = {
        /*                    STR STA CHA DEX INT AGI WIS */
        /* Warrior      */  { 10, 10,  0,  0,  0,  5,  0 },
        /* Cleric       */  {  5,  5,  0,  0,  0,  0, 10 },
        /* Paladin      */  { 10,  5, 10,  0,  0,  0,  5 },
        /* Ranger       */  {  5, 10,  0,  0,  0, 10,  5 },
        /* ShadowKnight */  { 10,  5,  5,  0, 10,  0,  0 },
        /* Druid        */  {  0, 10,  0,  0,  0,  0, 10 },
        /* Monk         */  {  5,  5,  0, 10,  0, 10,  0 },
        /* Bard         */  {  5,  0, 10, 10,  0,  0,  0 },
        /* Rogue        */  {  0,  0,  0, 10,  0, 10,  0 },
        /* Shaman       */  {  0,  5,  5,  0,  0,  0, 10 },
        /* Necromancer  */  {  0,  0,  0, 10, 10,  0,  0 },
        /* Wizard       */  {  0, 10,  0,  0, 10,  0,  0 },
        /* Magician     */  {  0, 10,  0,  0, 10,  0,  0 },
        /* Enchanter    */  {  0,  0, 10,  0, 10,  0,  0 }
    };

    static const uint8_t spendableStatPointsByClass[14] = {
        25, /* Warrior */
        30, /* Cleric */
        20, /* Paladin */
        20, /* Ranger */
        20, /* ShadowKnight */
        30, /* Druid */
        20, /* Monk */
        25, /* Bard */
        30, /* Rogue */
        30, /* Shaman */
        30, /* Necromancer */
        30, /* Wizard */
        30, /* Magician */
        30  /* Enchanter */
    };

    /* Iksar index correction */
    if (raceIndex > 12)
        raceIndex = 12;

    /* Is the race/class combination valid? */
    if (classIndex > 14 || (classesByRace[raceIndex] & (1 << classIndex)) == 0)
        return false;

    totalExpected = spendableStatPointsByClass[classIndex];
    total = 0;
    maxSpendableOneStat = totalExpected;

    if (maxSpendableOneStat > 25)
        maxSpendableOneStat = 25;

    for (i = 0; i < 7; i++)
    {
        uint32_t statValue = data->stats[i];
        uint32_t statTotal = baseStatsByRace[raceIndex][i] + statBonusesByClass[classIndex][i];

        /* Does this stat exceed the maximum possible value for this race and class? */
        if (statValue > (statTotal + maxSpendableOneStat))
            return false;

        totalExpected += statTotal;
        total += statValue;
    }

    /* Do they have more stat points overall than should be possible? */
    if (total > totalExpected)
        return false;

    return true;
}

static int cs_thread_handle_op_create_character(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    CharCreateData data;
    const char* shortName;
    Aligned a;

    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);

    if (!csc_is_authed(client) || !csc_is_name_approved(client) || aligned_remaining_data(&a) < sizeof(PSCS_PlayerProfile))
        return ERR_Invalid;

    /* name */
    aligned_read_buffer(&a, data.name, sizeof(data.name));
    /* surname */
    aligned_advance_by(&a, sizeof_field(PSCS_PlayerProfile, surname));
    /* genderId */
    data.genderId = (uint8_t)aligned_read_uint16(&a);
    /* raceId */
    data.raceId = aligned_read_int16(&a);
    /* classId */
    data.classId = (uint8_t)aligned_read_uint16(&a);
    /* level, experience, trainingPoints, currentMana */
    aligned_advance_by(&a,
        sizeof_field(PSCS_PlayerProfile, level)             +
        sizeof_field(PSCS_PlayerProfile, experience)        +
        sizeof_field(PSCS_PlayerProfile, trainingPoints)    +
        sizeof_field(PSCS_PlayerProfile, currentMana));
    /* faceId */
    data.faceId = aligned_read_uint8(&a);
    /* unknownA */
    aligned_advance_by(&a, sizeof_field(PSCS_PlayerProfile, unknownA));
    /* currentHp */
    data.currentHp = (uint8_t)aligned_read_int16(&a);
    /* unknownB */
    aligned_advance_by(&a, sizeof_field(PSCS_PlayerProfile, unknownB));
    /* STR */
    data.STR = aligned_read_uint8(&a);
    /* STA */
    data.STA = aligned_read_uint8(&a);
    /* CHA */
    data.CHA = aligned_read_uint8(&a);
    /* DEX */
    data.DEX = aligned_read_uint8(&a);
    /* INT */
    data.INT = aligned_read_uint8(&a);
    /* AGI */
    data.AGI = aligned_read_uint8(&a);
    /* WIS */
    data.WIS = aligned_read_uint8(&a);
    /* languages, unknownC, mainInventory, buffs, baggedItems, spellbook, memmedSpellIds, unknownD */
    aligned_advance_by(&a,
        sizeof_field(PSCS_PlayerProfile, languages)                     +
        sizeof_field(PSCS_PlayerProfile, unknownC)                      +
        sizeof_field(PSCS_PlayerProfile, mainInventoryItemIds)          +
        sizeof_field(PSCS_PlayerProfile, mainInventoryInternalUnused)   +
        sizeof_field(PSCS_PlayerProfile, mainInventoryItemProperties)   +
        sizeof_field(PSCS_PlayerProfile, buffs)                         +
        sizeof_field(PSCS_PlayerProfile, baggedItemIds)                 +
        sizeof_field(PSCS_PlayerProfile, baggedItemProperties)          +
        sizeof_field(PSCS_PlayerProfile, spellbook)                     +
        sizeof_field(PSCS_PlayerProfile, memmedSpellIds)                +
        sizeof_field(PSCS_PlayerProfile, unknownD));
    
    /* fixme: none of the loc and zone data coming from the client should be trusted */
    
    /* y */
    data.y = aligned_read_float(&a);
    /* x */
    data.x = aligned_read_float(&a);
    /* z */
    data.z = aligned_read_float(&a);
    /* heading */
    data.heading = aligned_read_float(&a);
    /* zoneShortName */
    shortName = aligned_current_type(&a, const char);
    data.zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_ZONE_SHORT_NAME_LENGTH);

    /* lots of stuff */
    aligned_advance_by(&a,
        sizeof_field(PSCS_PlayerProfile, unknownEDefault100)    +
        sizeof_field(PSCS_PlayerProfile, coins)                 +
        sizeof_field(PSCS_PlayerProfile, coinsBank)             +
        sizeof_field(PSCS_PlayerProfile, coinsCursor)           +
        sizeof_field(PSCS_PlayerProfile, skills)                +
        sizeof_field(PSCS_PlayerProfile, unknownF)              +
        sizeof_field(PSCS_PlayerProfile, autoSplit)             +
        sizeof_field(PSCS_PlayerProfile, pvpEnabled)            +
        sizeof_field(PSCS_PlayerProfile, unknownG)              +
        sizeof_field(PSCS_PlayerProfile, isGM)                  +
        sizeof_field(PSCS_PlayerProfile, unknownH)              +
        sizeof_field(PSCS_PlayerProfile, disciplinesReady)      +
        sizeof_field(PSCS_PlayerProfile, unknownI)              +
        sizeof_field(PSCS_PlayerProfile, hunger)                +
        sizeof_field(PSCS_PlayerProfile, thirst)                +
        sizeof_field(PSCS_PlayerProfile, unknownJ));

    /* bindZoneShortName[5] */
    shortName = aligned_current_type(&a, const char);
    data.bindPoint[0].zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH);

    shortName = aligned_current_type(&a, const char);
    data.bindPoint[1].zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH);

    shortName = aligned_current_type(&a, const char);
    data.bindPoint[2].zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH);

    shortName = aligned_current_type(&a, const char);
    data.bindPoint[3].zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH);

    shortName = aligned_current_type(&a, const char);
    data.bindPoint[4].zoneId = zone_id_by_short_name(shortName, str_len_bounded(shortName, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH));
    aligned_advance_by(&a, ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH);

    /* bankInventoryItemProperties, bankedBaggedItemProperties, unknownK */
    aligned_advance_by(&a,
        sizeof_field(PSCS_PlayerProfile, bankInventoryItemProperties)   +
        sizeof_field(PSCS_PlayerProfile, bankBaggedItemProperties)      +
        sizeof_field(PSCS_PlayerProfile, unknownK)                      +
        sizeof_field(PSCS_PlayerProfile, unknownL));

    /* bindLocY[5] */
    data.bindPoint[0].y = aligned_read_float(&a);
    data.bindPoint[1].y = aligned_read_float(&a);
    data.bindPoint[2].y = aligned_read_float(&a);
    data.bindPoint[3].y = aligned_read_float(&a);
    data.bindPoint[4].y = aligned_read_float(&a);

    /* bindLocX[5] */
    data.bindPoint[0].x = aligned_read_float(&a);
    data.bindPoint[1].x = aligned_read_float(&a);
    data.bindPoint[2].x = aligned_read_float(&a);
    data.bindPoint[3].x = aligned_read_float(&a);
    data.bindPoint[4].x = aligned_read_float(&a);

    /* bindLocZ[5] */
    data.bindPoint[0].z = aligned_read_float(&a);
    data.bindPoint[1].z = aligned_read_float(&a);
    data.bindPoint[2].z = aligned_read_float(&a);
    data.bindPoint[3].z = aligned_read_float(&a);
    data.bindPoint[4].z = aligned_read_float(&a);

    /* More stuff */
    aligned_advance_by(&a,
        sizeof_field(PSCS_PlayerProfile, bindLocHeading)                +
        sizeof_field(PSCS_PlayerProfile, bankInventoryInternalUnused)   +
        sizeof_field(PSCS_PlayerProfile, unknownM)                      +
        sizeof_field(PSCS_PlayerProfile, unixTimeA)                     +
        sizeof_field(PSCS_PlayerProfile, unknownN)                      +
        sizeof_field(PSCS_PlayerProfile, unknownODefault1)              +
        sizeof_field(PSCS_PlayerProfile, unknownP)                      +
        sizeof_field(PSCS_PlayerProfile, bankInventoryItemIds)          +
        sizeof_field(PSCS_PlayerProfile, bankBaggedItemIds));

    /* deity */
    data.deityId = aligned_read_uint16(&a);

    /* Verify char creation data validity */
    if (cs_thread_create_character_is_valid(&data))
    {
        ZPacket query;
        int rc;

        query.db.zQuery.qCSCharacterCreate.client = client;
        query.db.zQuery.qCSCharacterCreate.accountId = csc_get_account_id(client);
        query.db.zQuery.qCSCharacterCreate.data = alloc_type(CharCreateData);

        if (!query.db.zQuery.qCSCharacterCreate.data)
            return ERR_OutOfMemory;

        memcpy(query.db.zQuery.qCSCharacterCreate.data, &data, sizeof(data));

        rc = db_queue_query(cs->dbQueue, cs->csQueue, cs->dbId, cs->nextQueryId++, ZOP_DB_QueryCSCharacterCreate, &query);

        if (rc == ERR_None)
        {
            log_writef(cs->logQueue, cs->logId, "Creating character \"%s\" with race %s, gender %s, class %s, deity %s and starting zone %s (%s) for account id %lli",
                data.name, race_id_to_str(data.raceId), gender_id_to_str(data.genderId), class_id_to_str(data.classId), deity_id_to_str(data.deityId),
                zone_long_name_by_id(data.zoneId), zone_short_name_by_id(data.zoneId), csc_get_account_id(client));
        }
        else
        {
            free(query.db.zQuery.qCSCharacterCreate.data);
        }

        return rc;
    }

    return ERR_Invalid;
}

static void cs_thread_handle_db_character_create(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->db.zResult.rCSCharacterCreate.client;
    int rc;

    if (zpacket->db.zResult.hadErrorUnprocessed)
    {
        log_writef(cs->logQueue, cs->logId, "WARNING: %s: database reported error, query unprocessed", FUNC_NAME);
        return;
    }

    if (!cs_thread_is_client_valid(cs, client))
    {
        log_writef(cs->logQueue, cs->logId, "WARNING: %s: received database result for CharSelectClient that no longer exists", FUNC_NAME);
        return;
    }

    /* Reset the name approval flag, applies to this character creation attempt only */
    csc_set_name_approved(client, false);

    if (zpacket->db.zResult.hadError)
    {
        rc = cs_thread_send_name_approval(cs, client, false);
    }
    else
    {
        rc = cs_thread_trigger_send_char_info(cs, client, csc_get_account_id(client));
    }

    if (rc)
        cs_thread_drop_client(cs, client);
}

static int cs_thread_handle_op_delete_character(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    const char* name;
    uint32_t len;
    Aligned a;
    ZPacket query;
    int rc;

    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);

    len = aligned_remaining_data(&a);

    if (!csc_is_authed(client) || len < 2)
        goto invalid;

    name = aligned_current_type(&a, const char);
    len = str_len_bounded(name, (int)len);

    if (len == 0)
        goto invalid;

    query.db.zQuery.qCSCharacterDelete.accountId = csc_get_account_id(client);
    query.db.zQuery.qCSCharacterDelete.name = sbuf_create(name, len);

    if (!query.db.zQuery.qCSCharacterDelete.name)
        return ERR_OutOfMemory;

    sbuf_grab(query.db.zQuery.qCSCharacterDelete.name);

    rc = db_queue_query(cs->dbQueue, cs->csQueue, cs->dbId, cs->nextQueryId++, ZOP_DB_QueryCSCharacterDelete, &query);

    if (rc)
        sbuf_drop(query.db.zQuery.qCSCharacterDelete.name);

    return rc;
invalid:
    return ERR_Invalid;
}

static int cs_thread_handle_op_wear_change(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    Aligned a;
    uint32_t slot, op, flag, index;

    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);

    if (!csc_is_authed(client) || aligned_remaining_data(&a) < sizeof(PSCS_WearChange))
        return ERR_Invalid;

    /* PSCS_WearChange */
    /* unusedSpawnId */
    aligned_advance_by(&a, sizeof_field(PSCS_WearChange, unusedSpawnId));
    /* slotId */
    slot = aligned_read_uint8(&a);
    /* materialId */
    index = aligned_read_uint8(&a);
    /* operationId */
    op = aligned_read_uint16(&a);
    /* color, unknownA */
    aligned_advance_by(&a, sizeof_field(PSCS_WearChange, color) + sizeof_field(PSCS_WearChange, unknownA));
    /* flag */
    flag = aligned_read_uint8(&a);

    /* If the flag is 0xb1, this packet is some kind of echo; don't reply or it'll cause endless echo spam */
    if (op != CHAR_SELECT_THREAD_WEARCHANGE_OP_SWITCH_CHAR && flag != 0xb1)
    {
        TlgPacket* packet;

        if (index >= 10 || (slot != 7 && slot != 8))
            return ERR_None;

        packet = packet_create_type(OP_CS_WearChange, PSCS_WearChange);
        if (!packet) return ERR_OutOfMemory;

        aligned_init(&a, packet_data(packet), packet_length(packet));

        /* PSCS_WearChange */
        /* unusedSpawnId */
        aligned_write_uint32(&a, 0);
        /* slotId */
        aligned_write_uint8(&a, slot);
        /* materialId */
        aligned_write_uint8(&a, csc_get_weapon_material_id(client, index, slot));
        /* operationId, color, unknownA, flag, unknownB */
        aligned_write_zeroes(&a, aligned_remaining_space(&a));

        return csc_schedule_packet(client, cs->udpQueue, packet);
    }

    return ERR_None;
}

static int cs_thread_handle_op_enter(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    const char* name;
    uint32_t len;
    Aligned a;
    ZPacket toMain;
    int rc;

    /* Don't let the client try to queue multiple zone ins at once */
    if (csc_is_zoning(client))
        return ERR_Again;

    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);

    len = aligned_remaining_data(&a);

    if (!csc_is_authed(client) || len < 2)
        goto invalid;

    name = aligned_current_type(&a, const char);
    len = str_len_bounded(name, (int)len);

    if (len == 0)
        goto invalid;

    toMain.main.zZoneFromCharSelect.client = client;
    toMain.main.zZoneFromCharSelect.accountId = csc_get_account_id(client);
    toMain.main.zZoneFromCharSelect.isLocal = csc_is_local(client);
    toMain.main.zZoneFromCharSelect.name = sbuf_create(name, len);

    if (!toMain.main.zZoneFromCharSelect.name)
        return ERR_OutOfMemory;

    sbuf_grab(toMain.main.zZoneFromCharSelect.name);

    rc = ringbuf_push(cs->mainQueue, ZOP_MAIN_ZoneFromCharSelect, &toMain);

    if (rc == ERR_None)
    {
        csc_set_is_zoning(client, true);
    }
    else
    {
        sbuf_drop(toMain.main.zZoneFromCharSelect.name);
    }

    return rc;
invalid:
    return ERR_Invalid;
}

static int cs_thread_handle_op_echo(CharSelectThread* cs, CharSelectClient* client, uint16_t opcode)
{
    TlgPacket* packet = NULL;
    
    switch (opcode)
    {
    case OP_CS_Echo1:
        packet = cs->packetEcho1;
        break;
    
    case OP_CS_Echo2:
        packet = cs->packetEcho2;
        break;
    
    case OP_CS_Echo3:
        packet = cs->packetEcho3;
        break;
    
    case OP_CS_Echo4:
        packet = cs->packetEcho4;
        break;
    
    case OP_CS_Echo5:
        packet = cs->packetEcho5;
        break;
    }
    
    return csc_schedule_packet(client, cs->udpQueue, packet);
}

static void cs_thread_handle_packet(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->udp.zToServerPacket.clientObject;
    byte* data = zpacket->udp.zToServerPacket.data;
    uint16_t opcode = zpacket->udp.zToServerPacket.opcode;
    int rc = ERR_None;

    switch (opcode)
    {
    case OP_CS_LoginInfo:
        rc = cs_thread_handle_op_login_info(cs, client, zpacket);
        break;

    case OP_CS_GuildList:
        rc = cs_thread_handle_op_guild_list(cs, client);
        break;

    case OP_CS_NameApproval:
        rc = cs_thread_handle_op_name_approval(cs, client, zpacket);
        break;

    case OP_CS_CreateCharacter:
        rc = cs_thread_handle_op_create_character(cs, client, zpacket);
        break;

    case OP_CS_DeleteCharacter:
        rc = cs_thread_handle_op_delete_character(cs, client, zpacket);
        break;
    
    case OP_CS_WearChange:
        rc = cs_thread_handle_op_wear_change(cs, client, zpacket);
        break;

    case OP_CS_Enter:
        rc = cs_thread_handle_op_enter(cs, client, zpacket);
        break;

    case OP_CS_Echo1:
    case OP_CS_Echo2:
    case OP_CS_Echo3:
    case OP_CS_Echo4:
    case OP_CS_Echo5:
        rc = cs_thread_handle_op_echo(cs, client, opcode);
        break;

    case OP_CS_Ignore1:
    case OP_CS_Ignore2:
        break;
    
    default:
        log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_handle_packet: received unexpected char select opcode: 0x%04x", zpacket->udp.zToServerPacket.opcode);
        break;
    }
    
    if (rc)
    {
        uint32_t ip = csc_get_ip(client);
        uint16_t port = csc_get_port(client);
        
        log_writef(cs->logQueue, cs->logId, "ERROR: cs_thread_handle_packet: got error %s while processing packet with opcode %s from client from %u.%u.%u.%u:%u, disconnecting them to maintain consistency",
            enum2str_err(rc), enum2str_char_select_opcode(opcode), (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
        
        cs_thread_drop_client(cs, client);
    }
    
    if (data) free(data);
}

static void cs_thread_handle_login_auth(CharSelectThread* cs, ZPacket* zpacket)
{
    int64_t accountId = zpacket->cs.zLoginAuth.accountId;
    const char* sessionKey = zpacket->cs.zLoginAuth.sessionKey;
    CharSelectClient** clients = cs->clientsAwaitingAuth;
    uint32_t count = cs->clientsAwaitingAuthCount;
    uint32_t i;
    
    /* Check if we already have a client waiting for this auth (very unlikely) */
    for (i = 0; i < count; i++)
    {
        CharSelectClient* client = clients[i];
        
        if (csc_check_auth(client, accountId, sessionKey))
        {
            cs_thread_add_authed_client(cs, client, accountId, sessionKey);
            
            /* Pop and swap */
            count--;
            clients[i] = clients[count];
            cs->clientAwaitingAuthTimeouts[i] = cs->clientAwaitingAuthTimeouts[count];
            cs->clientsAwaitingAuthCount = count;
            return;
        }
    }
    
    count = cs->authsAwaitingClientCount;
    
    if (bit_is_pow2_or_zero(count))
    {
        uint32_t cap = (count == 0) ? 1 : count * 2;
        CharSelectAuth* auths;
        uint64_t* timeouts;
        
        auths = realloc_array_type(cs->authsAwaitingClient, cap, CharSelectAuth);
        if (!auths) goto fail;
        cs->authsAwaitingClient = auths;
        
        timeouts = realloc_array_type(cs->authAwaitingClientTimeouts, cap, uint64_t);
        if (!timeouts) goto fail;
        cs->authAwaitingClientTimeouts = timeouts;
    }
    
    cs->authsAwaitingClient[count].accountId = accountId;
    memcpy(cs->authsAwaitingClient[count].sessionKey, sessionKey, sizeof(cs->authsAwaitingClient[count].sessionKey));
    cs->authAwaitingClientTimeouts[count] = clock_milliseconds() + CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS;
    cs->authsAwaitingClientCount = count + 1;
    
    log_writef(cs->logQueue, cs->logId, "Authorized account id %lli, awaiting matching client connection", accountId);
    return;
    
fail:
    log_writef(cs->logQueue, cs->logId, "ERROR: cs_thread_handle_login_auth: out of memory while adding auth for account id %lli", accountId);
}

static void cs_thread_check_auth_timeouts(CharSelectThread* cs)
{
    uint64_t curTime = clock_milliseconds();
    CharSelectAuth* auths = cs->authsAwaitingClient;
    CharSelectClient** clients = cs->clientsAwaitingAuth;
    uint64_t* timeouts;
    uint32_t n, i;
    
    timeouts = cs->authAwaitingClientTimeouts;
    n = cs->authsAwaitingClientCount;
    i = 0;
    
    while (i < n)
    {
        if (timeouts[i] < curTime)
        {
            /* Pop and swap */
            n--;
            timeouts[i] = timeouts[n];
            auths[i] = auths[n];
            continue;
        }
        
        i++;
    }
    
    cs->authsAwaitingClientCount = n;
    
    timeouts = cs->clientAwaitingAuthTimeouts;
    n = cs->clientsAwaitingAuthCount;
    i = 0;
    
    while (i < n)
    {
        if (timeouts[i] < curTime)
        {
            cs_thread_drop_client(cs, clients[i]);
            
            /* Pop and swap */
            n--;
            timeouts[i] = timeouts[n];
            clients[i] = clients[n];
            continue;
        }
        
        i++;
    }
    
    cs->clientsAwaitingAuthCount = n;
}

static void cs_thread_remove_client_object(CharSelectThread* cs, CharSelectClient* client)
{
    if (client)
    {
        CharSelectClient** clients = cs->clients;
        uint32_t n = cs->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == client)
            {
                /* Pop and swap */
                n--;
                clients[i] = clients[n];
                cs->clientCount = n;
                goto finish;
            }
        }
        
        clients = cs->clientsAwaitingAuth;
        n = cs->clientsAwaitingAuthCount;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == client)
            {
                /* Pop and swap */
                n--;
                clients[i] = clients[n];
                cs->clientAwaitingAuthTimeouts[i] = cs->clientAwaitingAuthTimeouts[n];
                cs->clientsAwaitingAuthCount = n;
                goto finish;
            }
        }
        
    finish:
        csc_destroy(client);
    }
}

static void cs_thread_proc(void* ptr)
{
    CharSelectThread* cs = (CharSelectThread*)ptr;
    RingBuf* csQueue = cs->csQueue;
    ZPacket zpacket;
    int zop;
    int rc;
    
    for (;;)
    {
        /* The char select thread is purely packet-driven, always blocks until something needs to be done */
        rc = ringbuf_wait_for_packet(csQueue, &zop, &zpacket);
        
        if (rc)
        {
            log_writef(cs->logQueue, cs->logId, "ERROR: cs_thread_proc: ringbuf_wait_for_packet() returned error: %s", enum2str_err(rc));
            break;
        }
        
        switch (zop)
        {
        case ZOP_UDP_ToServerPacket:
            cs_thread_handle_packet(cs, &zpacket);
            break;

        case ZOP_UDP_NewClient:
            csc_init(&zpacket);
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ClientDisconnect:
            cs_thread_remove_client_object(cs, (CharSelectClient*)zpacket.udp.zClientDisconnect.clientObject);
            break;
        
        case ZOP_CS_TerminateThread:
            return;

        case ZOP_CS_AddGuild:
            cs_thread_handle_add_guild(cs, &zpacket);
            break;
        
        case ZOP_CS_LoginAuth:
            cs_thread_handle_login_auth(cs, &zpacket);
            break;
        
        case ZOP_CS_CheckAuthTimeouts:
            cs_thread_check_auth_timeouts(cs);
            break;
        
        case ZOP_DB_QueryCSCharacterInfo:
            cs_thread_handle_db_character_info(cs, &zpacket);
            break;

        case ZOP_DB_QueryCSCharacterNameAvailable:
            cs_thread_handle_db_name_approval(cs, &zpacket);
            break;

        case ZOP_DB_QueryCSCharacterCreate:
            cs_thread_handle_db_character_create(cs, &zpacket);
            break;
        
        default:
            log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_proc: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

static int cs_create_cached_packets(CharSelectThread* cs)
{
    Aligned a;
    
    /* 1-byte packets, content always zero */
    cs->packetLoginApproved = packet_create(OP_CS_LoginApproved, 1);
    if (!cs->packetLoginApproved) goto fail;
    packet_grab(cs->packetLoginApproved);
    packet_data(cs->packetLoginApproved)[0] = 0;
    
    cs->packetEnter = packet_create(OP_CS_Enter, 1);
    if (!cs->packetEnter) goto fail;
    packet_grab(cs->packetEnter);
    packet_data(cs->packetEnter)[0] = 0;
    
    /* Expansion info */
    cs->packetExpansionInfo = packet_create_type(OP_CS_ExpansionInfo, uint32_t);
    if (!cs->packetExpansionInfo) goto fail;
    packet_grab(cs->packetExpansionInfo);
    
    aligned_init(&a, packet_data(cs->packetExpansionInfo), packet_length(cs->packetExpansionInfo));
    aligned_write_uint32(&a, 1 | 2); /* Bitfield: 1 = Kunark, 2 = Velious, 4 = Luclin */
    
    /* Zero-length echo packets */
    cs->packetEcho1 = packet_create_opcode_only(OP_CS_Echo1);
    if (!cs->packetEcho1) goto fail;
    packet_grab(cs->packetEcho1);
    
    cs->packetEcho2 = packet_create_opcode_only(OP_CS_Echo2);
    if (!cs->packetEcho2) goto fail;
    packet_grab(cs->packetEcho2);
    
    cs->packetEcho3 = packet_create_opcode_only(OP_CS_Echo3);
    if (!cs->packetEcho3) goto fail;
    packet_grab(cs->packetEcho3);
    
    cs->packetEcho4 = packet_create_opcode_only(OP_CS_Echo4);
    if (!cs->packetEcho4) goto fail;
    packet_grab(cs->packetEcho4);
    
    cs->packetEcho5 = packet_create_opcode_only(OP_CS_Echo5);
    if (!cs->packetEcho5) goto fail;
    packet_grab(cs->packetEcho5);
    
    return ERR_None;
fail:
    return ERR_OutOfMemory;
}

CharSelectThread* cs_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp)
{
    CharSelectThread* cs = alloc_type(CharSelectThread);
    ZPacket zpacket;
    int rc;
    
    if (!cs) goto fail_alloc;
    
    cs->clientCount = 0;
    cs->authsAwaitingClientCount = 0;
    cs->clientsAwaitingAuthCount = 0;
    cs->guildCount = 0;
    cs->nextQueryId = 1;
    cs->dbId = dbId;
    cs->clients = NULL;
    cs->authsAwaitingClient = NULL;
    cs->clientsAwaitingAuth = NULL;
    cs->authAwaitingClientTimeouts = NULL;
    cs->clientAwaitingAuthTimeouts = NULL;
    cs->guildList = NULL;
    cs->csQueue = NULL;
    cs->mainQueue = mainQueue;
    cs->udpQueue = udp_get_queue(udp);
    cs->dbQueue = dbQueue;
    cs->logQueue = log_get_queue(log);
    
    cs->packetLoginApproved = NULL;
    cs->packetEnter = NULL;
    cs->packetExpansionInfo = NULL;
    cs->packetGuildList = NULL;
    cs->packetEcho1 = NULL;
    cs->packetEcho2 = NULL;
    cs->packetEcho3 = NULL;
    cs->packetEcho4 = NULL;
    cs->packetEcho5 = NULL;
    
    rc = log_open_file_literal(log, &cs->logId, CHAR_SELECT_THREAD_LOG_PATH);
    if (rc) goto fail;

    cs->csQueue = ringbuf_create_type(ZPacket, 1024);
    if (!cs->csQueue) goto fail;

    /* Initialize the default cached guild packet as soon as the thread starts */
    zpacket.cs.zAddGuild.isLast = true;
    zpacket.cs.zAddGuild.guild.guildId = 0;
    zpacket.cs.zAddGuild.guild.guildName = NULL;
    rc = ringbuf_push(cs->csQueue, ZOP_CS_AddGuild, &zpacket);
    if (rc) goto fail;
    
    rc = cs_create_cached_packets(cs);
    if (rc) goto fail;
    
    rc = udp_open_port(udp, CHAR_SELECT_THREAD_UDP_PORT, csc_sizeof(), cs->csQueue);
    if (rc) goto fail;

    rc = thread_start(cs_thread_proc, cs);
    if (rc) goto fail;
    
    return cs;
    
fail:
    cs_destroy(cs);
fail_alloc:
    return NULL;
}

CharSelectThread* cs_destroy(CharSelectThread* cs)
{
    if (cs)
    {
        cs->csQueue = ringbuf_destroy(cs->csQueue);
        
        cs->packetLoginApproved = packet_drop(cs->packetLoginApproved);
        cs->packetEnter = packet_drop(cs->packetEnter);
        cs->packetExpansionInfo = packet_drop(cs->packetExpansionInfo);
        cs->packetGuildList = packet_drop(cs->packetGuildList);
        cs->packetEcho1 = packet_drop(cs->packetEcho1);
        cs->packetEcho2 = packet_drop(cs->packetEcho2);
        cs->packetEcho3 = packet_drop(cs->packetEcho3);
        cs->packetEcho4 = packet_drop(cs->packetEcho4);
        cs->packetEcho5 = packet_drop(cs->packetEcho5);
        
        free(cs);
    }
    
    return NULL;
}

RingBuf* cs_get_queue(CharSelectThread* cs)
{
    return cs->csQueue;
}
