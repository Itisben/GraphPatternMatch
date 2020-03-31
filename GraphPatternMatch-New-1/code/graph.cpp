#include "graph.h"
#include "main.h"
int AuxGraph::ClearEdges(){
	vertices_.clear();
	return 0;
}
Graph::Graph(int max_vertex_num, bool directed, bool has_rgraph){
	Vertex v = { 0 };
	max_vertex_num_ = max_vertex_num;
	vertx_num_after_delete_ = max_vertex_num;
	label_num_ = -1; // not be initialized.
	has_rgraph_ = has_rgraph;
	debug_ = 0;
	directed_ = directed;

	//init the graph
	vertices_.reserve(max_vertex_num);
	vertices_.insert(vertices_.begin(), max_vertex_num, v);
	for (int i = 0; i < max_vertex_num; i++){
		vertices_[i].index = i;
		vertices_[i].exist = false;
	}

	if (true == has_rgraph){
		r_vertices_.reserve(max_vertex_num);
		r_vertices_.insert(r_vertices_.begin(), max_vertex_num, v);
		for (int i = 0; i < max_vertex_num; i++){
			r_vertices_[i].exist = false;
			r_vertices_[i].index = i;
		}

		//init topo sorted vertex
		topo_sorted_v_.reserve(max_vertex_num);
		for (auto it = vertices_.begin(); it != vertices_.end(); it++)
		{
			topo_sorted_v_.push_back(&*it);
		}
	}
}

Graph::~Graph(){
	for (int i = 0; i < max_vertex_num_; i++)
		vertices_[i].edges.clear();
	vertices_.clear();
}

int Graph::ReadEdges(const char* filename, char* delim){
	int err = 0;
	Datas *datas = new Datas;
	int vleft, vright, weight = 1;
	Edge e = { 0 };

	int error;
	FILE *fp;
	char strbuf[64] = { 0 };
	char *p;
	Data data;

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << filename << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 64, fp) != NULL)
	{
		if (0 == strlen(strbuf))
			continue;
		if (strbuf[0] == IGNORE_CUR_LINE)
			continue;
		if (strbuf[0] == IGNORE_BELOW_LINE)
			break;
		data.clear();
		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;

		data.push_back(atoi(p));
		for (int i = 1; (p = strtok(NULL, delim)) != NULL && i < DATA_LEN; i++){
			data.push_back(atoi(p));
		}

		vleft = data[0];
		vright = data[1];
		if (data.size() >= 3){
			weight = data[2];
		}
		
		if (1 == kRandWeight)
			weight = 1;
		
		if (vleft == vright){ //ignore the self loop
			continue;
		}
		vertices_[vleft].exist = true;
		vertices_[vright].exist = true;
		e.vertex = vright;
		e.weight = weight;
		vertices_[vleft].edges.push_back(e);

		if (false == directed()){ //undirected grpah read other edge
			e.vertex = vleft;
			e.weight = weight;
			vertices_[vright].edges.push_back(e);
		}

		//for reverse graph
		if (true == has_rgraph_){
			e.vertex = vleft;
			e.weight = weight;
			r_vertices_[vleft].exist = true;
			r_vertices_[vright].exist = true;
			r_vertices_[vright].edges.push_back(e);
			if (false == directed()){ //undirected graph 
				e.vertex = vright;
				e.weight = weight;
				vertices_[vleft].edges.push_back(e);
			}
		}
		
	}
	fclose(fp);
	return 0;
}

