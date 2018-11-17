#include "Editor.h"
#include "amtl/am-string.h"
#include "meta_api.h"
#include "Waypoint.h"
#include "Graph.h"
#include "Path.h"
#include "AStar.h"
#include "Utilities.h"

extern short g_sprBeam1;
extern short g_sprBeam4;
extern short g_sprArrow;

void Editor::Reset()
{
	m_pEditor = nullptr;
	m_pGraph = nullptr;
	m_pAim = nullptr;
	m_pCurrent = nullptr;
	m_time = 0.0;
	m_auto = false;
	m_conn = true;
	m_autoDist = 120;
	m_connDist = 180;
	m_pStart = nullptr;
	m_pGoal = nullptr;
}

edict_t *Editor::GetEditor() const
{
	return m_pEditor;
}

void Editor::SetEditor(edict_t *pEditor)
{
	m_pEditor = pEditor;

	if (pEditor == nullptr)
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Waypoint Editor: Off\n");
	}
	else
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Waypoint Editor: On (%s)\n", STRING(pEditor->v.netname)));
	}
}

Waypoint *Editor::ArgPoint(const char *pszArg) const
{
	if (strcmp(pszArg, "curr") == 0)
		return m_pCurrent;

	if (strcmp(pszArg, "aim") == 0)
		return m_pAim;

	return m_pGraph->GetWaypointAt(atoi(pszArg));
}

void Editor::SetGraph(Graph *pGraph)
{
	m_pGraph = pGraph;
}

void Editor::Create(float radius) const
{
	Waypoint *pPoint = m_pGraph->CreateWaypoint(m_pEditor->v.origin, radius, 0);

	if (pPoint == nullptr)
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Unable to create waypoint.\n");
	else
	{
		ConnectPoints(pPoint);
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Create Waypoint #%d\n", m_pGraph->Index(pPoint)));
	}
}

void Editor::Remove(Waypoint *pPoint, bool all)
{
	if (pPoint == nullptr && !all)
		return;

	if (all)
	{
		m_pGraph->Clear();
		UTIL_ClientPrintAll(HUD_PRINTTALK, "[WP] All waypoints on this map has been removed.\n");
	}
	else
	{
		int index = m_pGraph->Index(pPoint);
		m_pGraph->RemoveWaypoint(pPoint);

		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Remove Waypoint #%d.\n", index));
	}
}

void Editor::Connect(Waypoint *pPoint1, Waypoint *pPoint2, int type) const
{
	if (pPoint1 == nullptr || pPoint2 == nullptr || pPoint1 == pPoint2)
		return;

	switch (type)
	{
	case 1: // Out-going
		pPoint2->RemoveChild(pPoint1);
		pPoint1->AddChild(pPoint2);
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Connect waypoint #%d --> #%d. (Out-going)\n", m_pGraph->Index(pPoint1), m_pGraph->Index(pPoint2)));
		break;
	case 2: // In-coming
		pPoint1->RemoveChild(pPoint2);
		pPoint2->AddChild(pPoint1);
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Connect waypoint #%d <-- #%d. (In-coming)\n", m_pGraph->Index(pPoint1), m_pGraph->Index(pPoint2)));
		break;
	default: // Both ways
		pPoint1->AddChild(pPoint2);
		pPoint2->AddChild(pPoint1);
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Connect waypoints #%d <--> #%d. (Both ways)\n", m_pGraph->Index(pPoint1), m_pGraph->Index(pPoint2)));
		break;
	}
}

void Editor::Disconnect(Waypoint *pPoint1, Waypoint *pPoint2) const
{
	if (pPoint1 == nullptr || pPoint2 == nullptr || pPoint1 == pPoint2)
		return;

	pPoint1->RemoveChild(pPoint2);
	pPoint2->RemoveChild(pPoint1);

	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Disconnect waypoints #%d -- #%d. (Both ways)\n", m_pGraph->Index(pPoint1), m_pGraph->Index(pPoint2)));
}

