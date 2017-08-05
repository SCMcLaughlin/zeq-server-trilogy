
#include "gender_id.h"

const char* gender_id_to_str(int genderId)
{
    const char* ret;
    switch (genderId)
    {
    case GENDER_ID_Male: ret = "Male"; break;
    case GENDER_ID_Female: ret = "Female"; break;
    default:
    case GENDER_ID_Neuter: ret = "Neuter"; break;
    }
    return ret;
}
