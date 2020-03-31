#include "astar-ac.h"

AACGroup::AACGroup(Graph *g){
	g_ = g;
	label_num_ = g->label_num() + 1; //for 0
	items_.reserve(label_num_);

	list<int> item;
	items_.insert(items_.begin(), label_num_, item);

	//init all the group
	for (auto it = g_->vertices()->begin(); it != g_->vertices()->end(); it++){
		if (it->label < 0){
			cout << "Group init error" << endl;
			continue;
		}
		if (it->label >= (int)items_.size()){
			cout << "label_num " << label_num_ << endl;
			cout << "insert vertex  " << it->index << " to label " << it->label << endl;
		}
		items_[it->label].push_back(it->index);
	}
}

AstarAC::AstarAC(Graph *g, AACGroup *group, Query *query, int pivot_num){
	g_ = g;
	group_ = group;
	query_ = query;
	delta_ = query->delta();
	pivot_num_ = pivot_num;

	/*for vertex*/
	vector<int> dist;
	dist.insert(dist.begin(), pivot_num, kInfinity);
	pivot_distances_.insert(pivot_distances_.begin(), g_->max_vertex_num(), dist);

	/*supports and counts*/
	list<int> mylist;
	acc_supports_.insert(acc_supports_.begin(), g_->max_vertex_num(), mylist);
	acc_counts_.insert(acc_counts_.begin(), g_->max_vertex_num(), 0);

	/*init the temp use for first pruning*/
	PivotDemension pivot_demension;
	pivot_dimensions_.insert(pivot_dimensions_.begin(), pivot_num_, pivot_demension);
	acc_bitmap_.insert(acc_bitmap_.begin(), g_->max_vertex_num(), 0);
}

ERR AstarAC::ChoosePivots(int strategy){
	int s = rand() % g_->max_vertex_num();
	set<int> landmarks_set;
	vector<int> rand_range;

	for (int i = 0; i < g_->max_vertex_num(); i++){
		rand_range.push_back(i);
	}
	MyRandRange(&rand_range, g_->max_vertex_num());

	if (AAC_RANDOM_SELECT == strategy){
		pivots_.clear();
		pivots_.insert(pivots_.begin(), rand_range.begin(), rand_range.begin() + pivot_num_);
	}
#if 0
	else if (FASTEST_SELECT == strategy){

		auto fun = [](int s, int dir, Vertex *u, int argc, void **argv){
			int *farest_vertex = (int*)argv[0];
			int *max_dist = (int*)argv[1];
			if (u->dist == kInfinity)
				return -1;
			if (u->dist > *max_dist){
				*farest_vertex = u->index;
				*max_dist = u->dist;
			}
			return 0;
		};

		s = rand_range[0];
		int i = 1;
		while (true){
			int argc = 2;
			int farest_vertex = -1;
			int max_dist = 0;
			void *argv[] = { &farest_vertex, &max_dist };

			g_->Dijkstra(s, 0, fun, argc, argv);
			cout << "rand vertex: " << s << " fastest vertex: " << farest_vertex << " maxdist: " << max_dist << endl;

			if (landmarks_set.find(farest_vertex) == landmarks_set.end()){
				landmarks_set.insert(farest_vertex);
				s = farest_vertex;
			}
			else{
				s = rand_range[i++];
			}
			cout << "rand " << i << "   size " << landmarks_set.size() << endl;
			if (num <= (int)landmarks_set.size())
				break;
		}



		pivots_.clear();
		pivots_.insert(pivots_.begin(), landmarks_set.begin(), landmarks_set.end());
	}
#endif
	return 0;
}

ERR AstarAC::ConstructPivotsDist(){
	auto fun = [](int s, int dir, Vertex *u, int argc, void **argv){
		PivotDistances *dists = (PivotDistances*)argv[0];
		int *i = (int*)argv[1];
		dists->at(u->index).at(*i) = u->dist;
		return 0;
	};

	for (int i = 0; i < pivot_num_; i++){
		int pivot = pivots_[i];
		int argc = 2;
		void *argv[] = { &pivot_distances_, &i };
		g_->Dijkstra(pivot, 0, fun, argc, argv);
	}
	return 0;
}

