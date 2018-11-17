#include "Steer.h"
#include "meta_api.h"
#include "Path.h"
#include "Utilities.h"

extern short g_sprBeam1;

Vector Steer::Seek(edict_t *pEntity, const Vector &target)
{
	Vector offset = target - pEntity->v.origin;
	Vector desiredVelocity = TruncateLength(offset, pEntity->v.maxspeed);

	return desiredVelocity - pEntity->v.avelocity;
}

Vector Steer::TruncateLength(const Vector &vec, float maxLength)
{
	float maxLengthSquared = maxLength * maxLength;
	float vecLengthSquared = DotProduct(vec, vec);

	if (vecLengthSquared <= maxLengthSquared)
		return vec;

	return vec * (maxLength / sqrt(vecLengthSquared));
}

Vector Steer::FollowPath(edict_t *pEntity, float predictionTime, float radius, int maxPaths, const Path &path)
{
	float distance, minDist = 999999;

	Vector closestPos, targetPos;

	Vector vector;
	g_engfuncs.pfnAngleVectors(pEntity->v.angles, vector, Vector(), Vector());

	Vector futurePos = pEntity->v.origin + (vector * (pEntity->v.maxspeed * predictionTime));

	Waypoint *pPoint, *pPoint2;
	auto &waypoints = path.GetWaypoints();
	auto end = waypoints.begin() + min(path.Size(), (size_t)maxPaths) - 1;

	for (auto it = waypoints.begin(); it != end; ++it)
	{
		pPoint = *it;
		pPoint2 = *(it + 1);
		if (pPoint2 == nullptr)
			break;

		distance = UTIL_DistPointSegment(futurePos, pPoint->GetPos(), pPoint2->GetPos(), closestPos);

		if (distance <= radius)
		{
			return Seek(pEntity, pEntity->v.origin + (vector * pEntity->v.maxspeed));
		}

		if (distance < minDist)
		{
			minDist = distance;

			if ((closestPos - pPoint2->GetPos()).Length() >= (pEntity->v.maxspeed * predictionTime))
			{
				targetPos = pPoint2->GetPos();
			}
			else
			{
				targetPos = closestPos + ((pPoint2->GetPos() - pPoint->GetPos()).Normalize() * (pEntity->v.maxspeed * predictionTime));
			}
		}
	}

	if (minDist == 999999)
	{
		return Vector(0, 0, 0);
	}

	UTIL_BeamPoints(pEntity->v.enemy, Vector(targetPos), Vector(targetPos.x, targetPos.y, targetPos.z - 36), g_sprBeam1, 0, 0, 30, 10, 0, 0, 100, 255, 255, 0);
	return Seek(pEntity, targetPos);
}

void Steer::ApplyForce(edict_t *pEntity, const Vector &force, float maxForce, float mass)
{
	Vector clippedForce = TruncateLength(force, maxForce);
	Vector newAcceleration = clippedForce / mass;

	Vector newVelocity = pEntity->v.avelocity;
	newVelocity = newVelocity + newAcceleration;
	newVelocity = TruncateLength(newVelocity, pEntity->v.maxspeed);
	newVelocity.z = pEntity->v.velocity.z;

	pEntity->v.velocity = newVelocity;
	pEntity->v.avelocity = newVelocity;

	Vector angle;
	VEC_TO_ANGLES(newVelocity, angle);
	angle.x = 0;
	pEntity->v.angles = angle;

	MOVE_TO_ORIGIN(pEntity, pEntity->v.origin + newVelocity, 0.5, 1);
}