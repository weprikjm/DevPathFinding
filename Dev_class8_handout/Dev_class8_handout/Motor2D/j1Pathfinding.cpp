#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1PathFinding.h"

j1PathFinding::j1PathFinding() : j1Module(), map(NULL), last_path(DEFAULT_PATH_LENGTH),width(0), height(0)
{
	name.create("pathfinding");
}

// Destructor
j1PathFinding::~j1PathFinding()
{
	RELEASE_ARRAY(map);
}

// Called before quitting
bool j1PathFinding::CleanUp()
{
	LOG("Freeing pathfinding library");

	last_path.Clear();
	RELEASE_ARRAY(map);
	return true;
}

// Sets up the walkability map
void j1PathFinding::SetMap(uint width, uint height, uchar* data)
{
	this->width = width;
	this->height = height;

	RELEASE_ARRAY(map);
	map = new uchar[width*height];
	memcpy(map, data, width*height);
}

// Utility: return true if pos is inside the map boundaries
bool j1PathFinding::CheckBoundaries(const iPoint& pos) const
{
	return (pos.x >= 0 && pos.x <= (int)width &&
			pos.y >= 0 && pos.y <= (int)height);
}

// Utility: returns true is the tile is walkable
bool j1PathFinding::IsWalkable(const iPoint& pos) const
{
	uchar t = GetTileAt(pos);
	return t != INVALID_WALK_CODE && t > 0;
}

// Utility: return the walkability value of a tile
uchar j1PathFinding::GetTileAt(const iPoint& pos) const
{
	if(CheckBoundaries(pos))
		return map[(pos.y*width) + pos.x];

	return INVALID_WALK_CODE;
}

// To request all tiles involved in the last generated path
const p2DynArray<iPoint>* j1PathFinding::GetLastPath() const
{
	return &last_path;
}

// PathList ------------------------------------------------------------------------
// Looks for a node in this list and returns it's list node or NULL
// ---------------------------------------------------------------------------------
p2List_item<PathNode>* PathList::Find(const iPoint& point) const
{
	p2List_item<PathNode>* item = list.start;
	while(item)
	{
		if(item->data.pos == point)
			return item;
		item = item->next;
	}
	return NULL;
}

// PathList ------------------------------------------------------------------------
// Returns the Pathnode with lowest score in this list or NULL if empty
// ---------------------------------------------------------------------------------
p2List_item<PathNode>* PathList::GetNodeLowestScore() const
{
	p2List_item<PathNode>* ret = NULL;
	int min = 65535;

	p2List_item<PathNode>* item = list.end;
	while(item)
	{
		if(item->data.Score() < min)
		{
			min = item->data.Score();
			ret = item;
		}
		item = item->prev;
	}
	return ret;
}

// PathNode -------------------------------------------------------------------------
// Convenient constructors
// ----------------------------------------------------------------------------------
PathNode::PathNode() : g(-1), h(-1), pos(-1, -1), parent(NULL)
{}

PathNode::PathNode(int g, int h, const iPoint& pos, const PathNode* parent) : g(g), h(h), pos(pos), parent(parent)
{}

PathNode::PathNode(const PathNode& node) : g(node.g), h(node.h), pos(node.pos), parent(node.parent)
{}

// PathNode -------------------------------------------------------------------------
// Fills a list (PathList) of all valid adjacent pathnodes
// ----------------------------------------------------------------------------------
uint PathNode::FindWalkableAdjacents(PathList& list_to_fill) const
{
	iPoint cell;
	uint before = list_to_fill.list.count();

	// north
	cell.create(pos.x, pos.y + 1);
	
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// east
	cell.create(pos.x + 1, pos.y);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// south
	cell.create(pos.x, pos.y - 1);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// west
	cell.create(pos.x - 1, pos.y);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	return list_to_fill.list.count();
}

// PathNode -------------------------------------------------------------------------
// Calculates this tile score
// ----------------------------------------------------------------------------------
int PathNode::Score() const
{
	return g + h;
}

// PathNode -------------------------------------------------------------------------
// Calculate the F for a specific destination tile
// ----------------------------------------------------------------------------------
int PathNode::CalculateF(const iPoint& destination)
{
	g = parent->g + 1;
	h = pos.DistanceTo(destination);

	return g + h;
}

// ----------------------------------------------------------------------------------
// Actual A* algorithm: return number of steps in the creation of the path or -1 ----
// ----------------------------------------------------------------------------------
int j1PathFinding::CreatePath(const iPoint& origin, const iPoint& destination)
{
	// TODO 1: if origin or destination are not walkable, return -1
	last_path.Clear();

	if (IsWalkable(origin) && IsWalkable(destination))
	{
		uint num = 0;
		PathList open;
		PathList closed;
		PathList adjacents;
		PathNode source;
		p2List_item<PathNode>* closest;
		p2List_item<PathNode>* operations;
		bool isfinish = false;

		source.pos.x = origin.x;
		source.pos.y = origin.y;
		source.h = (abs(origin.x - destination.x) + abs(origin.y - destination.y));
		source.g = 0;
		source.parent = NULL;

		open.list.add(source);


		num = open.Find(source.pos)->data.FindWalkableAdjacents(adjacents);
		closed.list.add(open.Find(source.pos)->data);
		open.list.del(open.Find(source.pos));

		while (isfinish == NULL)
		{
			for (int i = 0; i < num; i++)
			{
				adjacents.list.At(i)->data.parent = &source;
				adjacents.list.At(i)->data.CalculateF(destination);
				adjacents.list.At(i)->data.h = adjacents.list.At(i)->data.pos.DistanceTo(destination);

				if (adjacents.list.At(i)->data.pos.DistanceManhattan(destination) == 0)
				{
					closed.list.add(adjacents.list.At(i)->data);
					isfinish = true;
					break;
				}
				
				if (closed.Find(adjacents.list.At(i)->data.pos)) { // if this adjacent square is already in the closed list ignore it
					continue; // Go to the next adjacent square
				}

				if (open.Find(adjacents.list.At(i)->data.pos) == NULL)
				{
					open.list.add(adjacents.list.At(i)->data);
				}


				else { // if its already in the open list
				}


			}

			adjacents.list.clear();

			closest = open.GetNodeLowestScore();
			num = open.Find(closest->data.pos)->data.FindWalkableAdjacents(adjacents);
			closed.list.add(closest->data);

			source = closest->data;

		}

		for (int i = 0; i < closed.list.count(); i++)
		{ 
			last_path.PushBack(closed.list.At(i)->data.pos);
			LOG("Point: %d, %d", last_path.At(i)->x, last_path.At(i)->y);
		}
		last_path.Flip();
		
		

	}



	// TODO 2: Create three lists: open, close
	// Add the origin tile to open
	// Iterate while we have tile in the open list

	// TODO 3: Move the lowest score cell from open list to the closed list
	
	// TODO 4: If we just added the destination, we are done!
	// Backtrack to create the final path
	// Use the Pathnode::parent and Flip() the path when you are finish

	// TODO 5: Fill a list of all adjancent nodes

	// TODO 6: Iterate adjancent nodes:
	// ignore nodes in the closed list
	// If it is NOT found, calculate its F and add it to the open list
	// If it is already in the open list, check if it is a better path (compare G)
	// If it is a better path, Update the parent

	return -1;
}

bool PathList::NodeExists()const
{
	
	return true;

}