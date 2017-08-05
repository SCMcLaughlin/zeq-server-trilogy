
#ifndef DEITY_ID_H
#define DEITY_ID_H

enum DeityId
{
    DEITY_ID_NONE,
    DEITY_ID_Bertoxxulous = 201,
    DEITY_ID_BrellSerilis,
    DEITY_ID_CazicThule,
    DEITY_ID_ErollisiMarr,
    DEITY_ID_Bristlebane,
    DEITY_ID_Innoruuk,
    DEITY_ID_Karana,
    DEITY_ID_MithanielMarr,
    DEITY_ID_Prexus,
    DEITY_ID_Quellious,
    DEITY_ID_RallosZek,
    DEITY_ID_RodcetNife,
    DEITY_ID_SolusekRo,
    DEITY_ID_TheTribunal,
    DEITY_ID_Tunare,
    DEITY_ID_Veeshan,
    DEITY_ID_Agnostic = 396
};

const char* deity_id_to_str(int deityId);

#endif/*DEITY_ID_H*/
