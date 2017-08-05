
#include "deity_id.h"

const char* deity_id_to_str(int deityId)
{
    const char* ret;
    switch (deityId)
    {
    case DEITY_ID_Bertoxxulous: ret = "Bertoxxulous"; break;
    case DEITY_ID_BrellSerilis: ret = "Brell Serilis"; break;
    case DEITY_ID_CazicThule: ret = "Cazic-Thule"; break;
    case DEITY_ID_ErollisiMarr: ret = "Erollisi Marr"; break;
    case DEITY_ID_Bristlebane: ret = "Bristlebane"; break;
    case DEITY_ID_Innoruuk: ret = "Innoruuk"; break;
    case DEITY_ID_Karana: ret = "Karana"; break;
    case DEITY_ID_MithanielMarr: ret = "Mithaniel Marr"; break;
    case DEITY_ID_Prexus: ret = "Prexus"; break;
    case DEITY_ID_Quellious: ret = "Quellious"; break;
    case DEITY_ID_RallosZek: ret = "Rallos Zek"; break;
    case DEITY_ID_RodcetNife: ret = "Rodcet Nife"; break;
    case DEITY_ID_SolusekRo: ret = "Solusek Ro"; break;
    case DEITY_ID_TheTribunal: ret = "The Tribunal"; break;
    case DEITY_ID_Tunare: ret = "Tunare"; break;
    case DEITY_ID_Veeshan: ret = "Veeshan"; break;
    default:
    case DEITY_ID_Agnostic: ret = "Agnostic"; break;
    }
    return ret;
}
