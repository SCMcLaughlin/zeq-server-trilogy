
#include "spell_proto.h"

typedef struct {
    uint32_t        flags;
    uint16_t        fieldCount;
    uint16_t        spellId;
    StaticBuffer*   path;
    StaticBuffer*   name;
} SpellProto;
