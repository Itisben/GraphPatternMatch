#include "arc-consistency.h"
#include <Windows.h>
#include "main.h"

ArcConsistency::ArcConsistency(Graph *g, Query *q, Group *group){
	g_ = g;
	q_ = q;
	group_ = group;
	label_num_ = g->label_num();
	delta_ = q->delta();

	//init the vetex supports list and support count
	SupportList support_list;
	FOR_EACH_DIRECTION;
	support_lists_[dir].insert(support_lists_[dir].begin(), g->vertices()->size(), support_list);
	support_counts_[dir].insert(support_counts_[dir].begin(), g->vertices()->size(), 0);
	END_FOR_EACH_DIRCTION;

	
}


int ArcConsistency::DeletedPruning(){
	int count = 0;
	SupportList *s[2];
	
	while (deleted_queue_.size() > 0){
		int u = deleted_queue_.front();
		
		deleted_queue_.pop();

		s[0] = &support_lists_[0][u];
		s[1] = &support_lists_[1][u];
	
		FOR_EACH_DIRECTION;
		for (auto it = s[dir]->begin(); it != s[dir]->end(); it++){
			support_counts_[OPP(dir)][it->vertex]--;
			if (0 == support_counts_[OPP(dir)][it->vertex]){
				group_->items(it->group_label)->erase(it->vertex);
				count++; 
				deleted_queue_.push(it->vertex);
			}
		}
		END_FOR_EACH_DIRCTION;
	}

	return count;
}

int ArcConsistency::SupportPruning(){
	int count = 0;
	/*clear the supports which is not really exist.*/
	int dir = 0; /* only for out because we only need out for construct result. */
	for (auto it = support_lists_[dir].begin(); it != support_lists_[dir].end(); it++){
		SupportList *support_list = &*it;
		if (0 == support_list->size())
			continue;
		for (auto it2 = support_list->begin(); it2 != support_list->end();){
			int vertex = it2->vertex;
			int label = it2->group_label;
			auto item = group_->items()->at(label);
			if (item.find(vertex) == item.end()){
				support_list->erase(it2++);
				count++;
			}
			else{
				it2++;
			}
		}
	}

	return count;
}

int ArcConsistency::TwohopArcPruning(Twohop *twohop){
	GroupItems *mygroup[2];
	list<array<int, 2>> arc_map_list;
	int label[2];

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		label[0] = it->first;
		label[1] = it->second;
		mygroup[0] = group_->items(label[0]);
		mygroup[1] = group_->items(label[1]);
		//key: vertex_index, array[0] center, array[1] dist, ,
		map<int, list<array<int, 2>>> arc_map; 
		set<int> right_set;

		/*for left pruning, build arc map for right*/
		int dir = 0;
		for (auto it1 = mygroup[OPP(dir)]->begin(); it1!= mygroup[OPP(dir)]->end(); it1++){
			int center = *it1;
			auto items = &twohop->clusters(center)->items[OPP(dir)];
			for (auto it2 = items->begin(); it2 != items->end(); it2++){
				int vertex = it2->first;
				int dist = it2->second;

				if (dist > delta_)
					continue;

				if (arc_map.find(vertex) != arc_map.end()){
					arc_map.insert({vertex, arc_map_list});
				}
				array<int, 2> a = { center, dist };
				arc_map[vertex].push_back(a);
			}
		}
		
		/*build support for left*/
		for (auto it1 = mygroup[(dir)]->begin(); it1 != mygroup[(dir)]->end();){
			int center = *it1;
			auto item = &twohop->clusters(center)->items[(dir)];
			set<int> support_set; //key: center in the arc_map
			for (auto it2 = item->begin(); it2 != item->end(); it2++){
				int vertex = it2->first;
				int dist = it2->second;
				if (dist > delta_)
					continue;

				/*find the candidata support and insert into support_map*/
				for (auto it3 = arc_map[vertex].begin(); it3 != arc_map[vertex].end(); it3++){
					int map_center = (*it3)[0];
					int map_dist = (*it3)[1];
					if (map_dist + dist <= delta_){ //find the support in the dist
						support_set.insert(map_center);
					}
				}
			}

			/*if no support for left, delete this center*/
			if (0 == support_set.size()){
				mygroup[dir]->erase(it1++);
				deleted_queue_.push(center);
				continue;
			}
			else{
				it1++;
			}
			/*build support*/
			for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
				int vertex = *it4;
				/*left*/
				Support support_left = { label[OPP(dir)], vertex };
				support_lists_[dir][center].push_back(support_left);
				support_counts_[dir][center]++;
				/*right, just opposite*/
				Support support_right = { label[dir], center };
				support_lists_[OPP(dir)][vertex].push_back(support_right);
				right_set.insert(vertex);
				support_counts_[OPP(dir)][vertex]++;
			}
		}

		/*fine the right side one with no support and delete it*/
		for (auto it1 = mygroup[OPP(dir)]->begin(); it1 != mygroup[OPP(dir)]->end();){
			int vertex = *it1;
			if (right_set.find(vertex) == right_set.end()){
				mygroup[OPP(dir)]->erase(it1++);
				deleted_queue_.push(vertex);
			}
			else{
				it1++;
			}
		}
	}

	DeletedPruning();
	//SupportPruning();
	return 0;
}

int ArcConsistency::TwohopArcPruning2(Twohop *twohop){
	GroupItems *mygroup[2];
	list<array<int, 2>> arc_map_list;
	int label[2];

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		label[0] = it->first;
		label[1] = it->second;
		mygroup[0] = group_->items(label[0]);
		mygroup[1] = group_->items(label[1]);
		//key: vertex_index, array[0] center, array[1] dist, ,
		map<int, list<array<int, 2>>> arc_map;
		set<int> right_set;

		/*for left pruning, build arc map for right*/
		FOR_EACH_DIRECTION;
		for (auto it1 = mygroup[OPP(dir)]->begin(); it1 != mygroup[OPP(dir)]->end(); it1++){
			int center = *it1;
			auto items = &twohop->clusters(center)->items[OPP(dir)];
			for (auto it2 = items->begin(); it2 != items->end(); it2++){
				int vertex = it2->first;
				int dist = it2->second;

				if (dist > delta_)
					continue;

				if (arc_map.find(vertex) != arc_map.end()){
					arc_map.insert({ vertex, arc_map_list });
				}
				array<int, 2> a = { center, dist };
				arc_map[vertex].push_back(a);
			}
		}

		/*build support for left*/
		for (auto it1 = mygroup[(dir)]->begin(); it1 != mygroup[(dir)]->end();){
			int center = *it1;
			auto item = &twohop->clusters(center)->items[(dir)];
			set<int> support_set; //key: center in the arc_map
			for (auto it2 = item->begin(); it2 != item->end(); it2++){
				int vertex = it2->first;
				int dist = it2->second;
				if (dist > delta_)
					continue;

				/*find the candidata support and insert into support_map*/
				for (auto it3 = arc_map[vertex].begin(); it3 != arc_map[vertex].end(); it3++){
					int map_center = (*it3)[0];
					int map_dist = (*it3)[1];
					if (map_dist + dist <= delta_){ //find the support in the dist
						support_set.insert(map_center);
					}
				}
			}

			/*if no support for left, delete this center*/
			if (0 == support_set.size()){
				mygroup[dir]->erase(it1++);
				deleted_queue_.push(center);
				continue;
			}
			else{
				it1++;
			}
			/*build support*/
			for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
				int vertex = *it4;
				/*left*/
				Support support_left = { label[OPP(dir)], vertex };
				support_lists_[dir][center].push_back(support_left);
				support_counts_[dir][center]++;
			}
		}
		END_FOR_EACH_DIRCTION
	}

	DeletedPruning();
	return 0;
}

