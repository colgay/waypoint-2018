#include "amxxmodule.h"
#include <amtl/am-threadlocal.h>
#include <amtl/am-autoptr.h>
#include <amtl/am-hashmap.h>
#include "Waypoint.h"
#include "Graph.h"
#include "AStar.h"
#include "Editor.h"
#include "Utilities.h"
#include "Natives.h"
#include "Steer.h"
#include "Path.h"
#include "HLTypeConversion.h"

AStar g_pathfinder;
Graph g_graph;
Editor g_editor;
HLTypeConversion g_typeConversion;

typedef ke::HashMap<edict_t *, Path *, ke::PointerPolicy<edict_t>> EdictMap;
EdictMap g_edictmap;

float g_updateTime;
short g_sprBeam1, g_sprBeam4, g_sprArrow;
char g_pszFilePath[256];

void ReleaseWaypoints(Path *pPath)
{
	auto &waypoints = pPath->GetWaypoints();
	for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
	{
		delete *it;
	}
}

void ReleaseEdictMap(edict_t *pent)
{
	EdictMap::Result r = g_edictmap.find(pent);
	if (r.found())
	{
		int index = ENTINDEX(pent);
		if (index > 0 && index <= gpGlobals->maxClients)
			ReleaseWaypoints(r->value);

		delete r->value;
		g_edictmap.remove(r);
	}
}

void OnAmxxAttach()
{
	AddNatives();
}

void OnPluginsLoaded()
{
	g_sprBeam1 = PRECACHE_MODEL("sprites/zbeam1.spr");
	g_sprBeam4 = PRECACHE_MODEL("sprites/zbeam4.spr");
	g_sprArrow = PRECACHE_MODEL("sprites/arrow1.spr");

	MF_BuildPathnameR(g_pszFilePath, sizeof(g_pszFilePath), "%s/waypoints/%s.txt", MF_GetLocalInfo("amxx_configsdir", "addons/amxmodx/configs"), STRING(gpGlobals->mapname));

	g_editor.SetGraph(&g_graph);
	g_editor.Load(g_pszFilePath);

	g_pathfinder.Init(&g_graph);

	g_typeConversion.init();
	g_edictmap.init();
}

void ServerDeactivate()
{
	g_pathfinder.Clear(true);
	g_graph.Clear();
	g_editor.Reset();

	int index;
	edict_t *pEnt;

	for (EdictMap::iterator iter = g_edictmap.iter(); !iter.empty(); iter.next())
	{
		pEnt = iter->key;
		index = ENTINDEX(pEnt);
		if (index > 0 && index <= gpGlobals->maxClients)
		{
			ReleaseWaypoints(iter->value); // *pWaypoint
		}

		delete iter->value; // *pPath
	}

	g_edictmap.clear();
}

void ClientDisconnect(edict_t *pEntity)
{
	if (g_editor.GetEditor() == pEntity)
		g_editor.SetEditor(nullptr);

	ReleaseEdictMap(pEntity);
}

void ClientPutInServer(edict_t *pEntity)
{
	EdictMap::Insert i = g_edictmap.findForAdd(pEntity);
	if (!i.found())
	{
		Path *pPath = new Path();
		g_edictmap.add(i, pEntity, pPath);
	}
}