Waypoint *Editor::GetAim(edict_t *pEntity, float maxDist, PointSet &hashset) const
{
	Vector start = pEntity->v.origin + pEntity->v.view_ofs;
	Vector end, pos;

	MAKE_VECTORS(pEntity->v.v_angle);
	end = start + (gpGlobals->v_forward * 4096);


	Waypoint *pPoint, *pResult = nullptr;
	float dist, minDist = maxDist;

	auto &waypoints = m_pGraph->GetWaypoints();

	for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
	{
		pPoint = *it;

		PointSet::Result r = hashset.find(pPoint);
		if (!r.found()) // not selected
			continue;

		pos = pPoint->GetPos();

		dist = UTIL_DistLineSegments(start, end, Vector(pos.x, pos.y, pos.z - 36), Vector(pos.x, pos.y, pos.z + 36));
		if (dist < minDist)
		{
			minDist = dist;
			pResult = pPoint;
		}
	}

	return pResult;
}

void Editor::ConnectPoints(Waypoint *pPoint) const
{
	if (!m_conn)
		return;

	auto &waypoints = m_pGraph->GetWaypoints();

	for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
	{
		if ((pPoint->GetPos() - (*it)->GetPos()).Length() <= m_connDist && m_connDist > 0)
		{
			if (UTIL_IsVisible(pPoint->GetPos(), (*it)->GetPos(), nullptr, ignore_monsters))
			{
				pPoint->AddChild(*it);
				(*it)->AddChild(pPoint);
			}
		}
	}
}

