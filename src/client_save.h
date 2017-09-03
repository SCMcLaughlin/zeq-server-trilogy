
#ifndef CLIENT_SAVE_H
#define CLIENT_SAVE_H

#include "define.h"
#include "buffer.h"
#include "loc.h"
#include "misc_struct.h"
#include "skills.h"
#include "spellbook.h"

struct Client;

typedef struct {
    uint32_t    itemId;
    uint16_t    slotId;
    uint8_t     charges;
    uint8_t     stackAmt;
} ClientSaveInvSlot;

typedef struct {
    uint16_t    spellId;
    uint64_t    recastTimestamp;
} ClientSaveMemmedSpell;

typedef struct ClientSave {
    int64_t                 characterId;
    int64_t                 experience;
    uint64_t                harmtouchTimestamp;
    uint64_t                disciplineTimestamp;
    int64_t                 curHp;
    int64_t                 curMana;
    StaticBuffer*           name;
    StaticBuffer*           surname;
    uint8_t                 level;
    uint8_t                 classId;
    uint8_t                 genderId;
    uint8_t                 faceId;
    uint8_t                 guildRank;
    bool                    isGM;
    bool                    isAutosplit;
    bool                    isPvP;
    uint8_t                 anonSetting;
    uint16_t                raceId;
    uint16_t                deityId;
    int16_t                 zoneId;
    int16_t                 instId;
    uint16_t                trainingPoints;
    uint16_t                hunger;
    uint16_t                thirst;
    uint16_t                drunkeness;
    uint32_t                guildId;
    float                   x;
    float                   y;
    float                   z;
    float                   heading;
    Coin                    coin;
    Coin                    coinCursor;
    Coin                    coinBank;
    uint8_t                 materialIds[9];
    uint32_t                tints[7];
    BindPoint               bindPoints[5];
    Skills                  skills;
    ClientSaveMemmedSpell   memmedSpells[MEMMED_SPELL_SLOTS];
    uint32_t                itemCount;
    uint32_t                cursorQueueCount;
    uint32_t                spellbookCount;
    ClientSaveInvSlot*      items;
    ClientSaveInvSlot*      cursorQueue;
    SpellbookSlot*          spellbook;
} ClientSave;

ZEQ_API void client_save(struct Client* client);
void client_save_destroy(ClientSave* save);

#endif/*CLIENT_SAVE_H*/
