
#include "spellbook.h"
#include "aligned.h"
#include "player_profile_packet_struct.h"

void spellbook_deinit(Spellbook* sb)
{
    free_if_exists(sb->knownSpells);
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