MatchResult *ArcConsistency::GetMatchResult(){
	list<array<int, 3>> dfs_tree_edges; /*vertex left, vertex right, is_root(1 or 0)*/
	list<array<int, 2>> other_edges; //except dfs tree edges
	MatchResult match_result;
	stack<int> ac_stack;

	stack<Vertex*> stack;
	int time = 0;
	Vertex *u, *v;

	/*topo sort first*/
	auto topo_sort_v = q_->TopoSort();

	/*dfs with topo order, find all dfs tree edges.*/
	for (auto it = q_->vertices()->begin(); it != q_->vertices()->end(); it++)
	{
		it->pai = -1;
		it->d = 0;
		it->f = 0;
		it->color = kWhite;
		if (false == it->exist)
			it->color = kColorNotExist;
	}

	for (auto it_u = topo_sort_v->begin(); it_u != topo_sort_v->end(); it_u++)
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
				if (-1 != u->pai){
					int is_root = 0;
					if (1 == stack.size())
						is_root = 1;
					array<int, 3> a = { u->pai, u->index, is_root };
					dfs_tree_edges.push_back(a);
					
				}
			}

			/*we can optimize here for avoiding the iteration.*/
			for (auto it_v = u->edges.begin(); it_v != u->edges.end(); it_v++)
			{
				v = &q_->vertices()->at(it_v->vertex);
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
	
	/**/
	match_result.insert(match_result.begin(), q_->max_vertex_num(), -1);
	for (auto it = dfs_tree_edges.begin(); it != dfs_tree_edges.end(); it++){
		int vertex[2] = { it->at(0), it->at(1) };
		SupportList *support_list[2] = { support_lists(vertex[0]), support_lists(vertex[1]) };
		int is_root = it->at(2);
		if (1 == is_root){
			for (auto it2 = support_list[0]->begin(); it2 != support_list[0]->end(); it2++){
				//ac_stack.push()
			}
			
		}
	}

	return NULL;
}

int NeighborAreaPruningOne(LabelPair *pair, Embedding *emb){
	int label[2] = { pair->first, pair->second };
	Rk *rk[2] = { emb->rks(label[0]), emb->rks(label[1]) };

	FOR_EACH_DIRECTION;

	END_FOR_EACH_DIRCTION;
	return 0;
}
/**/
#if 0
int ArcConsistency::EmbeddingACPruning(Embedding *embedding, bool with_support){
	Group *group[2];
	vector<pair<int, int>> ranges;
	pair<int, int> range = {0, 0};

	for (int i = 0; i < embedding->k_sub(); i++){
		ranges.push_back(range);
	}

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		group[0] = &groups_[it->first];
		group[1] = &groups_[it->second];

		FOR_EACH_DIRECTION;
		/*get right group range*/
		for (auto it = group[OPP(dir)]->begin(); it != group[OPP(dir)]->end(); it++){
			int vertex = *it;
			for (int i = 0; i < embedding->k(); i++){
				int I = embedding->rk(vertex)->at(i);
				if (I < ranges[i].first)
					ranges[i].first = I;
				if (I > ranges[i].second)
					ranges[i].second = I;
			}
		}
		/*for left pruning*/
		for (auto it = group[(dir)]->begin(); it != group[(dir)]->end(); it++){
			int vertex = *it;
			int dist = 0;
			for (int i = 0; i < embedding->k(); i++){
				int I = embedding->rk(vertex)->at(i);
				int min_item_dist = min(abs(ranges[i].first - I), abs(ranges[i].second - I));
				dist = max(min_item_dist, dist);
				if (dist > delta_)
				{
					group[dir]->erase(vertex);
					deleted_queue_.push(vertex);
					break;
				}
			}	
		}
		END_FOR_EACH_DIRCTION

	}
	DeletedPruning();
	return 0;
}

int ArcConsistency::EmbeddingACPruningByPair(Group *group[2]){
	
	return 0;
}
#endif

int ArcConsistency::write(const char *filename, char* delim){
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

	for (int label = 0; label < group_->label_num(); label++)
	{
		auto items = group_->items(label);
		for (auto it = items->begin(); it != items->end(); it++)
		{
			int center = *it;

			auto support_out = support_lists_[0][center];
			auto support_in = support_lists_[1][center];
			if (0 == support_out.size() && 0 == support_in.size())
				continue;
			// label, center, right support
			strbuf = _itoa(label, buf, 10); //label
			strbuf += delim;
			strbuf += _itoa(center, buf, 10); //center
			
			int c = support_counts_[0][center];
			strbuf += delim;
			strbuf += _itoa(c, buf, 10); //center
			for (auto it2 = support_out.begin(); it2 != support_out.end(); it2++){
				strbuf += delim;
				strbuf += "[";
				strbuf += _itoa(it2->group_label, buf, 10); //support label
				strbuf += "-";
				strbuf += _itoa(it2->vertex, buf, 10); //support vertex
				strbuf += "]";
			}

			c = support_counts_[1][center];
			strbuf += delim;
			strbuf += _itoa(c, buf, 10); //center
			for (auto it2 = support_in.begin(); it2 != support_in.end(); it2++){
				strbuf += delim;
				strbuf += "(";
				strbuf += _itoa(it2->group_label, buf, 10); //support label
				strbuf += "-";
				strbuf += _itoa(it2->vertex, buf, 10); //support vertex
				strbuf += ")";
			}

			strbuf += "\n";
			fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
			
		}

	}

	fclose(fp);
	return 0;
}

