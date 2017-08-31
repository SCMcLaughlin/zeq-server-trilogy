
#include "spellbook.h"
#include "aligned.h"
#include "client_save.h"
#include "player_profile_packet_struct.h"
#include "util_alloc.h"

void spellbook_deinit(Spellbook* sb)
{
    free_if_exists(sb->knownSpells);
}

int spellbook_save_all(Spellbook* sb, ClientSave* save)
{
    SpellbookSlot* slots = sb->knownSpells;
    uint32_t n = sb->count;

    save->spellbookCount = n;

    if (n == 0) return ERR_None;

    save->spellbook = alloc_array_type(n, SpellbookSlot);

    if (!save->spellbook) return ERR_OutOfMemory;

    memcpy(save->spellbook, slots, sizeof(SpellbookSlot) * n);
    return ERR_None;
}

void spellbook_write_pp(Spellbook* sb, Aligned* a)
{
    SpellbookSlot* slots = sb->knownSpells;
    uint32_t n = sb->count;
    uint32_t resetTo = aligned_position(a);
    uint32_t i;
    
    assert(MEMMED_SPELL_SLOTS == ZEQ_PP_MEMMED_SPELL_SLOT_COUNT);
    
    /* Spellbook spell ids */
    
    aligned_write_memset_no_advance(a, 0xff, ZEQ_PP_SPELLBOOK_SLOT_COUNT * sizeof(uint16_t));
    
    for (i = 0; i < n; i++)
    {
        SpellbookSlot* slot = &slots[i];
        uint16_t slotId = slot->slotId;
        
        if (slotId >= ZEQ_PP_SPELLBOOK_SLOT_COUNT)
            continue;
        
        aligned_advance_by(a, sizeof(uint16_t) * slotId);
        aligned_write_uint16(a, slot->spellId);
        aligned_reset_cursor_to(a, resetTo);
    }
    
    aligned_advance_by(a, ZEQ_PP_SPELLBOOK_SLOT_COUNT * sizeof(uint16_t));

    /* Memmed spell ids */
    for (i = 0; i < MEMMED_SPELL_SLOTS; i++)
    {
        uint16_t spellId = sb->memmed[i].spellId;
        
        aligned_write_uint16(a, (spellId) ? spellId : 0xffff);
    }
}

void spellbook_write_pp_gem_refresh(Spellbook* sb, Aligned* a, uint64_t time)
{
    uint32_t i;
    
    for (i = 0; i < MEMMED_SPELL_SLOTS; i++)
    {
        uint64_t timestamp = sb->memmed[i].recastTimestamp;
        
        aligned_write_uint32(a, (uint32_t)((time >= timestamp) ? 0 : timestamp - time));
    }
}
