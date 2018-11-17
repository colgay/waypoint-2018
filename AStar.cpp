#include "AStar.h"

#include "meta_api.h"

#include "Waypoint.h"
#include "Graph.h"
#include "Path.h"
#include "Utilities.h"

AStar::AStar()
{
	m_map.init();
}

AStar::~AStar()
{
	this->Release();
}

void AStar::Init(Graph *pGraph)
{
	m_pGraph = pGraph;

	Node *pNode;
	auto &waypoints = m_pGraph->GetWaypoints();
	for (auto it = waypoints.begin(); it != waypoints.end(); ++it)
	{
		pNode = new Node(*it);
		NodeMap::Insert i = m_map.findForAdd(*it);
		m_map.add(i, *it, pNode);
	}
}

AStar::Node::Node(Waypoint *pPoint) : f(9999999), g(9999999), isOpen(false), isClosed(false), parent(nullptr)
{
	wp = pPoint;
}

void AStar::Node::Reset()
{
	this->f = 9999999;
	this->g = 9999999;
	this->parent = nullptr;
	this->isOpen = false;
	this->isClosed = false;
}

bool AStar::CalcPath(Waypoint *pStart, Waypoint *pGoal, Path &path)
{
	Node *pNode = GetNode(pStart);
	if (pNode == nullptr)
		return false;

	Waypoint *pChild;
	Node *pChildNode;

	float gCost;

	pNode->f = (pStart->GetPos() - pGoal->GetPos()).Length();
	pNode->g = 0;
	PushOpen(pNode);

	while (!m_open.empty())
	{
		pNode = m_open.peek();

		if (pNode->wp == pGoal)
		{
			ReconstructPath(pNode, path);
			return true;
		}

		PopOpen();
		PushClosed(pNode);

		auto &children = pNode->wp->GetChildren();
		for (auto it = children.begin(); it != children.end(); ++it)
		{
			pChild = *it;
			pChildNode = GetNode(pChild);
			if (pChildNode == nullptr || pChildNode->isClosed)
				continue;

			gCost = pNode->g + (pNode->wp->GetPos() - pChild->GetPos()).Length();
			
			if (!pChildNode->isOpen)
				PushOpen(pChildNode);
			else if (gCost > pChildNode->g)
				continue;

			pChildNode->g = gCost;
			pChildNode->f = gCost + (pChild->GetPos() - pGoal->GetPos()).Length();
			pChildNode->parent = pNode;
		}
	}

	return false;
}

AStar::Node *AStar::GetNode(Waypoint *pPoint) const
{
	NodeMap::Result r = m_map.find(pPoint);
	if (r.found())
		return r->value;

	return nullptr;
}

void AStar::PushOpen(Node *pNode)
{
	pNode->isOpen = true;
	m_open.add(pNode);
}

void AStar::PopOpen()
{
	Node *pNode = m_open.peek();
	pNode->isOpen = false;
	m_open.popCopy();
}

void AStar::PushClosed(Node *pNode)
{
	pNode->isClosed = true;
	//m_closed.append(pNode);
}

void AStar::Clear(bool release)
{
	Node *pNode;
	for (NodeMap::iterator iter = m_map.iter(); !iter.empty(); iter.next())
	{
		pNode = iter->value;
		if (pNode != nullptr)
		{
			if (release)
				delete pNode;
			else
				pNode->Reset();
		}
	}

	if (release)
	{
		m_map.clear();
	}

	while (!m_open.empty())
	{
		m_open.popCopy();
	}
}

void AStar::Release()
{
	Node *pNode;
	for (NodeMap::iterator iter = m_map.iter(); !iter.empty(); iter.next())
	{
		pNode = iter->value;
		if (pNode != nullptr)
			delete pNode;
	}
}

void AStar::ReconstructPath(Node *pNode, Path &path)
{
	Node *pParent = pNode;

	do
	{
		path.Push(pParent->wp);
		pParent = pParent->parent;

	} while (pParent != nullptr);
}