int ArcConsistency::NeighborAreaPruning(){
	int count = 0;
	bool is_change;
	int my_loop_num = 0;
	int my_delete_num = 0;
	Areas areas;
	Area area;
	AreaI area_I;
	area.insert(area.begin(), embedding_->k(), area_I); /*for k demetion*/
	areas.insert(areas.begin(), g_->label_num(), area); /*for every label*/

	/*scan each Ri to find the area(Ri)*/
	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		auto label = *it;
		auto group_item = group_->items(label);
		for (int I = 0; I < embedding_->k(); I++) {
			AreaI area_I, area_I_new;
			/*buile area_I*/
			for (auto it2 = group_item->begin(); it2 != group_item->end(); it2++){
				int vertex = *it2;
				auto rk = embedding_->rks(vertex);
				area_I.insert({ max((*rk)[I] - delta_, 0), (*rk)[I] + delta_ });
			}
			/*combine area_I if necessary, int the area_I sorted by first.*/
			pair<int, int> pre = { -1, -1 }, cur;
			for (auto it2 = area_I.begin(); it2 != area_I.end(); it2++){
				if (pre.first == -1){
					pre = *it2;
					area_I_new.insert(*it2);
					continue;
				}
				cur = *it2;
				if (cur.first <= pre.second){
					if (cur.second > pre.second){
						/*cross, combine together*/
						area_I_new.erase(pre);
						area_I_new.insert({ pre.first, cur.second });
						pre = { pre.first, cur.second };
					}
					else {
						/*include do nothing*/
					}
				}
				else{
					/*not include*/
					area_I_new.insert(cur);
					pre = cur;
				}
			}
			areas[label][I] = area_I_new;
		}

	}

	/*scan Ri to filter*/
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		int label[2] = { it->first, it->second };
		GroupItems *group_item[2] = { group_->items(label[0]), group_->items(label[1]) };

		FOR_EACH_DIRECTION;
		for (auto it2 = group_item[dir]->begin(); it2 != group_item[dir]->end();){
			int vertex = *it2;
			auto rk = embedding_->rks(vertex);
			Area *area = &areas[label[OPP(dir)]];

			if (false == IsInNeighborArea(area, rk, embedding_->k())){
				group_item[dir]->erase(it2++);
				my_delete_num++;
				is_change = true;
			}
			else{
				it2++;
			}
		}
		END_FOR_EACH_DIRCTION;
	}

	return my_delete_num;
}

bool ArcConsistency::IsInNeighborArea(Area *area, Rk *rk, int k){
	for (int I = 0; I < k; I++){
		int d = (*rk)[I];
		AreaI * areai = &(*area)[I];
		bool in_areai = false;
		for (auto it = areai->begin(); it != areai->end(); it++){
			if (d >= it->first && d <= it->second){
				in_areai = true;
				break;
			}
		}
		if (false == in_areai)
			return false;
	}
	return true;
}

ERR ArcConsistency::EmbeddingArcPruning(){
	DWORD start_time, end_time;
	int count;
	start_time = GetTickCount();
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		auto label_pair = *it;
#ifdef DEBUG_AC_EMBEDDING_NEW
		EmbeddingArcPruningByGroup2(label_pair);
#else
		EmbeddingArcPruningByGroup(label_pair);
#endif
		//end_time = GetTickCount();
		//COUT(<< " EmbeddingArcPruning " << it->first << " " << it->second << " " << end_time - start_time << " ms" << endl;)
	}
	//end_time = GetTickCount();
	//COUT(<< " EmbeddingArcPruning end. start  DeletedPruning..." << " " << end_time - start_time << " ms" << endl;)
	EmbeddingArcPruningVerify();
	//count = DeletedPruning();
	//end_time = GetTickCount();
	//COUT(<< " DeletedPruning end. "<< count << " start SupportPruning..." << " " << end_time - start_time << " ms" << endl;)
#if 0
	count = SupportPruning();
	end_time = GetTickCount();
	COUT(<< " support pruning end " << count <<" " << end_time - start_time << " ms" << endl;)
#endif
	return 0;
}

