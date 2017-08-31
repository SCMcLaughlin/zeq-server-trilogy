
#ifndef SPELLBOOK_H
#define SPELLBOOK_H

#include "define.h"
#include "aligned.h"

#define MEMMED_SPELL_SLOTS 8

struct ClientSave;

typedef struct {
    uint16_t    slotId;
    uint16_t    spellId;
} SpellbookSlot;

typedef struct {
    uint16_t    spellId;
    uint32_t    castingTimeMs;
    uint64_t    recastTimestamp;
} MemmedSpell;

typedef struct {
    MemmedSpell     memmed[MEMMED_SPELL_SLOTS];
    uint32_t        count;
    SpellbookSlot*  knownSpells;
} Spellbook;

void spellbook_deinit(Spellbook* sb);
#define spellbook_memmed_spells(sb) ((sb)->memmed)
int spellbook_save_all(Spellbook* sb, struct ClientSave* save);
void spellbook_write_pp(Spellbook* sb, Aligned* a);
void spellbook_write_pp_gem_refresh(Spellbook* sb, Aligned* a, uint64_t time);

#endif/*SPELLBOOK_H*/
