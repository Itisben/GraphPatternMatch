#include "twohop.h"
#include "main.h"
Twohop::Twohop(Graph *g){
	TwohopCluster c = {0};

	g_ = g;
	max_num_ = g->max_vertex_num();
	clusters_.insert(clusters_.begin(), g->max_vertex_num(), c);
	
	//init the cluster
	for (int i = 0; i < g->max_vertex_num(); i++){
		clusters_[i].center = i;
	}

	debug_ = 0;
	pruning_ = true;
}

Twohop::~Twohop(){
	clusters_.clear();
}

int Twohop::GetDagInScc(Scc *scc, VertexSet *v_set)
{
	Cycles cycles;
	Cycle cycle;
	Vertex *u, *v;
	list<Vertex*> stack;
	int cycle_index = 0;

	/*dfs the scc*/
	for (auto it = scc->begin(); it != scc->end(); it++)
	{
		u = g_->vertex(*it);
		u->color = kWhite;
		if (false == u->exist){
			u->color = kColorNotExist;
		}		
		u->pai = -1;
	}

	for (auto it = scc->begin(); it != scc->end(); it++)
	{
		u = g_->vertex(*it);
		stack.push_back(u);
		while (stack.size() > 0)
		{
		NEXTITEM:
			u = stack.back();
			if (kWhite == u->color)
			{
				u->color = kGray;
			}

			for (auto it_e = u->edges.begin(); it_e != u->edges.end(); it_e++)
			{
				if (scc->end() == scc->find(it_e->vertex)) //not vertex in scc
				{
					continue;
				}
				v = g_->vertex(it_e->vertex);
				if (kWhite == v->color)
				{
					v->pai = u->index;
					stack.push_back(v);
					goto NEXTITEM;
				}
				else if (kGray == v->color) //cycle discover
				{
					/*construt all cycles in scc*/
					for (auto it = stack.rbegin(); it != stack.rend(); it++)
					{
						if (cycles.end() == cycles.find((*it)->index))
						{
							set<int> temp;
							temp.insert(cycle_index);
							cycles[(*it)->index] = temp;
						}
						else
						{
							cycles[(*it)->index].insert(cycle_index);
						}
						if ((*it)->index == v->index) //cyle
							break;
					}	
					cycle_index++;
				}
			}

			u->color = kBlack;
			if (stack.size()>0)
				stack.erase(--stack.end());
		}
	}

	/*set cover, greedy alogorithm, select the vertex with most cycle*/
	while (cycles.size() > 0)
	{
		int cycleNum = 0, selectedVertex; //select the most cover cycles of vertex 
		set<int> *coveredCycle;

		for (auto it = cycles.begin(); it != cycles.end(); it++)
		{
			if (it->second.size() > (unsigned int)cycleNum)
			{
				cycleNum = it->second.size();
				selectedVertex = it->first;
			}
		}
		v_set->insert(selectedVertex);
		coveredCycle = &cycles[selectedVertex];
		for (auto it = cycles.begin(); it != cycles.end();)
		{
			if (selectedVertex == it->first)
			{
				it++;
				continue;
			}
			for (auto it_covered_cycle = coveredCycle->begin(); it_covered_cycle != coveredCycle->end(); it_covered_cycle++)
				it->second.erase(*it_covered_cycle);

			if (0 == it->second.size())
				it = cycles.erase(it++);
			else
				it++;
		}
		auto it = cycles.find(selectedVertex);
		if (it != cycles.end())
			cycles.erase(it);

	}
	return 0;
}

VertexSet* Twohop::GetDag(VertexSet *v_set)
{

	v_set->clear();
	auto sccs = g_->GetSccs();
	for (auto it = sccs->begin(); it != sccs->end(); it++)
	{
		GetDagInScc(&*it, v_set);
	}
	return v_set;
}