void ClientCommand(edict_t *pEntity)
{
	const char *pszCmd;
	pszCmd = CMD_ARGV(0);

	if (strcmp(pszCmd, "follow") == 0)
	{
		edict_t *pEnt = NULL;
		while ((pEnt = UTIL_FindEntityByClassname(pEnt, "monster_test")))
		{
			if (FNullEnt(pEnt))
				continue;

			pEnt->v.enemy = pEntity;
		}

		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "unfollow") == 0)
	{
		edict_t *pEnt = NULL;
		while ((pEnt = UTIL_FindEntityByClassname(pEnt, "monster_test")))
		{
			if (FNullEnt(pEnt))
				continue;

			pEnt->v.enemy = NULL;
		}

		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "monster") == 0)
	{
		edict_t *pEnt = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
		if (pEnt != NULL)
		{
			Vector origin = pEntity->v.origin;
			origin.z += 80;

			SET_MODEL(pEnt, "models/player/vip/vip.mdl");
			SET_SIZE(pEnt, Vector(-16, -16, -36), Vector(16, 16, 36));
			SET_ORIGIN(pEnt, origin);

			pEnt->v.classname = MAKE_STRING("monster_test");
			pEnt->v.takedamage = DAMAGE_YES;
			pEnt->v.health = 200;
			pEnt->v.solid = SOLID_SLIDEBOX;
			pEnt->v.movetype = MOVETYPE_STEP;
			pEnt->v.maxspeed = 250;
			pEnt->v.gamestate = 1;

			pEnt->v.sequence = 4;
			pEnt->v.animtime = gpGlobals->time;
			pEnt->v.framerate = 1.0;

			pEnt->v.nextthink = gpGlobals->time + 0.1;

			EdictMap::Insert i = g_edictmap.findForAdd(pEnt);
			if (!i.found())
			{
				Path *pPath = new Path();
				g_edictmap.add(i, pEnt, pPath);
			}
		}

		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "wp") == 0)
	{
		pszCmd = CMD_ARGV(1);

		if (strcmp(pszCmd, "editor") == 0)
		{
			const char *pszArg = CMD_ARGV(2);

			if (strcmp(pszArg, "1") == 0)
			{
				g_editor.SetEditor(pEntity);
			}
			else if (strcmp(pszArg, "0") == 0)
			{
				g_editor.SetEditor(nullptr);
			}

			RETURN_META(MRES_SUPERCEDE);
		}

		if (g_editor.GetEditor() == nullptr)
		{
			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "create") == 0)
		{
			g_editor.Create(0);
			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "remove") == 0)
		{
			const char *pszArg = CMD_ARGV(2);
			if (strcmp(pszArg, "all") == 0)
			{
				g_editor.Remove(nullptr, true);
			}
			else
			{
				Waypoint *pPoint = g_editor.ArgPoint(pszArg);
				g_editor.Remove(pPoint);
			}

			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "conn") == 0)
		{
			const char *pszArg1 = CMD_ARGV(2);
			const char *pszArg2 = CMD_ARGV(3);
			const char *pszArg3 = CMD_ARGV(4);

			Waypoint *pPoint1 = g_editor.ArgPoint(pszArg1);
			Waypoint *pPoint2 = g_editor.ArgPoint(pszArg2);
			int type = pszArg3[0] ? atoi(pszArg3) : 0;

			g_editor.Connect(pPoint1, pPoint2, type);
			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "disconn") == 0)
		{
			const char *pszArg1 = CMD_ARGV(2);
			const char *pszArg2 = CMD_ARGV(3);

			Waypoint *pPoint1 = g_editor.ArgPoint(pszArg1);
			Waypoint *pPoint2 = g_editor.ArgPoint(pszArg2);

			g_editor.Disconnect(pPoint1, pPoint2);
			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "autowp") == 0)
		{
			const char *pszArg1 = CMD_ARGV(2);
			const char *pszArg2 = CMD_ARGV(3);

			if (strcmp(pszArg1, "dist") == 0)
			{
				g_editor.SetAutoWpDist(atof(pszArg2));
			}
			else
			{
				if (strcmp(pszArg1, "1") == 0)
					g_editor.SetAutoWp(true);
				else if (strcmp(pszArg1, "0") == 0)
					g_editor.SetAutoWp(false);
			}

			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "autoconn") == 0)
		{
			const char *pszArg1 = CMD_ARGV(2);
			const char *pszArg2 = CMD_ARGV(3);

			if (strcmp(pszArg1, "dist") == 0)
			{
				g_editor.SetAutoConnDist(atof(pszArg2));
			}
			else
			{
				if (strcmp(pszArg1, "1") == 0)
					g_editor.SetAutoConn(true);
				else if (strcmp(pszArg1, "0") == 0)
					g_editor.SetAutoConn(false);
			}

			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "path") == 0)
		{
			const char *pszArg1 = CMD_ARGV(2);
			const char *pszArg2 = CMD_ARGV(3);

			if (strcmp(pszArg1, "start") == 0 || strcmp(pszArg1, "begin") == 0)
			{
				g_editor.SetStart(g_editor.m_pCurrent);
			}
			else if (strcmp(pszArg1, "goal") == 0 || strcmp(pszArg1, "end") == 0)
			{
				g_editor.SetGoal(g_editor.m_pCurrent);
			}
			else if (strcmp(pszArg1, "find") == 0 || strcmp(pszArg1, "run") == 0 || strcmp(pszArg1, "go") == 0)
			{
				g_editor.PathFinder(&g_pathfinder);
			}

			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "save") == 0)
		{
			g_editor.Save(g_pszFilePath);
			RETURN_META(MRES_SUPERCEDE);
		}

		if (strcmp(pszCmd, "load") == 0)
		{
			g_editor.Load(g_pszFilePath);
			RETURN_META(MRES_SUPERCEDE);
		}
	}

	RETURN_META(MRES_IGNORED);
}

