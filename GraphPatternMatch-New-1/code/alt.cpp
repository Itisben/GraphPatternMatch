#include "alt.h"

AltGroup::AltGroup(Graph *g){
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

Alt::Alt(Graph *g, AltGroup *group, Query *query, int landmark_num){
	g_ = g;
	group_ = group;
	query_ = query;
	delta_ = query->delta();
	landmark_num_ = landmark_num;

	/*for vertex*/
	vector<int> dist;
	dist.insert(dist.begin(), landmark_num_, kInfinity);
	l_vertex_dists_.insert(l_vertex_dists_.begin(), g_->max_vertex_num(), dist);

	/*for group*/
	dist.clear();
	dist.insert(dist.begin(), landmark_num_, kInfinity);
	//l_group_min_dists_.insert(l_group_min_dists_.begin(), group_->label_num(), dist);
	//l_group_max_dists_.insert(l_group_max_dists_.begin(), group_->label_num(), dist);
}

ERR Alt::ChooseLandmark(int num, int strategy){
	landmark_num_ = num;
	int s = rand() % g_->max_vertex_num();
	set<int> landmarks_set;
	vector<int> rand_range;
	
	for (int i = 0; i < g_->max_vertex_num(); i++){
		rand_range.push_back(i);
	}
	MyRandRange(&rand_range, g_->max_vertex_num());

	if (RANDOM_SELECT == strategy){
		landmarks_.clear();
		landmarks_.insert(landmarks_.begin(), rand_range.begin(), rand_range.begin() + num);
	}
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
			
		
	
		landmarks_.clear();
		landmarks_.insert(landmarks_.begin(), landmarks_set.begin(), landmarks_set.end());
	}
	
	return 0;
}

ERR Alt::ConstructLandmarkDist(){
	auto fun = [](int s, int dir, Vertex *u, int argc, void **argv){
		LandmarkDists *dists = (LandmarkDists*)argv[0];
		int *i = (int*)argv[1];
		dists->at(u->index).at(*i) = u->dist;
		return 0;
	};

	for (int i = 0; i < landmark_num_; i++){
		int landmark = landmarks_[i];
		int argc = 2;
		void *argv[] = { &l_vertex_dists_, &i};
		g_->Dijkstra(landmark, 0, fun, argc, argv);
	}
	return 0;
}