int Twohop::ConstructClusters(VertexSet *v_set){
	int w[2], center;
	typedef struct { 
		int center;	int weight; AuxGraph*sp_g[2];
	}AuxCluster;
	vector<AuxCluster> aux_clusters;
	
	AuxCluster aux_cluster;
	/*call back fun*/
	auto dijkstraFun = [](int s, int dir, Vertex*u, int argc, void **data){
		AuxGraph *sp_graph = (AuxGraph*)data[0];

		/*insert the vertex*/
		AuxVertex aux_vertex = { 0 };
		aux_vertex.index = u->index;
		aux_vertex.exist = true;
		aux_vertex.dist = u->dist;
		sp_graph->vertices_[u->index] = aux_vertex;

		/*insert the edge*/
		if (u->pai != -1){
			AuxEdge e = { u->index };
			sp_graph->vertices_[u->pai].edges.push_back(e);
		}
		return 0;
	};

	aux_clusters.reserve(v_set->size());
	/*generate the sp graph by dijkstra for two hop cluster*/
	for (auto it = v_set->begin(); it != v_set->end(); it++){
		auto u = g_->vertex(*it);
		if (false == u->exist)
			continue;
		center = u->index;
		AuxGraph *sp_g[2] = {NULL};
		FOR_EACH_DIRECTION;
		sp_g[dir] = new AuxGraph();
		void *paras[1] = { sp_g[dir] };
		w[dir] = g_->Dijkstra(center, dir, dijkstraFun, 1, paras);
		/*for debug*/
		static int twohop_count = 0;
		twohop_count++;
		if ((0 == twohop_count % 200)) //for debug
			COUT(<< "twohop:: center "<<center <<" dijkstra vertex count : " << twohop_count / 2 << endl;);
		/*for debug end*/
		END_FOR_EACH_DIRCTION;

		aux_cluster.center = center;
		aux_cluster.sp_g[0] = sp_g[0];
		aux_cluster.sp_g[1] = sp_g[1];
		aux_cluster.weight = w[0] + w[1];
		aux_clusters.push_back(aux_cluster);
	}

	/**/
	auto fun = [](AuxCluster u, AuxCluster v){ return u.weight > v.weight; }; //big to small
	sort(aux_clusters.begin(), aux_clusters.end(), fun);
	for (auto it_cur = aux_clusters.begin(); it_cur != aux_clusters.end(); it_cur++){
		
		FOR_EACH_DIRECTION;
		if (true == pruning_){
			AuxGraph *spg = it_cur->sp_g[dir];
			for (auto it_pre = aux_clusters.begin(); it_pre != it_cur; it_pre++){
				ReduceSPGraph(spg, it_pre->center, dir);
			}
		}
		AddClusterItem(it_cur->center, dir, it_cur->sp_g[dir]);
		if (NULL != it_cur->sp_g[dir]){
			delete(it_cur->sp_g[dir]);
			it_cur->sp_g[dir] = NULL;
		}
		END_FOR_EACH_DIRCTION;

		RemoveVertex(it_cur->center);
	}

	return 0;
}

/*need some check*/
int Twohop::ReduceSPGraph(AuxGraph *spg, int pre_center, int dir){
	auto u = &spg->vertices_[pre_center]; //for sp_graph vertex u;	
	if (0 == u->edges.size())
		return -1;

	ClusterItems *ci_pre = &clusters_[pre_center].items[dir];
	queue<AuxVertex*> q;
	q.push(u);
	/*bfs*/
	while (q.size() > 0){
		u = q.front();
		q.pop();
		u->exist = false;
		if (ci_pre->find(u->index) != ci_pre->end())
			u->exist = false; //delete this spgraph node.

		for (auto it = u->edges.begin(); it != u->edges.end(); it++){
			auto v = &spg->vertices_[it->vertex];
			q.push(v);
		}
	}
	return 0;
}

int Twohop::AddClusterItem(int center, int dir, AuxGraph *sp_graph){
	ClusterItems *items = &clusters_[center].items[dir];
	ClusterItem new_item;
	queue<AuxVertex*> q;
	AuxVertex *u, *v;

	//bfs the sp_graph
	q.push(&sp_graph->vertices_[center]);
	while (q.size() > 0){
		u = q.front();
		q.pop();
		if (true == u->exist){	
			new_item = { u->index, u->dist };
			items->insert(new_item);
		}
		
		for (auto it = u->edges.begin(); it != u->edges.end(); it++){
			v = &sp_graph->vertices_[it->vertex];
			q.push(v);
		}
	}
	return 0;
}

int Twohop::RemoveVertex(int center){
	ClusterItems *items = clusters_[center].items;
	FOR_EACH_DIRECTION;
	for (auto it = items[dir].begin(); it != items[dir].end(); it++){
		int vertex = it->first;
		int dist = it->second;
		auto items = &clusters_[vertex].items[OPP(dir)];
		if (items->find(center) == items->end()){
			items->insert({center, dist});
		}
		else{
			if (dist < items->at(center)){
				items->at(center) = dist;
			}
		}
	}
	END_FOR_EACH_DIRCTION;

	g_->RemoveVertex(center, 3);
	return 0;
}

/*after GetDag*/
int Twohop::FixedStrategy(int finnal_vertex_num)
{
	
	stack<Range> stack;
	VertexSet vw;
	auto topo_order = g_->TopoSort();
	Range temp_r = { 0, g_->vertx_num_after_delete() - 1};

	stack.push(temp_r);
	while (stack.size() > 0)
	{
		Range range = stack.top();
		stack.pop();
		///COUT(<< ">>>>>>>range: " << range.start << " " << r.end << endl;)
		if ((range.end - range.start) > finnal_vertex_num)
		{
			/*extract the vertex that can partition the graph*/
			vw.clear();
			int mid = FixedStrategyExtractVertex(range, &vw);
			//COUT(<< ">>>>>>>>>> extrac vw size: " << vw.size() << endl;)
			ConstructClusters(&vw);

			temp_r.start = mid + 1;
			temp_r.end = range.end;
			stack.push(temp_r);
			temp_r.start = range.start;
			temp_r.end = mid;
			stack.push(temp_r);

		}
		else
		{
			vw.clear();
			for (int i = range.start; i <= range.end; i++)
				vw.insert(i);
			ConstructClusters(&vw);
			continue;
		}

	}

	return 0;

}