int Graph::ReadLabels(const char* filename, char* delim){
	int err = 0;
	Datas *datas = new Datas;
	int index, label;
	int error;
	FILE *fp;
	char strbuf[64] = { 0 };
	char *p;
	Data data;

	set<int> label_set;

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 64, fp) != NULL)
	{
		if (0 == strlen(strbuf))
			continue;
		if (strbuf[0] == IGNORE_CUR_LINE)
			continue;
		if (strbuf[0] == IGNORE_BELOW_LINE)
			break;
		data.clear();
		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;

		data.push_back(atoi(p));
		for (int i = 1; (p = strtok(NULL, delim)) != NULL && i < DATA_LEN; i++){
			data.push_back(atoi(p));
		}
		datas->push_back(data);

		index = data[0];
		label = data[1];
		vertices_[index].label = label;

		if (true == has_rgraph_){
			r_vertices_[index].label = label;
		}

		label_set.insert(label);

	}
	fclose(fp);

	label_num_ = label_set.size();

	return 0;
}
/*
* this dfs using stack rather than recusive.
*/
ERR Graph::Dfs(DfsFun dfsFun, int argc, void **argv)
{
	stack<Vertex*> stack;
	int time = 0;
	Vertex *u, *v;

	/*init all*/
	for (auto it = vertices_.begin(); it != vertices_.end(); it++)
	{
		it->pai = -1;
		it->d = 0;
		it->f = 0;
		it->color = kWhite;
		if (false == it->exist)
			it->color = kColorNotExist;
	}

	for (auto it_u = vertices_.begin(); it_u != vertices_.end(); it_u++)
	{
		u = &*it_u;

		if (kWhite != u->color) //jump over the one been iteratered.
			continue;

		stack.push(u);
		while (stack.size() > 0)
		{
NEXTITEM:
			u = stack.top();
			if (kWhite == u->color)
			{
				u->d = ++time;
				u->color = kGray;
				if (NULL != dfsFun){
					if (-1 == dfsFun(u, argc, argv)){
						return -1;
					}
				}
			}	

			/*we can optimize here for avoiding the iteration.*/
			for (auto it_v = u->edges.begin(); it_v != u->edges.end(); it_v++)
			{
				v = &vertices_[it_v->vertex];
				if (kWhite == v->color)
				{
					v->pai = u->index;
					stack.push(v);
					goto NEXTITEM;
				}
			}		
			
			u->f = ++time;
			u->color = kBlack;
			stack.pop();
		}
	}
	return 0;
}
/*
* this dfs using stack rather than recusive.
*/
ERR Graph::DfsInTopoOrder(DfsFun dfsFun, int argc, void **argv)
{
	stack<Vertex*> stack;
	int time = 0;
	Vertex *u, *v;

	TopoSort();

	/*init all*/
	for (auto it = vertices_.begin(); it != vertices_.end(); it++)
	{
		it->pai = -1;
		it->d = 0;
		it->f = 0;
		it->color = kWhite;
		if (false == it->exist)
			it->color = kColorNotExist;
	}

	for (auto it_u = topo_sorted_v_.begin(); it_u != topo_sorted_v_.end(); it_u++)
	{
		u = *it_u;

		if (kWhite != u->color) //jump over the one been iteratered.
			continue;

		stack.push(u);
		while (stack.size() > 0)
		{
		NEXTITEM:
			u = stack.top();
			if (kWhite == u->color)
			{
				u->d = ++time;
				u->color = kGray;
				if (NULL != dfsFun){
					if (-1 == dfsFun(u, argc, argv)){
						return -1;
					}
				}
			}

			/*we can optimize here for avoiding the iteration.*/
			for (auto it_v = u->edges.begin(); it_v != u->edges.end(); it_v++)
			{
				v = &vertices_[it_v->vertex];
				if (kWhite == v->color)
				{
					v->pai = u->index;
					stack.push(v);
					goto NEXTITEM;
				}
			}

			u->f = ++time;
			u->color = kBlack;
			stack.pop();
		}
	}
	return 0;
}

vector<Vertex*> *Graph::TopoSort(){
	Dfs();
	sort(topo_sorted_v_.begin(), topo_sorted_v_.end(), 
			[](Vertex *i, Vertex *j){ return (i->f>j->f);}
		);

	return &topo_sorted_v_;
}

