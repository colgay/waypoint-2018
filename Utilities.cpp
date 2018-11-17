#include "Utilities.h"

#include "extdll.h"

int gmsgTextMsg = 0;


char* UTIL_VarArgs(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

void UTIL_ClientPrintAll(int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4)
{
	if (gmsgTextMsg == 0)
	{
		gmsgTextMsg = GET_USER_MSG_ID(PLID, "TextMsg", NULL);

		if (!gmsgTextMsg)
			return;
	}

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgTextMsg);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void UTIL_BeamPoints(edict_t *pEntity, const Vector &pos1, const Vector &pos2, short sprite, int startFrame, int frameRate, int life, int width, int noise, int r, int g, int b, int brightness, int speed)
{
	MESSAGE_BEGIN(pEntity == nullptr ? MSG_BROADCAST : MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pos1.x);
	WRITE_COORD(pos1.y);
	WRITE_COORD(pos1.z);
	WRITE_COORD(pos2.x);
	WRITE_COORD(pos2.y);
	WRITE_COORD(pos2.z);
	WRITE_SHORT(sprite);
	WRITE_BYTE(startFrame);		// startframe
	WRITE_BYTE(frameRate);		// framerate
	WRITE_BYTE(life);		// life
	WRITE_BYTE(width);		// width
	WRITE_BYTE(noise);		// noise
	WRITE_BYTE(r);	// r
	WRITE_BYTE(g);		// g
	WRITE_BYTE(b);		// b
	WRITE_BYTE(brightness);	// brightness
	WRITE_BYTE(speed);		// speed
	MESSAGE_END();
}

bool UTIL_IsVisible(const Vector &start, const Vector &end, edict_t *skipEnt, int noMonsters)
{
	TraceResult tr;
	TRACE_LINE(start, end, noMonsters, skipEnt, &tr);

	if (tr.flFraction == 1.0)
		return true;

	return false;
}

bool UTIL_IsWalkable(const Vector &start, const Vector &end, int hull, edict_t *skipEnt, int noMonsters)
{
	TraceResult tr;
	TRACE_HULL(start, end, noMonsters, hull, skipEnt, &tr);

	if (tr.flFraction == 1.0)
		return true;

	return false;
}

float UTIL_DistPointSegment(const Vector &p, const Vector &sp0, const Vector &sp1, Vector &out)
{
	Vector v = sp1 - sp0;
	Vector w = p - sp0;

	float c1 = DotProduct(w, v);
	if (c1 <= 0)
	{
		out = sp0;
		return (p - sp0).Length();
	}

	float c2 = DotProduct(v, v);
	if (c2 <= c1)
	{
		out = sp1;
		return (p - sp1).Length();
	}

	float b = c1 / c2;
	out = sp0 + b * v;

	return (p - out).Length();
}

float UTIL_DistLineSegments(const Vector &s1p0, const Vector &s1p1, const Vector &s2p0, const Vector &s2p1)
{
	static const float SMALL_NUM = 0.00000001;

	Vector u = s1p1 - s1p0;
	Vector v = s2p1 - s2p0;
	Vector w = s1p0 - s2p0;

	float a = DotProduct(u, u); // always >= 0
	float b = DotProduct(u, v);
	float c = DotProduct(v, v); // always >= 0
	float d = DotProduct(u, w);
	float e = DotProduct(v, w);
	float D = a*c - b*b; // always >= 0
	float sc, sN, sD = D; // sc = sN / sD, default sD = D >= 0
	float tc, tN, tD = D; // tc = tN / tD, default tD = D >= 0

	// compute the line parameters of the two closest points
	if (D < SMALL_NUM) // the lines are almost parallel
	{
		sN = 0.0; // force using point P0 on segment S1
		sD = 1.0; // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	}
	else // get the closest points on the infinite lines
	{
		sN = (b*e - c*d);
		tN = (a*e - b*d);
		if (sN < 0.0) // sc < 0 => the s=0 edge is visible
		{
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD) // sc > 1  => the s=1 edge is visible
		{
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0) // tc < 0 => the t=0 edge is visible
	{
		tN = 0.0;
		// recompute sc for this edge
		if (-d < 0.0)
			sN = 0.0;
		else if (-d > a)
			sN = sD;
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD) // tc > 1  => the t=1 edge is visible
	{
		tN = tD;
		// recompute sc for this edge
		if ((-d + b) < 0.0)
			sN = 0;
		else if ((-d + b) > a)
			sN = sD;
		else
		{
			sN = (-d + b);
			sD = a;
		}
	}

	// finally do the division to get sc and tc
	sc = (fabs(sN) < SMALL_NUM ? 0.0 : sN / sD);
	tc = (fabs(tN) < SMALL_NUM ? 0.0 : tN / tD);

	Vector dP = w + (sc * u) - (tc * v);  // =  S1(sc) - S2(tc)
	return dP.Length();
}

edict_t *UTIL_FindEntityByClassname(edict_t *pStartEntity, const char *szName)
{
	return UTIL_FindEntityByString(pStartEntity, "classname", szName);
}

edict_t *UTIL_FindEntityByString(edict_t *pentStart, const char *szKeyword, const char *szValue)
{
	edict_t	*pentEntity;

	pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return pentEntity;

	return NULL;
}

edict_t *UTIL_FindEntityInSphere(edict_t *pentStart, const Vector &vecCenter, float flRadius)
{
	edict_t  *pentEntity;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return pentEntity;

	return NULL;
}

void UTIL_HudMessage(edict_t *pEntity, const hudtextparms_t &textparms, const char *pMessage)
{
	if (!pEntity)
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(textparms.channel & 0xFF);

	WRITE_SHORT(FixedSigned16(textparms.x, 1 << 13));
	WRITE_SHORT(FixedSigned16(textparms.y, 1 << 13));
	WRITE_BYTE(textparms.effect);

	WRITE_BYTE(textparms.r1);
	WRITE_BYTE(textparms.g1);
	WRITE_BYTE(textparms.b1);
	WRITE_BYTE(textparms.a1);

	WRITE_BYTE(textparms.r2);
	WRITE_BYTE(textparms.g2);
	WRITE_BYTE(textparms.b2);
	WRITE_BYTE(textparms.a2);

	WRITE_SHORT(FixedUnsigned16(textparms.fadeinTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.fadeoutTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.holdTime, 1 << 8));

	if (textparms.effect == 2)
		WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1 << 8));

	if (strlen(pMessage) < 512)
	{
		WRITE_STRING(pMessage);
	}
	else
	{
		char tmp[512];
		strncpy(tmp, pMessage, 511);
		tmp[511] = 0;
		WRITE_STRING(tmp);
	}
	MESSAGE_END();
}