int Twohop::FixedStrategyExtractVertex(Range range, VertexSet *vw)
{
	int mid = (range.start + range.end) / 2;
	Vertex *u, *v;
	set<int> bottom_vertex;

	for (int i = mid + 1; i < range.end; i++)
	{
		bottom_vertex.insert(g_->topo_sorted_v(i)->index);
	}
	vw->clear();
	/*find the vertices contain the cross edges.*/
	for (int i = range.start; i <= mid; i++)
	{
		u = g_->topo_sorted_v(i);
		if (false == u->exist)
			continue;
		
		for (auto it = u->edges.begin(); it != u->edges.end(); it++)
		{
			v = g_->vertices(it->vertex);
			if (false == v->exist)
				continue;
			if (bottom_vertex.end() != bottom_vertex.find(v->index))
			{
				vw->insert(u->index);
			}
		}

	}
	return mid;
}

int Twohop::WriteCluster(const char *filename, const char *delim){

	int error;
	FILE *fp;
	string strbuf;
	char buf[16];

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	for (auto it = clusters_.begin(); it != clusters_.end(); it++)
	{
		FOR_EACH_DIRECTION;	
		//sort first
		it->items[dir];
		for (auto it2 = it->items[dir].begin(); it2 != it->items[dir].end(); it2++)
		{
			// center, dir, index, weight
			strbuf = _itoa(it->center, buf, 10); //center
			strbuf += delim;
			strbuf += _itoa(dir, buf, 10); //dir
			strbuf += delim;
			strbuf += _itoa(it2->first, buf, 10); //index
			strbuf += delim;
			strbuf += _itoa(it2->second, buf, 10); //weight
			strbuf += "\n";

			//here for pruning save the memory.
			if (it2->second > kMaxDelta)
				continue;

			fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
		}
		
		END_FOR_EACH_DIRCTION;
	}

	fclose(fp);
	return 0;
	
}

int Twohop::ReadCluster(const char * filename, char* delim){
	int error;
	FILE *fp;
	char strbuf[128] = { 0 };
	char *p;
	int value[4];

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	/*center, dir, vertex, dist*/
	while (fgets(strbuf, 128, fp) != NULL)
	{
		if (4 > strlen(strbuf)) // jump the last line
			continue;

		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;
		value[0] = atoi(p);

		int i;
		for (i = 1; (p = strtok(NULL, delim)) != NULL; i++){
			if (NULL == p)
				break;
			value[i] = atoi(p);
		}
		
		if (i != 4)
			continue;

		clusters_[value[0]].center = value[0];
		ClusterItem ci = { value[2], value[3] };
		clusters_[value[0]].items[value[1]].insert(ci);
	}
	fclose(fp);
	return 0;
}

void Twohop::Start(bool pruning, int finnal_vertex_num){
	set_pruning(pruning);
	VertexSet v_set;
	GetDag(&v_set);
	COUT(<< "twohop:: get dag complete!" << endl;);
	ConstructClusters(&v_set);
	FixedStrategy(finnal_vertex_num);
}

int Twohop::CalculateDist(int v1, int v2){
	TwohopCluster *cluster_1 = &clusters_[v1];
	TwohopCluster *cluster_2 = &clusters_[v2];
	int sp = kInfinity;
	
	for (auto it = cluster_1->items[DOUT].begin(); it != cluster_1->items[DOUT].end(); it++){
		int vertex = it->first;
		int dist1 = it->second;
		auto it2 = cluster_2->items[AIN].find(vertex);
		if (it2 != cluster_2->items[AIN].end()){
			auto dist2 = it2->second;
			if (dist1 + dist2 < sp){
				sp = dist1 + dist2;
			}

			if (sp <= kDelta){
				return sp;	
			}
		}
	}

	return sp;
}
int Twohop::NaiveConstructCluster(Group *group){
	
	/*call back fun*/
	auto dijkstraFun = [](int s, int dir, Vertex*u, int argc, void **data){
#if 1
		if (u->dist > kMaxDelta) // for debug and pruning
			return -1;
#endif //for debug
		ClusterItems *item = (ClusterItems *)data[0];
		item->insert({u->index, u->dist});
		return 0;
	};

	for (int i = kStartLable; i <= kEndLabel; i++){
		auto item = group->items(i);
		for (auto it = item->begin(); it != item->end(); it++){
			FOR_EACH_DIRECTION;
			int center = *it;
			ClusterItems *item = &clusters_[center].items[dir];
			void *paras[1] = {item};
			g_->Dijkstra(center, dir, dijkstraFun, 1, paras);

			/*for debug*/
			static int twohop_count = 0;
			twohop_count++;
			if ((0 == twohop_count % 100)) //for debug
				COUT(<< "twohop label:"<< i <<" center:" << center << " dijkstra vertex count : " << twohop_count / 2 << endl;);
			/*for debug end*/
			
			END_FOR_EACH_DIRCTION;
		}
	}

	return 0;
}