/*
*s : the index of the start vertex.
*para_fun: the function that is for the dijkstra
*/
int Graph::Dijkstra(int s, int direction, DijkstraFun dikstraFun, int argc, void **argv){
	if (false == has_rgraph_ && direction == AIN)
		return -1;
	
	typedef struct{
		int index;
		int dist;
	}HeapV;
	Vertices *cur_vertices;
	vector<HeapV> heap;
	set<int> heap_set;
	HeapV u;
	HeapV heapV = {0};
	Vertex *u_p, *v_p;

	/*minimum heap*/
	auto fun = [](HeapV u, HeapV v){ return u.dist > v.dist; };
	
	//init heap
	heap.reserve(max_vertex_num_);

	//init
	if (0 == direction)
		cur_vertices = &vertices_;
	else
		cur_vertices = &r_vertices_;

	if (false == cur_vertices->at(s).exist)
		return -2;

	for (auto it = cur_vertices->begin(); it != cur_vertices->end(); it++)
	{
		it->dist = kInfinity;
		it->pai = -1;
		it->color = kWhite;
		if (false == it->exist){
			it->color = kColorNotExist;
		}
	}
	cur_vertices->at(s).dist = 0;

	//COUT(<< "the shortest path start from:" << s << endl;);
	heapV = { s, 0 };
	heap.push_back(heapV);
	make_heap(heap.begin(), heap.end(), fun);

	while (heap.size() > 0)
	{
		/*find the next nearest vertex by heap*/
		bool has_next = false;
		while (heap.size() > 0){
			u = heap.front();
			pop_heap(heap.begin(), heap.end(), fun);
			heap.pop_back();
			/*if this vertex has been visted, than get next.
			*if this vertex has not been visted, get it and jump out.*/
			if (heap_set.end() == heap_set.find(u.index)){
				has_next = true;
				u_p = &cur_vertices->at(u.index);
				u_p->color = kBlack;
				u_p->dist = u.dist;
				heap_set.insert(u.index);
				/*call back*/
				if (-1 == dikstraFun(s, direction, u_p, argc, argv))
					return -1;
				break;
			}
		}
		
		if (false == has_next)
			break;

		//here create the sp graph
		u_p = &(*cur_vertices)[u.index];
		for (auto it = u_p->edges.begin(); it != u_p->edges.end(); it++)
		{
			v_p = &(*cur_vertices)[it->vertex];
			if (kWhite != v_p->color) //jump over not exist vertex.
				continue;
			//relax
			int new_dist = u_p->dist + it->weight;;
			if (new_dist < v_p->dist)
			{
				v_p->dist = new_dist;
				v_p->pai = u_p->index;
				HeapV heapv = { v_p->index, new_dist };
				heap.push_back(heapv);
				push_heap(heap.begin(), heap.end(), fun);	
			}
		}


	}
	return heap_set.size();
}