void Editor::Draw(edict_t *pEntity)
{
	if (pEntity == m_pEditor && pEntity->v.deadflag == DEAD_NO)
	{
		if (m_auto && (pEntity->v.flags & FL_ONGROUND))
		{
			Waypoint *point = m_pGraph->GetClosestPoint(pEntity->v.origin, m_autoDist);
			if (point == nullptr)
			{
				point = m_pGraph->CreateWaypoint(pEntity->v.origin, 0, 0);
				if (point != NULL)
					ConnectPoints(point);
			}
		}

		if (gpGlobals->time < m_time + 0.5)
			return;

		auto &waypoints = m_pGraph->GetWaypoints();
		if (waypoints.empty())
			return;

		m_time = gpGlobals->time;

		Vector origin = pEntity->v.origin;

		PointSet hashset;
		hashset.init();

		Waypoint *pPoint = nullptr;
		float dist, minDist;
		Vector pos;
		int size = min(waypoints.length(), 45);

		m_pCurrent = nullptr;

		for (int i = 0; i < size; i++)
		{
			pPoint = waypoints.at(0);
			minDist = 9999999;

			for (auto iter = waypoints.begin(); iter != waypoints.end(); ++iter)
			{
				PointSet::Result r = hashset.find(*iter);
				if (r.found()) // selected
					continue;

				dist = ((*iter)->GetPos() - origin).Length();
				if (dist < minDist)
				{
					minDist = dist;
					pPoint = *iter;
				}
			}

			PointSet::Insert p = hashset.findForAdd(pPoint);
			if (!p.found())
				hashset.add(p, pPoint);

			pos = pPoint->GetPos();

			if (m_pCurrent == nullptr && minDist <= 64)
			{
				m_pCurrent = pPoint;

				auto &children = pPoint->GetChildren();

				// Show out-going connections
				for (auto iter = children.begin(); iter != children.end(); ++iter)
				{
					Waypoint *pChild = *iter;

					if (pChild->GetChild(pPoint))
					{
						UTIL_BeamPoints(pEntity,
							Vector(pos.x, pos.y, pos.z),
							Vector(pChild->GetPos().x, pChild->GetPos().y, pChild->GetPos().z),
							g_sprBeam1, 0, 0,
							5, 10, 3,
							200, 200, 0, 255,
							0);
					}
					else
					{
						UTIL_BeamPoints(pEntity,
							Vector(pos.x, pos.y, pos.z),
							Vector(pChild->GetPos().x, pChild->GetPos().y, pChild->GetPos().z),
							g_sprBeam1, 0, 0,
							5, 10, 3,
							200, 100, 0, 255,
							0);
					}
				}

				// Show in-coming connections
				for (auto iter = waypoints.begin(); iter != waypoints.end(); ++iter)
				{
					auto pChild = *iter;

					if (pChild->GetChild(pPoint) && !pPoint->GetChild(pChild))
					{
						UTIL_BeamPoints(pEntity,
							Vector(pos.x, pos.y, pos.z),
							Vector(pChild->GetPos().x, pChild->GetPos().y, pChild->GetPos().z),
							g_sprBeam1, 0, 0,
							5, 10, 4,
							200, 0, 0, 255,
							0);
					}
				}

				UTIL_BeamPoints(pEntity,
					Vector(pos.x, pos.y, pos.z - 36),
					Vector(pos.x, pos.y, pos.z + 36),
					g_sprBeam4, 0, 0,
					5, 10, 0,
					255, 0, 0, 255,
					0);
			}
			else
			{
				UTIL_BeamPoints(pEntity,
					Vector(pos.x, pos.y, pos.z - 36),
					Vector(pos.x, pos.y, pos.z + 36),
					g_sprBeam4, 0, 0,
					5, 10, 0,
					0, 255, 0, 255,
					0);
			}
		}

		if (m_pCurrent != nullptr)
			hashset.removeIfExists(m_pCurrent);

		m_pAim = GetAim(pEntity, 32, hashset);

		if (m_pAim != nullptr)
		{
			UTIL_BeamPoints(pEntity,
				origin,
				m_pAim->GetPos(),
				g_sprArrow, 0, 0,
				5, 10, 0,
				255, 255, 255, 255,
				0);
		}

		hudtextparms_t textparms;
		textparms.a1 = 0;
		textparms.a2 = 0;
		textparms.r2 = 255;
		textparms.g2 = 255;
		textparms.b2 = 250;
		textparms.r1 = 0;
		textparms.g1 = 255;
		textparms.b1 = 0;
		textparms.x = 0.35;
		textparms.y = 0.25;
		textparms.effect = 0;
		textparms.fxTime = 0.0;
		textparms.holdTime = 0.6;
		textparms.fadeinTime = 0.0;
		textparms.fadeoutTime = 0.5;
		textparms.channel = 4;

		UTIL_HudMessage(pEntity, textparms, UTIL_VarArgs("Current Waypoint #%d", m_pCurrent));
	}
}

void Editor::SetAutoWpDist(float dist)
{
	m_autoDist = dist;
	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Auto waypoint distance changed to %.2f\n", dist));
}

void Editor::SetAutoConnDist(float dist)
{
	m_connDist = dist;
	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Auto connection distance changed to %.2f\n", dist));
}

void Editor::SetAutoWp(bool toggle)
{
	m_auto = toggle;
	if (m_auto)
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Auto waypoint: On\n");
	else
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Auto waypoint: Off\n");
}

void Editor::SetAutoConn(bool toggle)
{
	m_conn = toggle;
	if (m_conn)
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Auto connection: On\n");
	else
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Auto connection: Off\n");
}

void Editor::Save(const char *pszPath)
{
	FILE *pFile = fopen(pszPath, "w");
	if (pFile != NULL)
	{
		Vector pos;
		Waypoint *pPoint;
		auto &waypoints = m_pGraph->GetWaypoints();
		for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
		{
			pPoint = *it;
			pos = pPoint->GetPos();

			fprintf(pFile, "%f %f %f %f %d \"", pos.x, pos.y, pos.z, pPoint->GetRadius(), pPoint->GetFlags());

			auto &children = pPoint->GetChildren();
			for (auto iter = children.begin(); iter != children.end(); ++iter)
			{
				if (iter != children.begin())
					fputs(",", pFile);

				fprintf(pFile, "%d", m_pGraph->Index(*iter));
			}
			
			fputs("\"\n", pFile);
		}

		fclose(pFile);

		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] All %d waypoints has been saved.\n", m_pGraph->Size()));
	}
}