ERR AstarAC::WritePivotsDists(const char *filename, const char *delim){
	int error;
	FILE *fp;
	string strbuf;
	char buf[32];

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	for (int i = 0; i < (int)pivot_distances_.size(); i++)
	{
		int vertex = i;
		strbuf = _itoa(i, buf, 10); //vertex ID
		auto dists = &pivot_distances_[i];
		for (auto it = dists->begin(); it != dists->end(); it++)
		{
			// vertexID, landm value 1, 2 3, 
			strbuf += delim;
			int dist;
			if (*it == kInfinity)
				dist = -1;
			else
				dist = *it;
			strbuf += _itoa(dist, buf, 10);
		}
		strbuf += "\n";
		fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
	}

	fclose(fp);
	return 0;

}

ERR AstarAC::ReadPivotsDists(const char *filename, const char* delim){
	int error;
	FILE *fp;
	int kBufSize = 256;
	char *strbuf = new char[kBufSize];
	char *p;

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, kBufSize, fp) != NULL)
	{
		if (2 >= strlen(strbuf))
			continue;
		if (strbuf[0] == IGNORE_CUR_LINE)
			continue;
		if (strbuf[0] == IGNORE_BELOW_LINE)
			break;

		p = strtok(strbuf, delim); /*change the strbuf pointer.*/
		if (NULL == p)
			continue;
		int vertex = atoi(p);
		for (int i = 0; i < pivot_num_ && (p = strtok(NULL, delim)) != NULL; i++){
			int value = atoi(p);
			if (-1 == value)
				value = kInfinity;
			pivot_distances_[vertex][i] = value;
		}
	}

	fclose(fp);
	delete[](strbuf);
	return 0;
}

int AstarAC::EstimatDist(int u2, int u1){
	int value = 0, new_value = 0;

	for (int i = 0; i < pivot_num_; i++){
		new_value = abs(pivot_distances_[u1][i] - pivot_distances_[u2][i]);
		if (new_value > value)
			value = new_value;
	}

	return value;
}
/*
*return:bool , false: no pattern match result. true: some pattern match result.
*		total: total visited vertices.
*		total_delete: total delete vertices.
*/
pair<int, int> AstarAC::FirstPrune(){
	list<int> *group_items[2];
	int total = 0;
	int total_delete = 0;

	auto sort_functin = [](pair<int, int> a, pair<int, int> b){
		return a.second < b.second;
	};

	
	/*build support by previous buckets.*/
	for (auto it = query_->label_pairs()->begin(); it != query_->label_pairs()->end(); it++){
		auto label_pair = *it;
		int label[2] = { label_pair.first, label_pair.second };
		group_items[0] = group_->items(label[0]);
		group_items[1] = group_->items(label[1]);

		FOR_EACH_DIRECTION;
		/*init demensions and bitmap*/
		for (int p = 0; p < pivot_num_; p++){
			pivot_dimensions_[p].clear();
			pivot_dimensions_[p].reserve(group_items[(dir)]->size());
			for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();it++){
				int u1 = *it;
				pivot_dimensions_[p].push_back({ u1, pivot_distances_[u1][p] });
			}
			sort(pivot_dimensions_[p].begin(), pivot_dimensions_[p].end(), sort_functin);
		}

	
		/*pruning start. note that we should start from the new visited vertex.*/
		for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
			
			int u1 = *it;
			bool has_suport = false;
			int sub_total = 0;
			/*init bitmap for intersection operation*/
			//fill(acc_bitmap_.begin(), acc_bitmap_.end(), 0);
			memset(&acc_bitmap_[0],0,sizeof(acc_bitmap_[0])* acc_bitmap_.size());

			for (int p = 0; p < pivot_num_; p++){
				int dist = pivot_distances_[u1][p];
				int range_start = max(0, dist - delta_);
				int range_end = dist + delta_;
				//PivotDemension::iterator low;
				auto low = lower_bound(pivot_dimensions_[p].begin(), pivot_dimensions_[p].end(), make_pair(1, range_start), sort_functin);
				int pos = low - pivot_dimensions_[p].begin();
				int size = pivot_dimensions_[p].size();

				if (pos < size)
				{
					while (pivot_dimensions_[p][pos].second <= range_end){
						sub_total++;
						int u = pivot_dimensions_[p][pos].first;
						acc_bitmap_[u]++;
						if (pivot_num_ == acc_bitmap_[u]){
							acc_supports_[u1].push_back(u);
							has_suport = true;
						}

						pos++;
						if (pos >= size)
							break;
					}
				}
			}

			total += sub_total / pivot_num_;
			if (false == has_suport){
				group_items[OPP(dir)]->erase(it++);
				total_delete++;
				/*put into delete-list*/
				if (false == acc_supports_[u1].empty())
					queue_.push(u1);
			}
			else
			{
				it++;
			}
		}

		/*if this group is empty, no match result is exist.*/
		if (group_items[OPP(dir)]->empty()){
			//return false;
		}
		END_FOR_EACH_DIRCTION;
	}

	
	return{ total, total_delete};
}

