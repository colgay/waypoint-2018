#pragma once

#include "amtl/am-hashset.h"
#include "amtl/am-hashmap.h"
#include "extdll.h"

class Waypoint;
class Graph;
class AStar;

class Editor
{
public:
	Editor() : m_time(0), m_auto(false), m_conn(true), m_autoDist(120), m_connDist(180) {}
	~Editor() {}

	typedef ke::HashSet<Waypoint *, ke::PointerPolicy<Waypoint>> PointSet;

	edict_t *GetEditor() const;
	void SetEditor(edict_t *pEditor);
	void SetStart(Waypoint *pPoint);
	void SetGoal(Waypoint *pPoint);
	void SetAutoWpDist(float dist);
	void SetAutoConnDist(float dist);
	void SetAutoWp(bool toggle);
	void SetAutoConn(bool toggle);
	Waypoint *ArgPoint(const char *pszArg) const;
	void SetGraph(Graph *pGraph);
	void Create(float radius) const;
	void ConnectPoints(Waypoint *pPoint) const;
	void Remove(Waypoint *pPoint, bool all = false);
	void Connect(Waypoint *pPoint1, Waypoint *pPoint2, int type) const;
	void Disconnect(Waypoint *pPoint1, Waypoint *pPoint2) const;
	Waypoint *GetAim(edict_t *pEntity, float maxDist, PointSet &hashset) const;
	void Draw(edict_t *pEntity);
	void Reset();
	void Save(const char *pszPath);
	void Load(const char *pszPath);
	void PathFinder(AStar *pFinder);

	Waypoint *m_pAim, *m_pCurrent;

private:
	Waypoint *m_pStart, *m_pGoal;
	edict_t *m_pEditor;
	Graph *m_pGraph;
	float m_time;
	bool m_auto, m_conn;
	float m_autoDist, m_connDist;
};