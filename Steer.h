#pragma once

#include "extdll.h"

class Path;

class Steer
{
public:
	static Vector Seek(edict_t *pEntity, const Vector &target);
	static Vector FollowPath(edict_t *pEntity, float predictionTime, float radius, int maxPaths, const Path &path);
	static Vector TruncateLength(const Vector &vec, float maxLength);
	static void ApplyForce(edict_t *pEntity, const Vector &force, float maxForce, float mass);

private:
	Steer() {}
};