ERR ArcConsistency::EmbeddingArcPruningByGroup(LabelPair label_pair){
	int bucket_size;
	int emb_k = embedding_->k();

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	int dir = 0;
	/*init*/
	int I_max = 0; /*find the max I value*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}

	bucket_size = I_max / delta_ + 1;
	Bucket bucket;
	Buckets buckets(bucket_size, bucket); /*every demention has bucket_size bucket*/
	BucketsK buckets_k(emb_k, buckets); /*k-demention*/
	set<int> right_set;

	/*build buckets*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it; //right
		for (int I = 0; I < emb_k; I++){
			/*put q in to the 3 buckets if I can find 3 buckets*/
			int I_value = embedding_->rks(q)->at(I);
			int bucket_index = I_value / delta_;

			int start = max(bucket_index - 1, 0); /*find 3 buckets*/
			int end = min(bucket_index + 1, bucket_size - 1); /* 0 to bucket_size - 1*/
			for (int bucket = start; bucket <= end; bucket++)
				buckets_k[I][bucket].insert(q);
		}
	}

	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){
		int p = *it; //left
		Cans cans; /*put all qualified bucket pointers*/

		/*build can for p*/
		Rk *rk = embedding_->rks(p);
		for (int I = 0; I < emb_k; I++){
			int bucket_index = rk->at(I) / delta_;
			if (bucket_index < bucket_size) /*if p cannot find bucket*/
				cans.push_back(&buckets_k[I][bucket_index]);
		}

		/*find the intersection of all bucket in the cans*/
		auto fun = [](Bucket *b1, Bucket *b2){
			return b1->size() < b2->size();
		};

		/*sort for finding the shortest buckets in can*/
		sort(cans.begin(), cans.end(), fun);
		Bucket *b0 = cans[0];
		set<int> support_set;
		for (auto it3 = b0->begin(); it3 != b0->end(); it3++){
			int q = *it3;
			bool is_intersection = true;
			for (int i = 1; i < (int)cans.size(); i++){
				if (cans[i]->find(q) == cans[i]->end()){
					is_intersection = false;
					break;
				}
			}
			/*add to the support */
			if (true == is_intersection){
				support_set.insert(q);
			}

		}

		/*add support to the left*/
		if (0 == support_set.size()){
			deleted_queue_.push(p);
			group_items[(dir)]->erase(it++);
			continue;
		}

		for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
			int q = *it4;
			/*for left out*/
			Support s = { g_->get_label(q), q };
			support_lists_[dir][p].push_back(s);
			support_counts_[dir][p]++;
			/*for right in*/
			Support s2 = { g_->get_label(p), p };
			support_lists_[OPP(dir)][q].push_back(s2);
			support_counts_[OPP(dir)][q]++;
			/*build right set*/
			right_set.insert(q);
		}
		it++;
	}

	/*find the right with no support and delete it*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
		int q = *it;
		if (right_set.find(q) == right_set.end()){
			group_items[OPP(dir)]->erase(it++);
		}
		else
		{
			it++;
		}
	}
	return 0;
}

/*new for direted grpah*/
ERR ArcConsistency::EmbeddingArcPruningByGroup2(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();

	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	int dir = 0;

	//NeighborAreaPruningByGroup(label_pair);

	/*find the max I value except infinigy*/
	int I_max = 0; 
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}
	
	end_time = GetTickCount();
	COUT(<< "find the max I value except infinigy " << end_time - start_time << " ms" << endl;)

	/*init bucket size and infinity*/
	int bucket_size = I_max / delta_ + 1;
	int emb_k = embedding_->k();
	int infinity_bucket_index = bucket_size;
	Bucket bucket;
	/*every demention has bucket_size bucket, the last one is for the infinity bucket*/
	Buckets buckets(bucket_size+1, bucket); 
	BucketsK buckets_k(emb_k, buckets); /*k-demention*/

	set<int> right_set; /*for right side pruning*/

	/*build buckets*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it; //right
		for (int I = 0; I < emb_k; I++){
			/*put q in to 0 - bucket_index + 1 buckets*/
			int I_value = embedding_->rks(q)->at(I);
			if (kInfinity != I_value){
				int bucket_index = I_value / delta_;

				int end = min(bucket_index + 1, bucket_size - 1); /* can not oversize the last one*/
				for (int bucket = 0; bucket <= end; bucket++)
					buckets_k[I][bucket].insert(q);
			} else /*build infinity bucket*/
			{
				buckets_k[I][infinity_bucket_index].insert(q);
			}
		}
	}

	end_time = GetTickCount();
	COUT(<< "build buckets " << end_time - start_time << " ms"<<endl;)
	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){
		/*for debug*/
		static int build_support_count = 0;
		build_support_count++;
		static int build_support_total = 0;
		if (build_support_count <= build_support_total)
			COUT(<< build_support_count << endl;)

		int p = *it; //left
		NewCans cans; /*put all qualified bucket pointers*/
		cans.reserve(embedding_->k());

		/*build can for p*/
		Rk *rk = embedding_->rks(p);
		for (int I = 0; I < emb_k; I++){
			int I_value = rk->at(I);
			array<Bucket*, 2> bucket_array = { NULL };
			if (kInfinity != I_value){
				int bucket_index = I_value / delta_;
				if (bucket_index < bucket_size) /*just protect*/
					bucket_array[0] = &buckets_k[I][bucket_index];
			}
			bucket_array[1] = &buckets_k[I][infinity_bucket_index];
			if (NULL == bucket_array[0]){
				bucket_array[0] = &temp_bucket;//just protect, it is meaningless.
			}
			cans.push_back(bucket_array);
		}

		if (build_support_count <= build_support_total){
			end_time = GetTickCount();
			COUT(<< "	build cans " << p << " " << end_time - start_time << " ms" << endl;)
		}
		/*find the intersection of all bucket in the cans*/
		auto fun = [](array<Bucket*, 2> b1, array<Bucket*, 2> b2){
			return (b1[0]->size() + b1[1]->size()) < (b2[0]->size() + b2[1]->size());
		};

		/*sort for finding the shortest buckets in can*/
		sort(cans.begin(), cans.end(), fun);
		BucketArray b0 = cans[0];
		vector<int> support_set;

		for (int a = 0; a <= 1; a++){ //for each one in the array
			for (auto it3 = b0[a]->begin(); it3 != b0[a]->end(); it3++){
				int q = *it3;
				bool is_intersection = true;
				for (int i = 1; i < (int)cans.size(); i++){
					if (cans[i][0]->find(q) == cans[i][0]->end()
						&& cans[i][1]->find(q) == cans[i][1]->end()){
						is_intersection = false;
						break;
					}
				}
				/*add to the support */
				if (true == is_intersection){
					support_set.push_back(q);
				}

			}
		}
		
		if (build_support_count <= build_support_total){
			end_time = GetTickCount();
			COUT(<< "	find intersection " << p << " " << end_time - start_time << " ms" << endl;)
		}
		/*add support to the left*/
		if (0 == support_set.size()){
			group_items[(dir)]->erase(it++);
			deleted_queue_.push(p);
			continue;
		}
		Support s;
		for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
			int q = *it4;
			/*for left out*/
			//Support s = { g_->get_label(q), q };
			s.group_label = label_pair.second;
			s.vertex = q;
			support_lists_[dir][p].push_back(s);
			support_counts_[dir][p]++;

			/*for right in*/
			//Support s2 = { g_->get_label(p), p };
			s.group_label = label_pair.first;
			s.vertex = p;
			support_lists_[OPP(dir)][q].push_back(s);
			support_counts_[OPP(dir)][q]++;
			/*build right set*/
			right_set.insert(q);
		}
		it++;

		if (build_support_count <= build_support_total){
			end_time = GetTickCount();
			COUT(<< "	build support " << p << " " << end_time - start_time << " ms" << endl;)
		}
		
	}
	
	end_time = GetTickCount();
	COUT(<< "build support " << end_time - start_time << " ms" << endl;)

	/*find the right with no support and delete it*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
		int q = *it;
		if (right_set.find(q) == right_set.end()){
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(q);
		}
		else
		{
			it++;
		}
	}
	end_time = GetTickCount();
	COUT(<< "prun the right support " << end_time - start_time << " ms" << endl;)
	return 0;
}

int ArcConsistency::NeighborAreaPruningByGroup(LabelPair label_pair){
	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	Area area;
	AreaI area_I;
	area.insert(area.begin(), embedding_->k(), area_I); /*for k demetion*/

	/*scan each Ri to find the area(Ri)*/
	for (int I = 0; I < embedding_->k(); I++) {
		AreaI area_I, area_I_new;
		/*buile area_I*/
		for (auto it2 = group_items[1]->begin(); it2 != group_items[1]->end(); it2++){
			int vertex = *it2;
			auto rk = embedding_->rks(vertex);
			int value = (*rk)[I];
			area_I.insert({ max(value - delta_, 0), value + delta_ });
		}
		/*combine area_I if necessary, int the area_I sorted by first.*/
		pair<int, int> pre = { -1, -1 }, cur;
		for (auto it2 = area_I.begin(); it2 != area_I.end(); it2++){
			if (pre.first == -1){
				pre = *it2;
				area_I_new.insert(*it2);
				continue;
			}
			cur = *it2;
			if (cur.first <= pre.second){
				if (cur.second > pre.second){
					/*cross, combine together*/
					area_I_new.erase(pre);
					area_I_new.insert({ pre.first, cur.second });
					pre = { pre.first, cur.second };
				}
				else {
					/*include do nothing*/
				}
			}
			else{
				/*not include*/
				area_I_new.insert(cur);
				pre = cur;
			}
		}
		area[I] = area_I_new;
	}

	

	/*scan Ri to filter*/
	for (auto it2 = group_items[0]->begin(); it2 != group_items[0]->end();){
		int vertex = *it2;
		auto rk = embedding_->rks(vertex);

		if (false == IsInNeighborArea(&area, rk, embedding_->k())){
			group_items[0]->erase(it2++);
			deleted_queue_.push(vertex);
		}
		else{
			it2++;
		}
	}
	
	return 0;
}