void Editor::Load(const char *pszPath)
{
	m_pGraph->Clear();

	FILE *pFile = fopen(pszPath, "r");
	if (pFile != NULL)
	{
		SERVER_PRINT(UTIL_VarArgs("joe is %s\n", pszPath));

		ke::Vector<ke::AString> neighbors;
		Waypoint *pPoint, *pChild;
		Vector pos;
		float radius;
		int flags, index;

		char szLine[512];
		char *pszChar;

		while (fgets(szLine, sizeof(szLine), pFile))
		{
			if (!szLine[0])
				continue;

			pszChar = strtok(szLine, " ");
			for (int i = 0; i < 3; i++)
			{
				pos[i] = atof(pszChar);
				pszChar = strtok(NULL, " ");
			}

			radius = atof(pszChar);
			pszChar = strtok(NULL, " ");

			flags = atof(pszChar);
			pszChar = strtok(NULL, " ");

			pPoint = m_pGraph->CreateWaypoint(pos, radius, flags);
			if (pPoint == nullptr)
				break;

			if (strlen(pszChar) >= 2)
			{
				pszChar[strlen(pszChar) - 2] = 0;
				pszChar++;

				neighbors.append(ke::AString(pszChar));
			}
			else
			{
				neighbors.append(ke::AString(""));
			}
		}

		for (size_t i = 0; i < neighbors.length(); i++)
		{
			pPoint = m_pGraph->GetWaypointAt(i);
			if (pPoint == nullptr)
				break;

			pszChar = const_cast<char *>(neighbors[i].chars());
			pszChar = strtok(pszChar, ",");
			while (pszChar != NULL)
			{
				index = atoi(pszChar);

				pChild = m_pGraph->GetWaypointAt(index);
				if (pChild != nullptr)
					pPoint->AddChild(pChild);

				pszChar = strtok(NULL, ",");
			}
		}

		fclose(pFile);
		
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Loaded %d waypoints.\n", m_pGraph->Size()));
	}
}

void Editor::SetStart(Waypoint *pPoint)
{
	m_pStart = pPoint;
	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Pathfinder: Set Start (#%d)\n", m_pGraph->Index(m_pStart)));
}

void Editor::SetGoal(Waypoint *pPoint)
{
	m_pGoal = pPoint;
	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Pathfinder: Set Goal (#%d)\n", m_pGraph->Index(m_pGoal)));
}

void Editor::PathFinder(AStar *pFinder)
{
	if (m_pStart == nullptr || m_pGoal == nullptr)
		RETURN_META(MRES_SUPERCEDE);

	Path path;
	if (pFinder->CalcPath(m_pStart, m_pGoal, path))
	{
		auto &waypoints = path.GetWaypoints();

		if (waypoints.length() >= 2)
		{
			for (auto it = waypoints.begin(); it != (waypoints.end() - 1); ++it)
			{
				Waypoint *pPoint = *it;
				Waypoint *pPoint2 = *(it + 1);

				SERVER_PRINT(UTIL_VarArgs("(#%d #%d), ", m_pGraph->Index(pPoint), m_pGraph->Index(pPoint2)));

				UTIL_BeamPoints(m_pEditor,
					pPoint->GetPos(), pPoint2->GetPos(),
					g_sprBeam1, 0, 0,
					100, 20, 3,
					0, 100, 255, 255,
					0);
			}
		}

		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, UTIL_VarArgs("[WP] Pathfinder: Path Found (%d)\n", waypoints.length()));
	}
	else
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "[WP] Pathfinder: Path Not Found.\n");
	}

	pFinder->Clear();
}