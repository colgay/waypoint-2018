#pragma once

#include "amtl/am-hashmap.h"
#include "amtl/am-priority-queue.h"
#include "Waypoint.h"

class Graph;
class Path;

class AStar
{
public:
	struct Node
	{
		float f, g;
		bool isOpen, isClosed;
		Node *parent;
		Waypoint *wp;

		void Reset();
		Node(Waypoint *pPoint);
	};

	AStar();
	~AStar();

	void Init(Graph *pGraph);
	bool CalcPath(Waypoint *pStart, Waypoint *pGoal, Path &path);
	void Clear(bool release = false);

private:
	void ReconstructPath(Node *pNode, Path &path);
	void PushOpen(Node *pNode);
	void PopOpen();
	void PushClosed(Node *pNode);
	void Release();
	Node *GetNode(Waypoint *pPoint) const;

	struct NodeCompare
	{
		bool operator ()(Node *left, Node *right) const {
			return left->f < right->f;
		}
	};

	ke::PriorityQueue<Node *, NodeCompare> m_open;
	//ke::Vector<Node *> m_closed;
	Graph *m_pGraph;

	typedef ke::HashMap<Waypoint *, Node *, ke::PointerPolicy<Waypoint>> NodeMap;
	NodeMap m_map;
};