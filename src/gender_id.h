
#ifndef GENDER_ID_H
#define GENDER_ID_H

enum GenderId
{
    GENDER_ID_Male,
    GENDER_ID_Female,
    GENDER_ID_Neuter,
};

const char* gender_id_to_str(int genderId);

#endif/*GENDER_ID_H*/
