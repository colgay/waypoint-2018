#pragma once

#include <amtl\am-vector.h>

class Vector;
class Waypoint;

class Graph
{
public:
	Graph();
	~Graph();

	Waypoint *CreateWaypoint(const Vector &pos, float radius, int flags);
	void RemoveWaypoint(Waypoint *pPoint);
	int Index(Waypoint *pPoint) const;
	Waypoint *GetWaypointAt(size_t index) const;
	Waypoint *GetClosestPoint(const Vector &pos, float maxRadius) const;
	Vector GetClosestPath(const Vector &pos, Waypoint *&pBegin, Waypoint *&pEnd);
	size_t Size() const { return m_waypoints.length(); }
	void Clear();
	const ke::Vector<Waypoint *> &GetWaypoints() const { return m_waypoints; }

private:
	ke::Vector<Waypoint *> m_waypoints;
};

