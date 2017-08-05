
#ifndef LOC_H
#define LOC_H

typedef struct {
    float x, y, z;
} Loc;

typedef struct {
    float x, y, z, heading;
} LocH;

typedef struct {
    int     zoneId;
    LocH    loc;
} BindPoint;

#endif/*LOC_H*/
