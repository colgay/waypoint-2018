#include "Waypoint.h"

Waypoint::Waypoint(const Vector &pos, float radius, int flags) : m_position(pos), m_radius(radius), m_flags(flags)
{
	m_flagsmap.init();
}

Waypoint::~Waypoint()
{
	for (auto it = m_children.begin(); it != m_children.end(); ++it)
	{
		if ((*it)->GetChild(this))
		{
			(*it)->RemoveChild(this);
		}
	}
}

bool Waypoint::GetChild(Waypoint *pPoint) const
{
	for (auto it = m_children.begin(); it != m_children.end(); ++it)
	{
		if (*it == pPoint)
			return true;
	}

	return false;
}

bool Waypoint::AddChild(Waypoint *pPoint)
{
	if (this->GetChild(pPoint) || pPoint == this)
		return false;

	m_children.append(pPoint);
	return true;
}

void Waypoint::RemoveChild(Waypoint *pPoint)
{
	for (auto it = m_children.begin(); it != m_children.end(); ++it)
	{
		if (*it == pPoint)
		{
			m_children.remove(it - m_children.begin());
			break;
		}
	}
}

int Waypoint::GetChildFlags(Waypoint *pPoint) const
{
	FlagsMap::Result r = m_flagsmap.find(pPoint);
	if (r.found())
		return r->value;

	return 0;
}

void Waypoint::SetChildFlags(Waypoint *pPoint, int flags)
{
	if (flags == 0)
	{
		PopChildFlags(pPoint);
		return;
	}

	FlagsMap::Insert i = m_flagsmap.findForAdd(pPoint);
	if (!i.found())
		m_flagsmap.add(i, pPoint, flags);
	else
		i->value = flags;
}

void Waypoint::PopChildFlags(Waypoint *pPoint)
{
	FlagsMap::Result r = m_flagsmap.find(pPoint);
	if (r.found())
		m_flagsmap.remove(r);
}