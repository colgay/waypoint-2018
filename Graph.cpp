#include "Graph.h"
#include "amtl/am-hashset.h"
#include "amtl/am-hashmap.h"
#include "Waypoint.h"
#include "Utilities.h"
#include "meta_api.h"

Graph::Graph()
{

}

Graph::~Graph()
{
	Clear();
}

void Graph::Clear()
{
	for (auto it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
	{
		delete *it;
	}

	m_waypoints.clear();
}

Waypoint *Graph::CreateWaypoint(const Vector &pos, float radius, int flags)
{
	Waypoint *pPoint = new Waypoint(pos, radius, flags);

	m_waypoints.append(pPoint);
	return pPoint;
}

void Graph::RemoveWaypoint(Waypoint *pPoint)
{
	for (auto it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
	{
		(*it)->PopChildFlags(pPoint);

		if (*it == pPoint)
		{
			delete *it;
			m_waypoints.remove(it - m_waypoints.begin());
			break;
		}
	}
}

int Graph::Index(Waypoint *pPoint) const
{
	for (auto it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
	{
		if (*it == pPoint)
			return it - m_waypoints.begin();
	}

	return -1;
}

Waypoint *Graph::GetWaypointAt(size_t index) const
{
	if (index >= m_waypoints.length())
		return nullptr;

	return m_waypoints.at(index);
}

Waypoint *Graph::GetClosestPoint(const Vector &pos, float maxRadius) const
{
	Waypoint *pPoint, *pResult = nullptr;
	float dist, minDist = maxRadius;

	for (auto it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
	{
		pPoint = *it;

		dist = (pPoint->GetPos() - pos).Length();
		if (dist < minDist && UTIL_IsVisible(pos, pPoint->GetPos(), NULL, ignore_monsters))
		{
			pResult = pPoint;
			minDist = dist;
		}
	}

	return pResult;
}

Vector Graph::GetClosestPath(const Vector &pos, Waypoint *&pBegin, Waypoint *&pEnd)
{
	typedef ke::HashSet<Waypoint *, ke::PointerPolicy<Waypoint>> PathSet;
	PathSet hashset;
	hashset.init();

	float dist, minDist = 999999;
	Vector closestPos, outputPos;
	Waypoint *pPoint, *pChild;

	for (auto it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
	{
		pPoint = *it;

		PathSet::Insert p = hashset.findForAdd(pPoint);
		if (!p.found())
			hashset.add(p, pPoint);

		auto &children = pPoint->GetChildren();
		for (auto iter = children.begin(); iter != children.end(); ++iter)
		{
			pChild = *iter;

			PathSet::Insert p = hashset.findForAdd(pChild);
			if (p.found())
				continue;

			hashset.add(p, pChild);

			dist = UTIL_DistPointSegment(pos, pPoint->GetPos(), pChild->GetPos(), closestPos);
			if (dist < minDist && UTIL_IsVisible(pos, closestPos, NULL, ignore_monsters))
			{
				minDist = dist;
				pBegin = pPoint;
				pEnd = pChild;
				outputPos = closestPos;
			}
		}
	}

	return outputPos;
}