#include "Natives.h"

#include "amxxmodule.h"
#include "natives_handles.h"
#include "HLTypeConversion.h"

#include "Waypoint.h"
#include "Graph.h"
#include "Path.h"
#include "AStar.h"
#include "Steer.h"

NativeHandle<Path> g_pathHandler;

extern Graph g_graph;
extern HLTypeConversion g_typeConversion;
extern void ReleaseEdictMap(edict_t *pEntity);

static cell AMX_NATIVE_CALL wp_GetClosestPoint(AMX *amx, cell *params)
{
	cell *pAddr = MF_GetAmxAddr(amx, params[1]);

	Vector pos = Vector(amx_ctof(pAddr[0]), amx_ctof(pAddr[1]), amx_ctof(pAddr[2]));
	float radius = amx_ctof(params[2]);

	Waypoint *pPoint = g_graph.GetClosestPoint(pos, radius);
	if (pPoint == nullptr)
		return -1;

	return g_graph.Index(pPoint);
}

static cell AMX_NATIVE_CALL wp_GetClosestPath(AMX *amx, cell *params)
{
	cell *pAddr = MF_GetAmxAddr(amx, params[1]);

	Vector pos = Vector(amx_ctof(pAddr[0]), amx_ctof(pAddr[1]), amx_ctof(pAddr[2]));

	cell *pOut1 = MF_GetAmxAddr(amx, params[2]);
	cell *pOut2 = MF_GetAmxAddr(amx, params[3]);

	Waypoint *pBegin, *pEnd;
	Vector output = g_graph.GetClosestPath(pos, pBegin, pEnd);
	
	*pOut1 = g_graph.Index(pBegin);
	*pOut2 = g_graph.Index(pEnd);

	cell *pOutput = MF_GetAmxAddr(amx, params[4]);
	pOutput[0] = amx_ftoc(output.x);
	pOutput[1] = amx_ftoc(output.y);
	pOutput[2] = amx_ftoc(output.z);

	return 0;
}

static cell AMX_NATIVE_CALL wp_GetPos(AMX *amx, cell *params)
{
	int index = params[1];
	Waypoint *pPoint = g_graph.GetWaypointAt(index);
	if (pPoint == nullptr)
		return 0;

	Vector pos = pPoint->GetPos();

	cell *pResult = MF_GetAmxAddr(amx, params[2]);
	pResult[0] = amx_ftoc(pos.x);
	pResult[1] = amx_ftoc(pos.y);
	pResult[2] = amx_ftoc(pos.z);

	return 1;
}

static cell AMX_NATIVE_CALL wp_IsValid(AMX *amx, cell *params)
{
	if (g_graph.GetWaypointAt(params[1]) == nullptr)
		return 0;

	return 1;
}

static cell AMX_NATIVE_CALL wp_PathGet(AMX *amx, cell *params)
{
	Path *pPath = g_pathHandler.lookup(params[1]);
	if (pPath == nullptr)
		return -1;

	size_t index = params[2];

	auto &waypoints = pPath->GetWaypoints();
	if (index >= pPath->Size())
		return -1;

	Waypoint *pPoint = waypoints.at(index);
	return g_graph.Index(pPoint);
}

static cell AMX_NATIVE_CALL wp_PathSize(AMX *amx, cell *params)
{
	Path *pPath = g_pathHandler.lookup(params[1]);
	if (pPath == nullptr)
		return 0;

	return pPath->Size();
}

static cell AMX_NATIVE_CALL wp_PathCreate(AMX *amx, cell *params)
{
	return g_pathHandler.create();
}

static cell AMX_NATIVE_CALL wp_PathDelete(AMX *amx, cell *params)
{
	size_t index = params[1];

	Path *pPath = g_pathHandler.lookup(params[1]);
	if (pPath == nullptr)
		return 0;

	g_pathHandler.destroy(index);
	return 1;
}

static cell AMX_NATIVE_CALL wp_PathClear(AMX *amx, cell *params)
{
	Path *pPath = g_pathHandler.lookup(params[1]);
	if (pPath == nullptr)
		return 0;

	pPath->Clear();
	return 1;
}

static cell AMX_NATIVE_CALL wp_ReleaseEdictMap(AMX *amx, cell *params)
{
	edict_t *pEntity = g_typeConversion.id_to_edict(params[1]);
	if (pEntity == nullptr)
		return 0;

	ReleaseEdictMap(pEntity);
	return 1;
}

static cell AMX_NATIVE_CALL steerApplyForce(AMX *amx, cell *params)
{
	edict_t *pEntity = g_typeConversion.id_to_edict(params[1]);
	if (pEntity == nullptr)
		return 0;

	cell *pAddr = MF_GetAmxAddr(amx, params[2]);
	Vector force = Vector(amx_ctof(pAddr[0]), amx_ctof(pAddr[1]), amx_ctof(pAddr[2]));

	float maxForce = amx_ctof(params[3]);
	float mass = amx_ctof(params[4]);

	Steer::ApplyForce(pEntity, force, maxForce, mass);
	return 1;
}

static cell AMX_NATIVE_CALL steerSeek(AMX *amx, cell *params)
{
	edict_t *pEntity = g_typeConversion.id_to_edict(params[1]);
	if (pEntity == nullptr)
		return 0;

	cell *pAddr = MF_GetAmxAddr(amx, params[2]);
	Vector target = Vector(amx_ctof(pAddr[0]), amx_ctof(pAddr[1]), amx_ctof(pAddr[2]));

	Vector force = Steer::Seek(pEntity, target);

	cell *pResult = MF_GetAmxAddr(amx, params[3]);
	pResult[0] = amx_ftoc(force.x);
	pResult[1] = amx_ftoc(force.y);
	pResult[2] = amx_ftoc(force.z);

	return 1;
}

static cell AMX_NATIVE_CALL steerFollowPath(AMX *amx, cell *params)
{
	edict_t *pEntity = g_typeConversion.id_to_edict(params[1]);
	if (pEntity == nullptr)
		return 0;

	Path *pPath = g_pathHandler.lookup(params[5]);
	if (pPath == nullptr)
		return 0;

	float predictionTime = amx_ctof(params[2]);
	float radius = amx_ctof(params[3]);
	int maxPaths = params[4];

	Vector force = Steer::FollowPath(pEntity, predictionTime, radius, maxPaths, *pPath);

	cell *pResult = MF_GetAmxAddr(amx, params[6]);
	pResult[0] = amx_ftoc(force.x);
	pResult[1] = amx_ftoc(force.y);
	pResult[2] = amx_ftoc(force.z);

	return 1;
}

AMX_NATIVE_INFO Waypoint_Natives[] =
{
	{ "wp_GetClosestPoint", wp_GetClosestPoint },
	{ "wp_GetClosestPath", wp_GetClosestPath },
	{ "wp_GetPos", wp_GetPos },
	{ "wp_IsValid", wp_IsValid },
	{ "wp_PathCreate", wp_PathCreate },
	{ "wp_PathDelete", wp_PathDelete },
	{ "wp_PathGet", wp_PathGet },
	{ "wp_PathSize", wp_PathSize },
	{ "wp_PathClear", wp_PathClear },
	{ "wp_ReleaseEdictMap", wp_ReleaseEdictMap },
	{ "steerApplyForce", steerApplyForce },
	{ "steerSeek", steerSeek },
	{ "steerFollowPath", steerFollowPath },
	{ nullptr, nullptr }
};

void AddNatives()
{
	MF_AddNatives(Waypoint_Natives);
}