int ArcConsistency::EmbeddingArcPruningVerify(){
	int count = 0;
	DWORD start_time, end_time;
	start_time = GetTickCount();
	/*iterate all the out support*/
	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		int label = *it;
		GroupItems *group_items = group_->items(label);
		for (auto it2 = group_items->begin(); it2 != group_items->end();){
			int s = *it2;
			SupportList *support_list = support_lists(s);
			if (0 == support_list->size()){
				it2++;
				continue;
			}
			
			int ret = EmbeddingArcPruningVerifyBySupport(s, support_list);
			if (1 == ret){
				group_items->erase(it2++);
			}
			else{
				it2++;
			}
			count++;
			end_time = GetTickCount();
			//COUT(<< "  EmbeddingArcPruningVerifyBySupport " << end_time - start_time << " ms"<< endl;)
		}
	}
	return count;
}

/*return 1, delete this*/
ERR ArcConsistency::EmbeddingArcPruningVerifyBySupport(int s, SupportList *support_list){
	/*jump over the empty support list.*/
	auto dijkstraFun = [](int s, int dir, Vertex *u, int argc, void **argv){
		set<int> *support_set = (set<int>*)argv[0];
		int delta = *(int*)argv[1];

		if (u->dist <= delta){
			support_set->erase(u->index);
		}
		else{
			return -1; //quite dijkstra
		}
		return 0;
	};
	
	/*init*/
	set<int> support_set;
	for (auto it = support_list->begin(); it != support_list->end(); it++){
		support_set.insert(it->vertex);
	}

	/*find all the qualified vertex and erase in support_set.*/
	int argc = 2;
	void *argv[2] = { &support_set, &delta_ };
	g_->Dijkstra(s, 0, dijkstraFun, argc, argv);

	/*remove unqualified supports. vertex in support_set should be removed all.*/
	for (auto it = support_list->begin(); it != support_list->end();){
		int vertex = it->vertex;
		if (support_set.find(vertex) != support_set.end()){
			support_counts_[0][s]--;
			support_counts_[1][vertex]--;
			support_list->erase(it++);
		}
		else{
			it++;
		}
	}
	return 0;
}

ERR ArcConsistency::ClearSupports(){
	//clear the support list.
	SupportList support_list;
	FOR_EACH_DIRECTION;
	for (auto it = support_lists_[dir].begin(); it != support_lists_[dir].end(); it++){
		it->clear();
	}
	END_FOR_EACH_DIRCTION;

	//reset count
	FOR_EACH_DIRECTION;
	for (auto it = support_counts_[dir].begin(); it != support_counts_[dir].end(); it++){
		*it = 0;
	}
	END_FOR_EACH_DIRCTION;
	return 0;
}
#ifndef EMBEDDING_AC
/***for EmbeddingAC************************************/
EmbeddingAC::EmbeddingAC(Graph *g, Query *q, Group *group){
	g_ = g;
	q_ = q;
	group_ = group;
	label_num_ = g->label_num();
	delta_ = q->delta();
	debug_ = 0;
	//init the vetex supports list and support count
	SupportSet support_set;
	FOR_EACH_DIRECTION;
	support_sets_[dir].insert(support_sets_[dir].begin(), g->vertices()->size(), support_set);
	END_FOR_EACH_DIRCTION;
}


ERR EmbeddingAC::EmbeddingArcPruning(int method,  bool neighbor_area_pruning, DWORD start_time){
	DWORD end_time;
	int count;

	if (true == neighbor_area_pruning){
		NeighborAreaPruning();
		end_time = GetTickCount();
		COUT(<< "==NeighborAreaPruning " << end_time - start_time << "ms" << endl;)
	}
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		auto label_pair = *it;
				
			if (HASH_AC_JOIN == method){ /*hash join*/
				EmbeddingArcPruningByGroupUndirect(label_pair);
#if 0
				if (false == g_->directed()){
					EmbeddingArcPruningByGroupUndirect(label_pair);
				}
				else
				{
					EmbeddingArcPruningByGroup(label_pair);
				}
#endif
			}
			else if (NAIVE_AC_JOIN == method){ /*naive*/
				EmbeddingArcPruningByGroup2(label_pair);
			}
		

		end_time = GetTickCount();
		COUT(<< "==EmbeddingArcPruning " << it->first << " " << it->second << " " << end_time - start_time << "ms" << endl;)
	}
	end_time = GetTickCount();
	COUT(<< "==EmbeddingArcPruning end. start  delete..." << " " << end_time - start_time << "ms" << endl;)
	
	DeletedPruning();
	end_time = GetTickCount();
	COUT(<< "==delete end. start  verification..." << " " << end_time - start_time << "ms" << endl;)

	return 0;
}


