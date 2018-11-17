#pragma once

#include "extdll.h"
#include "meta_api.h"

char* UTIL_VarArgs(const char *format, ...);

void UTIL_ClientPrintAll(int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);

void UTIL_BeamPoints(edict_t *pEntity, const Vector &pos1, const Vector &pos2, short sprite, int startFrame, int frameRate, int life, int width, int noise, int r, int g, int b, int brightness, int speed);

bool UTIL_IsVisible(const Vector &start, const Vector &end, edict_t *skipEnt, int noMonsters);

bool UTIL_IsWalkable(const Vector &start, const Vector &end, int hull, edict_t *skipEnt, int noMonsters);

float UTIL_DistLineSegments(const Vector &s1p0, const Vector &s1p1, const Vector &s2p0, const Vector &s2p1);

float UTIL_DistPointSegment(const Vector &p, const Vector &sp0, const Vector &sp1, Vector &out);

edict_t *UTIL_FindEntityByClassname(edict_t *pStartEntity, const char *szName);

edict_t *UTIL_FindEntityByString(edict_t *pentStart, const char *szKeyword, const char *szValue);

edict_t *UTIL_FindEntityInSphere(edict_t *pentStart, const Vector &vecCenter, float flRadius);

void UTIL_HudMessage(edict_t *pEntity, const hudtextparms_t &textparms, const char *pMessage);