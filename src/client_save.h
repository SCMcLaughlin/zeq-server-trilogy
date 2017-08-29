
#ifndef CLIENT_SAVE_H
#define CLIENT_SAVE_H

struct Client;

typedef struct {
    uint32_t    itemId;
    uint16_t    slotId;
    uint8_t     charges;
    uint8_t     stackAmt;
} ClientSaveInvSlot;

typedef struct {
    int64_t             characterId;
    int64_t             experience;
    uint64_t            harmtouchTimestamp;
    uint64_t            disciplineTimestamp;
    uint64_t            creationTimestamp;
    uint32_t            ip;
    uint16_t            trainingPoints;
    uint16_t            hunger;
    uint16_t            thirst;
    uint16_t            drunkeness;
    uint32_t            guildId;
    uint8_t             materialIds[9];
    uint32_t            tints[7];
    uint32_t            itemCount;
    ClientSaveInvSlot*  items;
} ClientSave;

void client_save(struct Client* client);

#endif/*CLIENT_SAVE_H*/