/*
*s : the index of the start vertex.
*para_fun: the function that is for the dijkstra
*/
int Graph::AStar(int s, int direction, DijkstraFun dikstraFun, int argc, void **argv,
	HeuristicFun heuristicFun, int argc2, void **argv2){
	if (false == has_rgraph_ && direction == AIN)
		return -1;

	typedef struct{
		int index;
		int dist;
	}HeapV;
	Vertices *cur_vertices;
	vector<HeapV> heap;
	set<int> heap_set;
	HeapV u;
	HeapV heapV = { 0 };
	Vertex *u_p, *v_p;

	/*minimum heap*/
	auto fun = [](HeapV u, HeapV v){ return u.dist > v.dist; };

	//init heap
	heap.reserve(max_vertex_num_);

	//init
	if (0 == direction)
		cur_vertices = &vertices_;
	else
		cur_vertices = &r_vertices_;

	if (false == cur_vertices->at(s).exist)
		return -2;

	for (auto it = cur_vertices->begin(); it != cur_vertices->end(); it++)
	{
		it->dist = kInfinity;
		it->pai = -1;
		it->color = kWhite;
		if (false == it->exist){
			it->color = kColorNotExist;
		}
	}
	cur_vertices->at(s).dist = 0;

	COUT(<< "the shortest path start from:" << s << endl;);
	heapV = { s, 0 };
	heap.push_back(heapV);
	make_heap(heap.begin(), heap.end(), fun);

	while (heap.size() > 0)
	{
		/*find the next nearest vertex by heap*/
		bool has_next = false;
		while (heap.size() > 0){
			u = heap.front();
			pop_heap(heap.begin(), heap.end(), fun);
			heap.pop_back();
			/*if this vertex has been visted, than get next.
			*if this vertex has not been visted, get it and jump out.*/
			if (heap_set.end() == heap_set.find(u.index)){
				has_next = true;
				u_p = &cur_vertices->at(u.index);
				u_p->color = kBlack;
				u_p->dist = u.dist;
				heap_set.insert(u.index);
				/*call back*/
				if (-1 == dikstraFun(s, direction, u_p, argc, argv))
					return -1;
				break;
			}
		}

		if (false == has_next)
			break;

		//here create the sp graph
		u_p = &(*cur_vertices)[u.index];
		for (auto it = u_p->edges.begin(); it != u_p->edges.end(); it++)
		{
			v_p = &(*cur_vertices)[it->vertex];
			if (kWhite != v_p->color) //jump over not exist vertex.
				continue;
			//relax
			/*heuristic funtion begin*/
			int h = heuristicFun(direction, v_p, argc, argv); 
			/*heuristic funtion end*/
			int new_dist = u_p->dist + it->weight + h;
			if (new_dist < v_p->dist)
			{
				v_p->dist = new_dist;
				v_p->pai = u_p->index;
				HeapV heapv = { v_p->index, new_dist };
				heap.push_back(heapv);
				push_heap(heap.begin(), heap.end(), fun);
			}
		}
	}
	return heap_set.size();
}
void Graph::TextDijkstra(){
	for (auto it = vertices_.rbegin(); it != vertices_.rend(); it++){
		Dijkstra(it->index, 0);
	}
}
/*
*get scc : out the each tree in the depth-first forest in the reverse graph.
*/
Sccs * Graph::GetSccs()
{
	if (false == has_rgraph_)
		return NULL;

	stack<Vertex*> stack;
	Vertex *u, *v;
	set<int> scc;

	auto topo = TopoSort();

	/*init*/
	for (auto it = vertices_.begin(); it != vertices_.end(); it++)
	{
		it->color = kWhite;
		if (false == it->exist)
			it->color = kColorNotExist;
		it->pai = -1;
	}

	/*call dfs in reverse graph */
	for (auto it_v = topo->begin(); it_v != topo->end(); it_v++)
	{
		if (scc.size() > 1) /*if scc size is 0, it is no cycle in it.*/
		{
			sccs_.push_back(scc); /*scc is the dfs tree in the forest*/
		}
		scc.clear();

		u = &r_vertices_[(*it_v)->index];
		if (u->color != kWhite)
			continue;
		stack.push(u);
		while (stack.size() > 0)
		{
		NEXTITEM:
			u = stack.top();
			if (kWhite == u->color)
			{
				u->color = kGray;
				scc.insert(u->index);
			}
				
			for (auto it_e = u->edges.begin(); it_e != u->edges.end(); it_e++)
			{
				v = &r_vertices_[(*it_e).vertex];
				if (kWhite == v->color)
				{
					v->pai = u->index;
					stack.push(v);
					goto NEXTITEM;
				}
			}

			u->color = kBlack;
			stack.pop();
		}
	}
	return &sccs_;
}

int Graph::RemoveVertex(int i, int threshold){
	static int count = 0;
	vertices_[i].exist = false;
	vertx_num_after_delete_--;
	count++;
	//clear
	if (count <= threshold) return 1;

	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		for (auto it_e = it->edges.begin(); it_e != it->edges.end();){
			if (false == vertices_[it_e->vertex].exist){
				it->edges.erase(it_e++);
			}
			else{
				it_e++;
			}
		}
	}
	count = 0;
	return 0;
}