void PlayerPreThink(edict_t *pEntity)
{
	if (gpGlobals->time >= g_updateTime + 0.05)
	{
		if (pEntity->v.deadflag != DEAD_NO || !(pEntity->v.flags & FL_ONGROUND))
			RETURN_META(MRES_IGNORED);

		EdictMap::Result r = g_edictmap.find(pEntity);
		if (r.found())
		{
			Path *pPath = r->value;
			if (pPath == nullptr)
				RETURN_META(MRES_IGNORED);

			if (pPath->Size() >= 10)
			{
				delete pPath->At(0);
				pPath->Pop();
			}

			Waypoint *pPoint;
			if (pPath->Size() > 0)
			{
				pPoint = pPath->Back();
				if ((pPoint->GetPos() - pEntity->v.origin).Length() >= 50)
				{
					pPoint = new Waypoint(pEntity->v.origin);
					pPath->PushBack(pPoint);
				}
			}
			else
			{
				pPoint = new Waypoint(pEntity->v.origin);
				pPath->PushBack(pPoint);
			}

			Vector pos;
			auto &waypoints = pPath->GetWaypoints();
			
			for (auto iter = waypoints.begin(); iter != waypoints.end(); ++iter)
			{
				pos = (*iter)->GetPos();
				UTIL_BeamPoints(pEntity, Vector(pos.x, pos.y, pos.z - 36), pos, g_sprBeam1, 0, 0, 1, 10, 0, 255, 0, 0, 255, 0);
			}
		}

		g_updateTime = gpGlobals->time;
	}

	g_editor.Draw(pEntity);
	RETURN_META(MRES_IGNORED);
}

void DispatchThink(edict_t *pent)
{
	if (strcmp(STRING(pent->v.classname), "monster_test") != 0)
		RETURN_META(MRES_IGNORED);

	edict_t *pEnemy = pent->v.enemy;
	if (!FNullEnt(pEnemy) && pEnemy->v.deadflag == DEAD_NO)
	{
		Vector origin = pent->v.origin;
		Vector target = pEnemy->v.origin;
		Vector steering;

		EdictMap::Result r = g_edictmap.find(pent);

		if (r.found())
		{
			Path *pPath = r->value;

			if (gpGlobals->time >= pent->v.frags + 0.5)
			{
				Waypoint *pBegin, *pEnd;
				if (pPath->Size() > 1)
				{
					pPath->GetClosestPath(origin, 1000, pBegin, pEnd);
				}
				else
				{
					pBegin = g_graph.GetClosestPoint(origin, 1000);
				}

				pEnd = g_graph.GetClosestPoint(target, 1000);

				pPath->Clear();

				if (pBegin != nullptr && pEnd != nullptr)
				{
					g_pathfinder.CalcPath(pBegin, pEnd, *pPath);
					g_pathfinder.Clear();
				}

				pent->v.frags = gpGlobals->time;
			}

			bool hasFollowed = false;
			EdictMap::Result r = g_edictmap.find(pEnemy);

			if (r.found())
			{
				Path *pPath2 = r->value;
				if (pPath2->Size() > 1)
				{
					Waypoint *pEnd;
					Vector pos = pPath2->GetClosestPath(origin, 1000, pEnd, pEnd);
					if ((origin - pos).Length() <= 100)
					{
						if (pEnd == pPath2->Back() || UTIL_IsWalkable(origin, target, head_hull, pent, ignore_monsters))
						{
							steering = Steer::Seek(pent, target);
							hasFollowed = true;
						}
						else
						{
							steering = Steer::FollowPath(pent, 0.3, 30, 10, *pPath2);
							hasFollowed = true;
						}
					}
				}
			}

			if (pPath->Size() > 1 && !hasFollowed)
			{
				steering = Steer::FollowPath(pent, 0.3, 30, 10, *pPath);
			}
		}

		Steer::ApplyForce(pent, steering, 40, 1);

		if (pent->v.flags & FL_ONGROUND)
		{
			Vector vec = pent->v.angles;
			vec.x = 0;
			g_engfuncs.pfnAngleVectors(vec, vec, nullptr, nullptr);
			
			Vector pos = origin + (vec * 20.0);
			pos.z = pos.z + 12;

			TraceResult tr;
			TRACE_LINE(pos, Vector(pos.x, pos.y, pos.z - 48), dont_ignore_monsters, pent, &tr);

			if (!tr.fStartSolid && tr.flFraction < 1.0 && tr.flFraction < 0.66666)
			{
				pent->v.velocity.z += 250;
			}
		}
	}

	pent->v.nextthink = gpGlobals->time + 0.1;
	RETURN_META(MRES_IGNORED);
}

void StartFrame_Post()
{
	RETURN_META(MRES_IGNORED);
}