ERR EmbeddingAC::EmbeddingArcPruningByGroup(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();

	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	FOR_EACH_DIRECTION;
	if (0 == group_items[dir]->size()){
		for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); ){
			int vertex = *it;
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(vertex);
		}
		return 0;
	}
	END_FOR_EACH_DIRCTION;

	int dir = 0;
	/*find the max I value except infinigy*/
	int I_max = 0;
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}

	/*init bucket size and infinity*/
	int bucket_size = I_max / delta_ + 1;
	int emb_k = embedding_->k();
	int infinity_bucket_index = bucket_size;
	Bucket bucket;
	/*every demention has bucket_size bucket, the last one is for the infinity bucket*/
	Buckets buckets(bucket_size + 1, bucket);
	BucketsK buckets_k(emb_k, buckets); /*k-demention*/

	set<int> right_set; /*for right side pruning*/

	/*build buckets*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it; //right
		for (int I = 0; I < emb_k; I++){
			/*put q in to 0 - bucket_index + 1 buckets*/
			int I_value = embedding_->rks(q)->at(I);
			if (kInfinity != I_value){
				int bucket_index = I_value / delta_;

				int end = min(bucket_index + 1, bucket_size - 1); /* can not oversize the last one*/
				for (int bucket = 0; bucket <= end; bucket++)
					buckets_k[I][bucket].insert(q);
			}
			else /*build infinity bucket*/
			{
				buckets_k[I][infinity_bucket_index].insert(q);
			}
		}
	}

	end_time = GetTickCount();
	COUT(<< "	build buckets " << end_time - start_time << " ms" << endl;);
	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){

		int p = *it; //left
		NewCans cans; /*put all qualified bucket pointers*/
		cans.reserve(embedding_->k());

		/*build can for p*/
		Rk *rk = embedding_->rks(p);
		for (int I = 0; I < emb_k; I++){
			int I_value = rk->at(I);
			array<Bucket*, 2> bucket_array = { NULL };
			if (kInfinity != I_value){
				int bucket_index = I_value / delta_;
				if (bucket_index < bucket_size) /*just protect*/
					bucket_array[0] = &buckets_k[I][bucket_index];
			}
			bucket_array[1] = &buckets_k[I][infinity_bucket_index];
			if (NULL == bucket_array[0]){
				bucket_array[0] = &temp_bucket;//just protect, it is meaningless.
			}
			cans.push_back(bucket_array);
		}

		/*find the intersection of all bucket in the cans*/
		auto fun = [](array<Bucket*, 2> b1, array<Bucket*, 2> b2){
			return (b1[0]->size() + b1[1]->size()) < (b2[0]->size() + b2[1]->size());
		};

		/*sort for finding the shortest buckets in can*/
		sort(cans.begin(), cans.end(), fun);
		BucketArray b0 = cans[0];
		vector<int> support_set;

		for (int a = 0; a <= 1; a++){ //for each one in the array
			for (auto it3 = b0[a]->begin(); it3 != b0[a]->end(); it3++){
				int q = *it3;
				bool is_intersection = true;
#if 1
				for (int i = 1; i < (int)cans.size(); i++){
					if (cans[i][0]->find(q) == cans[i][0]->end()
						&& cans[i][1]->find(q) == cans[i][1]->end()){
						is_intersection = false;
						break;
					}
				}
#endif
				/*add to the support */
				if (true == is_intersection){
					support_set.push_back(q);
				}

			}
		}

		/*add support to the left*/
		if (0 == support_set.size()){
			group_items[(dir)]->erase(it++);
			deleted_queue_.push(p);
			continue;
		}
		for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
			int q = *it4;
			/*for left out*/
			support_sets_[dir][p].insert(q);
			/*for right in*/
			support_sets_[OPP(dir)][q].insert(p);
			/*build right set*/
			right_set.insert(q);
		}
		it++;
	}

	end_time = GetTickCount();
	COUT(<< "	build support " << end_time - start_time << " ms" << endl;);

	/*find the right with no support and delete it*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
		int q = *it;
		if (right_set.find(q) == right_set.end()){
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(q);
		}
		else
		{
			it++;
		}
	}

	end_time = GetTickCount();
	COUT(<< "	delete right side " << end_time - start_time << " ms" << endl;);

	return 0;
}

int EmbeddingAC::EmbeddingArcPruningByGroup2(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();
	int total = 0;
	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	int dir = 0;

	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end(); it++){
		int u1 = *it;
		for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
			int u2 = *it2;
			if (embedding_->CalculateL(u1, u2) <= delta_){
				support_sets_[0][u1].insert(u2);
				support_sets_[1][u2].insert(u1);
			}
			else{
				total++;
			}
		}
	}
	return total; //unqualified
}

ERR EmbeddingAC::EmbeddingArcPruningByGroupUndirect(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();

	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	FOR_EACH_DIRECTION;
	if (0 == group_items[dir]->size()){
		for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); ){
			int vertex = *it;
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(vertex);
		}
		return 0;
	}
	END_FOR_EACH_DIRCTION;

	int dir = 0;
	/*find the max I value except infinigy*/
	int I_max = 0;
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}

	/*init bucket size and infinity*/
	int bucket_size = I_max / delta_;
	int emb_k = embedding_->k();
	Bucket bucket;
	/*every demention has bucket_size bucket, the last one is for the infinity bucket*/
	Buckets buckets(bucket_size , bucket);
	BucketsK buckets_k(emb_k, buckets); /*k-demention*/

	set<int> right_set; /*for right side pruning*/

	/*build buckets*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it; //right
		for (int I = 0; I < emb_k; I++){
			/*put q in to 0 - bucket_index + 1 buckets*/
			int I_value = embedding_->rks(q)->at(I);
			
			int bucket_index = I_value / delta_;
			int start = max(bucket_index - 1, 0);
			int end = min(bucket_index + 1, bucket_size - 1); /* can not oversize the last one*/
			for (int index = start; index <= end; index++){
				buckets_k[I][index].insert(q);
			}
			
		}
	}

	end_time = GetTickCount();
	COUT(<< "	build buckets " << end_time - start_time << " ms" << endl;);
	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){

		int p = *it; //left
		Cans cans; /*put all qualified bucket pointers*/
		cans.reserve(embedding_->k());

		/*build can for p*/
		Rk *rk = embedding_->rks(p);
		for (int I = 0; I < emb_k; I++){
			//if (I > 10 && I < 90) //debug
				//continue;
			int I_value = rk->at(I);
			int bucket_index = I_value / delta_;
			if (bucket_index < bucket_size)
				cans.push_back(&buckets_k[I][bucket_index]);
		}

		/*find the intersection of all bucket in the cans*/
		vector<int> support_set;
		FindIntersection(&cans, &support_set);

		/*add support to the left*/
		if (0 == support_set.size()){
			group_items[(dir)]->erase(it++);
			deleted_queue_.push(p);
			continue;
		}
#if 1
		for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
			int q = *it4;
			/*for left out*/
			support_sets_[dir][p].insert(q);
			/*for right in*/
			support_sets_[OPP(dir)][q].insert(p);
			/*build right set*/
			right_set.insert(q);
		}
#else
		support_sets_[dir][p].insert(support_set.begin(), support_set.end());
		right_set.insert(support_set.begin(), support_set.end());

