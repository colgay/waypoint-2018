#pragma once

#include "amtl/am-vector.h"
#include "amtl/am-hashmap.h"
#include "extdll.h"

class Waypoint
{
public:
	Waypoint(const Vector &pos, float radius = 0, int flags = 0);
	~Waypoint();

	Vector GetPos() const { return m_position; }
	float GetRadius() const { return m_radius; }
	int GetFlags() const { return m_flags; }
	
	void SetPos(const Vector &pos) { m_position = pos; }
	void SetRadius(float radius) { m_radius = radius; }
	void SetFlags(int flags) { m_flags = flags; }

	bool GetChild(Waypoint *pPoint) const;
	bool AddChild(Waypoint *pPoint);
	void RemoveChild(Waypoint *pPoint);

	int GetChildFlags(Waypoint *pPoint) const;
	void SetChildFlags(Waypoint *pPoint, int flags);
	void PopChildFlags(Waypoint *pPoint);

	const ke::Vector<Waypoint *> &GetChildren() const { return m_children; }

private:
	Vector m_position;
	float m_radius;
	int m_flags;
	ke::Vector<Waypoint *> m_children;
	typedef ke::HashMap<Waypoint *, int, ke::PointerPolicy<Waypoint>> FlagsMap;
	FlagsMap m_flagsmap;
};