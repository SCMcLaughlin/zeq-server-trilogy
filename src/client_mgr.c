
#include "client_mgr.h"
#include "db_thread.h"
#include "main_thread.h"
#include "enum_zop.h"

int cmgr_init(MainThread* mt)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    RingBuf* mainQueue = mt_get_queue(mt);
    RingBuf* dbQueue = mt_get_db_queue(mt);
    int dbId = mt_get_db_id(mt);
    int rc;
    
    cmgr->guildCount = 0;
    
    cmgr->guilds = NULL;

    rc = db_queue_query(dbQueue, mainQueue, dbId, 0, ZOP_DB_QueryMainGuildList, NULL);
    if (rc) goto ret;

ret:
    return rc;
}

void cmgr_deinit(ClientMgr* cmgr)
{
    free_if_exists(cmgr->guilds);
}

void cmgr_handle_guild_list(MainThread* mt, ZPacket* zpacket)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    Guild* guilds;
    uint32_t count;

    /* Taking direct ownership of the buffer allocated by the DB thread */
    count = zpacket->db.zResult.rMainGuildList.count;
    guilds = zpacket->db.zResult.rMainGuildList.guilds;

    cmgr->guildCount = count;
    cmgr->guilds = guilds;

    if (guilds && count)
    {
        RingBuf* csQueue = mt_get_cs_queue(mt);
        ZPacket toCS;
        uint32_t i;
        int rc;

        count--;

        for (i = 0; i <= count; i++)
        {
            toCS.cs.zAddGuild.isLast = (i == count);
            toCS.cs.zAddGuild.guild = guilds[i];

            sbuf_grab(toCS.cs.zAddGuild.guild.guildName);

            rc = ringbuf_push(csQueue, ZOP_CS_AddGuild, &toCS);

            if (rc)
            {
                sbuf_drop(toCS.cs.zAddGuild.guild.guildName);
                break;
            }
        }
    }
}