#endif
		it++;
	}

	end_time = GetTickCount();
	COUT(<< "	build support " << end_time - start_time << " ms" << endl;);

	/*find the right with no support and delete it*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
		int q = *it;
		if (right_set.find(q) == right_set.end()){
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(q);
			continue;
		}
		it++;
	}

	end_time = GetTickCount();
	COUT(<< "	delete right side " << end_time - start_time << " ms" << endl;);

	return 0;
}

#if 0
ERR EmbeddingAC::EmbeddingArcPruningByGroupUndirect2(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();

	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	FOR_EACH_DIRECTION;
	if (0 == group_items[dir]->size()){
		for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
			int vertex = *it;
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(vertex);
		}
		return 0;
	}
	END_FOR_EACH_DIRCTION;

	int dir = 0;
	/*find the max I value except infinigy*/
	int I_max = 0;
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}

	/*init bucket size and infinity*/
	int bucket_size = I_max / delta_;
	int emb_k = embedding_->k();
	NBucket bucket;
	/*every demention has bucket_size bucket, the last one is for the infinity bucket*/
	NBuckets buckets(bucket_size, bucket);
	NBucketsK buckets_k(emb_k, buckets); /*k-demention*/

	set<int> right_set; /*for right side pruning*/

	/*build buckets*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end(); it++){
		int q = *it; //right
		for (int I = 0; I < emb_k; I++){
			/*put q in to 0 - bucket_index + 1 buckets*/
			int I_value = embedding_->rks(q)->at(I);

			int bucket_index = I_value / delta_;
			int start = max(bucket_index - 1, 0);
			int end = min(bucket_index + 1, bucket_size - 1); /* can not oversize the last one*/
			for (int index = start; index <= end; index++){
				buckets_k[I][index].push_back(q);
			}

		}
	}

	end_time = GetTickCount();
	COUT(<< "	build buckets " << end_time - start_time << " ms" << endl;);
	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end();){

		int p = *it; //left
		Cans cans; /*put all qualified bucket pointers*/
		cans.reserve(embedding_->k());

		/*build can for p*/
		Rk *rk = embedding_->rks(p);
		for (int I = 0; I < emb_k; I++){
			//if (I > 10 && I < 90) //debug
			//continue;
			int I_value = rk->at(I);
			int bucket_index = I_value / delta_;
			if (bucket_index < bucket_size)
				cans.push_back(&buckets_k[I][bucket_index]);
		}

		/*find the intersection of all bucket in the cans*/
		vector<int> support_set;
		FindIntersection(&cans, &support_set);

		/*add support to the left*/
		if (0 == support_set.size()){
			group_items[(dir)]->erase(it++);
			deleted_queue_.push(p);
			continue;
		}
#if 0
		for (auto it4 = support_set.begin(); it4 != support_set.end(); it4++){
			int q = *it4;
			/*for left out*/
			support_sets_[dir][p].insert(q);
			/*for right in*/
			support_sets_[OPP(dir)][q].insert(p);
			/*build right set*/
			right_set.insert(q);
		}
#else
		support_sets_[dir][p].insert(support_set.begin(), support_set.end());
		right_set.insert(support_set.begin(), support_set.end());

#endif
		it++;
	}

	end_time = GetTickCount();
	COUT(<< "	build support " << end_time - start_time << " ms" << endl;);

	/*find the right with no support and delete it*/
	for (auto it = group_items[OPP(dir)]->begin(); it != group_items[OPP(dir)]->end();){
		int q = *it;
		if (right_set.find(q) == right_set.end()){
			group_items[OPP(dir)]->erase(it++);
			deleted_queue_.push(q);
			continue;
		}
		it++;
	}

	end_time = GetTickCount();
	COUT(<< "	delete right side " << end_time - start_time << " ms" << endl;);

	return 0;
}
#endif
ERR EmbeddingAC::FindIntersection(Cans *cans, vector<int> *result){
	
	auto fun = [](Bucket* b1, Bucket *b2){
		return (b1->size() < b2->size());
	};

	/*sort for finding the shortest buckets in can*/
	sort(cans->begin(), cans->end(), fun);
	if (0 == cans->size())
		return 0;
	Bucket* first_bucket = cans->at(0);

#if 0 /*use set_intersection, it seem not efficient*/
	result->assign(first_bucket->begin(), first_bucket->end());
	return 0;
	vector<int> buffer;
	for (int i = 1; i < (int)cans->size(); i++){
		buffer.clear();
		set_intersection(result->begin(), result->end(),
			cans->at(i)->begin(), cans->at(i)->end(), back_inserter(buffer));
		swap(*result, buffer);
	}

#else
	for (auto it2 = first_bucket->begin(); it2 != first_bucket->end(); it2++){
		int q = *it2;
		bool is_intersection = true;

		for (int i = 1; i < (int)cans->size(); i++){
			if (q < *cans->at(i)->begin() || q > *cans->at(i)->rbegin())
			{
				is_intersection = false;
				break;
			}
			else if (cans->at(i)->find(q) == cans->at(i)->end()){
				is_intersection = false;
				break;

			}
		}

		/*add to the support */
		if (true == is_intersection){
			result->push_back(q);
		}
	}
#endif
	return 0;
}

#if 0
ERR EmbeddingAC::EmbeddingArcPruningByGroup3(LabelPair label_pair){
	DWORD start_time, end_time;
	start_time = GetTickCount();

	Bucket temp_bucket;

	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);

	int dir = 0;
	/*build hash for R2*/
	vector<map<int, set<int>>> hash_table;
	hash_table.insert()
	for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
		int u2 = *it2;
		
	}
	/*build support by previous buckets.*/
	for (auto it = group_items[(dir)]->begin(); it != group_items[(dir)]->end(); it++){
		int u1 = *it;
		


	}
	return 0;
}
#endif
int EmbeddingAC::NeighborAreaPruning(){
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		auto label_pair = *it;
		NeighborAreaPruningByGroup(label_pair);
	}
	DeletedPruning();
	return 0;
}
int EmbeddingAC::NeighborAreaPruningByGroup(LabelPair label_pair){
	int label[2] = { label_pair.first, label_pair.second };
	GroupItems *group_items[2];
	group_items[0] = group_->items(label[0]);
	group_items[1] = group_->items(label[1]);
	int count = 0;
	Area area;
	AreaI area_I;
	area.insert(area.begin(), embedding_->k(), area_I); /*for k demetion*/

	/*scan each Ri to find the area(Ri)*/
	FOR_EACH_DIRECTION;
	for (int I = 0; I < embedding_->k(); I++) {
		AreaI area_I, area_I_new;
		/*buile area_I*/
		for (auto it2 = group_items[OPP(dir)]->begin(); it2 != group_items[OPP(dir)]->end(); it2++){
			int vertex = *it2;
			auto rk = embedding_->rks(vertex);
			int value = (*rk)[I];

	/*		if (DOUT == dir){
				area_I.insert({ max(value - delta_, 0), value + delta_ });
				area_I.insert({ 0, value + delta_ });
			}
			else{
				area_I.insert({ value - delta_, kInfinity });
			}*/
			area_I.insert({ max(value - delta_, 0), value + delta_ });
		}
		/*combine area_I if necessary, int the area_I sorted by first.*/
		pair<int, int> pre = { -1, -1 }, cur;
		for (auto it2 = area_I.begin(); it2 != area_I.end(); it2++){
			if (pre.first == -1){
				pre = *it2;
				area_I_new.insert(*it2);
				continue;
			}
			cur = *it2;
			if (cur.first <= pre.second){
				if (cur.second > pre.second){
					/*cross, combine together*/
					area_I_new.erase(pre);
					area_I_new.insert({ pre.first, cur.second });
					pre = { pre.first, cur.second };
				}
				else {
					/*include do nothing*/
				}
			}
			else{
				/*not include*/
				area_I_new.insert(cur);
				pre = cur;
			}
		}
		area[I] = area_I_new;
	}



	/*scan Ri to filter*/
	for (auto it2 = group_items[dir]->begin(); it2 != group_items[dir]->end();){
		int vertex = *it2;
		auto rk = embedding_->rks(vertex);

		if (false == IsInNeighborArea(&area, rk, embedding_->k())){
			group_items[dir]->erase(it2++);
			deleted_queue_.push(vertex);
			count++;
		}
		else{
			it2++;
		}
	}
	END_FOR_EACH_DIRCTION;
	return count;
}