pair<int, int> AstarAC::EmbeddingPrune(Embedding *embedding){
	list<int> *group_items[2];
	int total = 0;
	int total_delete = 0;
	bool flag;
	bool has_one;

	flag = false;
	/*build support by previous buckets.*/
	for (auto it = query_->label_pairs()->begin(); it != query_->label_pairs()->end(); it++){
		auto label_pair = *it;
		int label[2] = { label_pair.first, label_pair.second };
		group_items[0] = group_->items(label[0]);
		group_items[1] = group_->items(label[1]);

		FOR_EACH_DIRECTION;
		for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){
			int u1 = *it;
			has_one = false;
			for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
				int u2 = *it2;
				total++;
				if (embedding->CalculateL(u1, u2) <= delta_){
					has_one = true;
				}

			}
			if (false == has_one){
				group_items[(dir)]->erase(it++);
				total_delete++;
			}
			else{
				it++;
			}

		}
		END_FOR_EACH_DIRCTION;
	}

	return{total, total_delete};
}

#if 0
ERR AstarAC::AStar(int s, int label){
	auto dikstrafun = [](int s, int dir, Vertex *u, int argc, void **argv){
		int *delta = (int *)argv[0];
		if (u->dist > *delta){
			return -1;
		}
		return 0;
	};

	auto heuristicfun = [](int dir, Vertex *u, int argc, void **argv){
		int value = 0, new_value = 0;
		LandmarkDists* l_vertex_dists_ = (LandmarkDists*)argv[0];
		LandmarkDists* l_group_min_dists_ = (LandmarkDists*)argv[1];
		LandmarkDists* l_group_max_dists_ = (LandmarkDists*)argv[2];
		int landmark_num = *(int*)argv[3];
		int label = *(int*)argv[4];

		int v1 = u->index;
		int v2 = label;
		for (int i = 0; i < landmark_num; i++){
			int value1 = abs((*l_vertex_dists_)[v1][i] - (*l_group_min_dists_)[v2][i]);
			int value2 = abs((*l_vertex_dists_)[v1][i] - (*l_group_max_dists_)[v2][i]);
			new_value = max(value1, value2); /*find the farest node in the cluster not the nearest one.*/
			if (new_value > value)
				value = new_value;
		}
		return value;
	};

	int argc = 1;
	void *argv[] = { &delta_ };
	int argc2 = 5;
	void *argv2[] = { &l_vertex_dists_, &l_group_min_dists_, &l_group_max_dists_, &landmark_num_, &label };
	g_->AStar(s, 0, dikstrafun, argc, argv, heuristicfun, argc2, argv2);
	return 0;
}
#endif
