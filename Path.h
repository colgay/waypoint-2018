#pragma once

#include <amtl/am-vector.h>

#include "Waypoint.h"
#include "Utilities.h"

class Path
{
public:
	Path() {}
	~Path() {}

	void Push(Waypoint *pPoint)
	{
		if (m_waypoints.empty())
			m_waypoints.append(pPoint);
		else
			m_waypoints.insert(0, pPoint);
	}

	void PushBack(Waypoint *pPoint)
	{
		m_waypoints.append(pPoint);
	}

	void Pop()
	{
		m_waypoints.remove(0);
	}

	void PopBack()
	{
		m_waypoints.pop();
	}

	Waypoint *Back()
	{
		return m_waypoints.back();
	}

	Waypoint *At(size_t index)
	{
		return m_waypoints.at(index);
	}

	Vector GetClosestPath(const Vector &pos, float maxDist, Waypoint *&pBegin, Waypoint *&pEnd)
	{
		Waypoint *pPoint, *pPoint2;
		Vector closestPos, outputPos;
		float dist, minDist = maxDist;

		for (auto it = m_waypoints.begin(); it != (m_waypoints.end() - 1); ++it)
		{
			pPoint = *it;
			pPoint2 = *(it + 1);

			if (pPoint2 == nullptr)
				continue;

			dist = UTIL_DistPointSegment(pos, pPoint->GetPos(), pPoint2->GetPos(), closestPos);
			if (dist < minDist && UTIL_IsWalkable(pos, closestPos, head_hull, NULL, ignore_monsters))
			{
				minDist = dist;
				pBegin = pPoint;
				pEnd = pPoint2;
				outputPos = closestPos;
			}
		}

		return outputPos;
	}

	const ke::Vector<Waypoint *> &GetWaypoints() const
	{
		return m_waypoints;
	}

	void Clear()
	{
		m_waypoints.clear();
	}

	size_t Size() const
	{
		return m_waypoints.length();
	}

private:
	ke::Vector<Waypoint *> m_waypoints;
};