ERR Alt::WriteLandmark(const char *filename, const char *delim){
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

	for (int i = 0; i < (int)l_vertex_dists_.size(); i++)
	{	
		int vertex = i;
		strbuf = _itoa(i, buf, 10); //vertex ID
		auto dists = &l_vertex_dists_[i];
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

ERR Alt::ReadLandmark(const char *filename, const char* delim){
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
		for (int i = 0; (p = strtok(NULL, delim)) != NULL; i++){
			int value = atoi(p);
			if (-1 == value)
				value = kInfinity;
			l_vertex_dists_[vertex][i] = value;
		}
	}

	fclose(fp);
	delete[](strbuf);
	return 0;
}

#if 0
ERR Alt::UpdateLandmarkGroupDist(bool is_in_query){
	int max, min;
	if (is_in_query == false){
		for (int label = 0; label < group_->label_num(); label++){
			for (int landmark_index = 0; landmark_index < landmark_num(); landmark_index++){
				l_group_min_dists_[label][landmark_index] = kInfinity;
				l_group_max_dists_[label][landmark_index] = kInfinity;

				max = 0;
				min = kInfinity;
				auto item = group_->items(label);
				if (false == item->empty()){
					for (auto it = item->begin(); it != item->end(); it++){
						int value = l_vertex_dists_[*it][landmark_index];
						if (value > max)
							max = value;
						if (value < min)
							min = value;
					}
					l_group_min_dists_[label][landmark_index] = min;
					l_group_max_dists_[label][landmark_index] = max;
				}
			}
		}
	}
	else{
		for (auto it = query_->label_set()->begin(); it != query_->label_set()->end(); it++){
			int label = *it;
			for (int landmark_index = 0; landmark_index < landmark_num(); landmark_index++){
				l_group_min_dists_[label][landmark_index] = kInfinity;
				l_group_max_dists_[label][landmark_index] = kInfinity;

				max = 0;
				min = kInfinity;
				auto item = group_->items(label);
				if (false == item->empty()){
					for (auto it = item->begin(); it != item->end(); it++){
						int value = l_vertex_dists_[*it][landmark_index];
						if (value > max)
							max = value;
						if (value < min)
							min = value;
					}
					l_group_min_dists_[label][landmark_index] = min;
					l_group_max_dists_[label][landmark_index] = max;
				}
			}
		}
	}
	return 0;
}
#endif

int Alt::EstimatDist(int u2, int u1){
	int value = 0, new_value = 0;

	for (int i = 0; i < landmark_num_; i++){
		new_value = abs(l_vertex_dists_[u1][i] - l_vertex_dists_[u2][i]);
		if (new_value > value)
			value = new_value;
	}

	return value;
}

int Alt::EstimatDistToGroup(int u, vector<vector<int>> *area_dists){
	int value = 0, new_value = 0;
	

	for (int i = 0; i < landmark_num_; i++){
		new_value = BinarySearch(l_vertex_dists_[u][i], &area_dists->at(i));
		if (new_value > value)
			value = new_value;
	}

	return value;

}

int Alt::BinarySearch(int d, vector<int> *area){
	int begin = 0, end = area->size() - 1;
	int mid;

	int value2 = *area->rbegin();
	int value1 = *area->begin();
	if (d >= value2)
		return abs(d - value2);
	if (d <= value1)
		return abs(d - value1);

	while (begin < end){
		mid = (begin + end) / 2;
		
		if (d == area->at(mid)){
			return 0;
		}
		else if (d < area->at(mid)){
			end = mid - 1;
		}
		else{
			begin = mid+1;
		}
	}
	int value = area->at(begin);
	if (d > value){
		return min(abs(value - d), abs(area->at(begin + 1) - d));
	}
	else{
		return min(abs(value - d), abs(area->at(begin - 1) - d));
	}
}

int Alt::FirstPrune(){
	list<int> *group_items[2];
	int total = 0;
	bool flag;
BEGIN:
	flag = false;
	/*build support by previous buckets.*/
	for (auto it = query_->label_pairs()->begin(); it != query_->label_pairs()->end(); it++){
		auto label_pair = *it;
		int label[2] = { label_pair.first, label_pair.second };
		group_items[0] = group_->items(label[0]);
		group_items[1] = group_->items(label[1]);

		FOR_EACH_DIRECTION;
#if 0
		vector<vector<int>> area_dists; //landmark, dist
		vector<int> temp;
		area_dists.insert(area_dists.begin(), landmark_num_, temp);
		for (int i = 0; i < landmark_num_; i++){
			area_dists[i].reserve(group_items[OPP(dir)]->size());
			for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
				int vertex = *it;
				area_dists[i].push_back(l_vertex_dists_[vertex][i]);
			}
			sort(area_dists[i].begin(), area_dists[i].end());
		}

		for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){
			int v = *it;
			int e_dist = EstimatDistToGroup(v, &area_dists);
			//cout << v << " to label " << label[OPP(dir)] << " dist "<< e_dist << endl;
			if (e_dist > delta_){
				group_items[(dir)]->erase(it++);
				total++;
				flag = true;
			}
			else{
				it++;
			}
		}
#else
		for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){
			int u1 = *it;
			bool has = false;
			for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
				int u2 = *it2;
				//cout << v << " to label " << label[OPP(dir)] << " dist "<< e_dist << endl;
				int e_dist = EstimatDist(u1, u2);
				if (e_dist <= delta_){
					has = true;
					break;
				}		
			}

			if (false == has){
				group_items[(dir)]->erase(it++);
				total++;
				flag = true;
			}
			else
			{
				it++;
			}
		}
#endif
		END_FOR_EACH_DIRCTION;
	}

	if (true == flag)
		goto BEGIN;
	return total;
}
int Alt::EmbeddingPrune(Embedding *embedding){
	list<int> *group_items[2];
	int total = 0;
	bool flag;
	bool has_one;
BEGIN:
	flag = false;
	/*build support by previous buckets.*/
	for (auto it = query_->label_pairs()->begin(); it != query_->label_pairs()->end(); it++){
		auto label_pair = *it;
		int label[2] = { label_pair.first, label_pair.second };
		group_items[0] = group_->items(label[0]);
		group_items[1] = group_->items(label[1]);

		FOR_EACH_DIRECTION;
		for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end(); it++){
			int u1 = *it;
			has_one = false;
			for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
				int u2 = *it2;
				if (embedding->CalculateL(u1, u2) <= delta_){
					has_one = true;
				}
				
			}
			if (has_one == false){
				group_items[(dir)]->erase(it++);
				total++;
			}
			else{
				it++;
			}
				
		}
		END_FOR_EACH_DIRCTION;
	}

	//if (true == flag)
		//goto BEGIN;
	return total;
}
#if 0
ERR Alt::AStar(int s, int label){
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
	void *argv2[] = { &l_vertex_dists_, &l_group_min_dists_, &l_group_max_dists_, &landmark_num_ , &label};
	g_->AStar(s, 0, dikstrafun, argc, argv, heuristicfun, argc2, argv2);
	return 0;
}
#endif
int HeuristicFuntion(){
	return 0;
}
ERR ArcConsistency(){
	return 0;
}