int EmbeddingAC::EmbeddingArcPruningVerify(){
	int count = 0;
	DWORD start_time, end_time;
	start_time = GetTickCount();
	/*iterate all the out support*/
	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		int label = *it;
		GroupItems *group_items = group_->items(label);
		for (auto it2 = group_items->begin(); it2 != group_items->end();){
			int s = *it2;
			SupportSet *support_set = &support_sets_[0][s];
			if (0 == support_set->size()){
				it2++;
				continue;
			}

			EmbeddingArcPruningVerifyBySupport(s, support_set);
			if (0 == support_set->size()){
				group_items->erase(it2++);
				deleted_queue_.push(s);
				//DeletedPruning();
			}
			else{
				it2++;
			}
			count++;
			end_time = GetTickCount();
			//COUT(<< "  EmbeddingArcPruningVerifyBySupport " << end_time - start_time << " ms"<< endl;)
		}
	}
	DeletedPruning();
	return count;
}
#if 1
/*return 1, delete this*/
ERR EmbeddingAC::EmbeddingArcPruningVerifyBySupport(int s, SupportSet *support_set){
	/*jump over the empty support list.*/
	auto dijkstraFun = [](int s, int dir, Vertex *u, int argc, void **argv){
		set<int> *temp_set = (set<int>*)argv[0];
		int delta = *(int*)argv[1];

		if (u->dist <= delta){
			temp_set->erase(u->index);
		}
		else{
			return -1; //quite dijkstra
		}
		return 0;
	};

	/*init*/
	set<int> temp_set;
	for (auto it = support_set->begin(); it != support_set->end(); it++){
		temp_set.insert(*it);
	}

	/*find all the qualified vertex and erase in support_set.*/
	int argc = 2;
	void *argv[2] = { &temp_set, &delta_ };
	g_->Dijkstra(s, 0, dijkstraFun, argc, argv);

	/*remove unqualified supports. vertex in support_set should be removed all.*/
	for (auto it = support_set->begin(); it != support_set->end();){
		int vertex = *it;
		if (temp_set.find(vertex) != temp_set.end()){
			support_set->erase(it++);
			auto opp_support_set = &support_sets_[1][vertex];
			opp_support_set->erase(s);
			if (0 == opp_support_set->size()){
				group_->items(g_->get_label(vertex))->erase(vertex);
				deleted_queue_.push(vertex);
			}
			
		}
		else{
			it++;
		}
	}
	return 0;
}
#else
ERR EmbeddingAC::EmbeddingArcPruningVerifyBySupport(int s, SupportSet *support_set){
	/*jump over the empty support list.*/
	auto dijkstraFun = [](int s, int dir, Vertex *u, int argc, void **argv){
		int delta = *(int*)argv[0];
		if (u->dist > delta){
			return -1; //quite dijkstra
		}
		return 0;
	};

	/*init*/
	set<int> temp_set;
	for (auto it = support_set->begin(); it != support_set->end(); it++){
		temp_set.insert(*it);
	}

	/*find all the qualified vertex and erase in support_set.*/
	int argc = 1;
	void *argv[1] = { &delta_ };
	g_->Dijkstra(s, 0, dijkstraFun, argc, argv);

	/*remove unqualified supports. vertex in support_set should be removed all.*/
	for (auto it = support_set->begin(); it != support_set->end();){
		int vertex = *it;
		if (g_->vertices(vertex)->dist > delta_){
			
			support_set->erase(it++);	
		}
		else{
			it++;
		}
	}
	if (0 == support_set->size()){
		group_->items(g_->get_label(s))->erase(vertex);
		deleted_queue_.push(vertex);
	}
		
	return 0;
}
#endif
bool EmbeddingAC::IsInNeighborArea(Area *area, Rk *rk, int k){
	for (int I = 0; I < k; I++){
		int d = (*rk)[I];
		AreaI * areai = &(*area)[I];
		bool in_areai = false;
		for (auto it = areai->begin(); it != areai->end(); it++){
			if (d >= it->first && d <= it->second){
				in_areai = true;
				break;
			}
		}
		if (false == in_areai)
			return false;
	}
	return true;
}

int EmbeddingAC::DeletedPruning(){
	int count = 0;
	SupportSet *s[2];

	while (deleted_queue_.size() > 0){
		int u = deleted_queue_.front();
		int u_label = g_->get_label(u);
		deleted_queue_.pop();

		s[0] = &support_sets_[0][u];
		s[1] = &support_sets_[1][u];

		FOR_EACH_DIRECTION;
		for (auto it = s[dir]->begin(); it != s[dir]->end();it++){
			int support = *it;
			SupportSet *opp_support_set = &support_sets_[OPP(dir)][support];
			opp_support_set->erase(u);
			if (0 == opp_support_set->size()){
				group_->items(g_->get_label(support))->erase(support);
				deleted_queue_.push(support);
			}
		}
		END_FOR_EACH_DIRCTION;
	}

	return count;
}


int EmbeddingAC::write(const char *filename, char* delim){
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

	for (int label = 0; label < group_->label_num(); label++)
	{
		auto items = group_->items(label);
		for (auto it = items->begin(); it != items->end(); it++)
		{
			int center = *it;

			auto support_out = support_sets_[0][center];
			auto support_in = support_sets_[1][center];
			if (0 == support_out.size() && 0 == support_in.size())
				continue;
			// label, center, right support
			strbuf = _itoa(label, buf, 10); //label
			strbuf += delim;
			strbuf += _itoa(center, buf, 10); //center

			for (auto it2 = support_out.begin(); it2 != support_out.end(); it2++){
				strbuf += delim;
				strbuf += "[";
				strbuf += _itoa(g_->get_label(*it2), buf, 10); //support label
				strbuf += "-";
				strbuf += _itoa(*it2, buf, 10); //support vertex
				strbuf += "]";
			}

			for (auto it2 = support_in.begin(); it2 != support_in.end(); it2++){
				strbuf += delim;
				strbuf += "(";
				strbuf += _itoa(g_->get_label(*it2), buf, 10); //support label
				strbuf += "-";
				strbuf += _itoa(*it2, buf, 10); //support vertex
				strbuf += ")";
			}

			strbuf += "\n";
			fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);

		}

	}

	fclose(fp);
	return 0;
}

#endif