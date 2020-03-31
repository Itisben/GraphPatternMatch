#include "edge-join.h"
//#include "main.h"

EdgeJoin::EdgeJoin(Graph *g, Query *q, Group *group, Embedding* embedding,
	Twohop *twohop, Cluster *cluster){
	g_ = g;
	q_ = q;
	group_ = group;
	delta_ = q->delta();
	embedding_ = embedding;
	twohop_ = twohop;
	cluster_ = cluster;
	debug_ = 0;
}

ERR EdgeJoin::NeighborAreaPruning(int max_loop_num, int *loop_num, int *delete_num){
	bool is_change;
	int my_loop_num = 0;
	int my_delete_num = 0;
	Areas areas;
	Area area;
	AreaI area_I;
	area.insert(area.begin(), embedding_->k(), area_I); /*for k demetion*/
	areas.insert(areas.begin(), g_->label_num(), area); /*for every label*/

BEGIN:
	is_change = false;
	my_loop_num++;

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
					} else {
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

	if (true == is_change && my_loop_num <= max_loop_num)
		goto BEGIN;

	*loop_num = my_loop_num;
	*delete_num = my_delete_num;
	return 0;
}

bool EdgeJoin::IsInNeighborArea(Area *area, Rk *rk, int k){
	for (int I = 0; I < k; I++){
		int d = (*rk)[I];
		if (-1 == d)
			d = kInfinity;
		AreaI * areai = &(*area)[I];
		/*only one in the areai, then in. Here is confusing. */
		bool in_areai = false;
		for (auto it = areai->begin(); it!= areai->end(); it++){
			if (d >= it->first && d <= it->second){
				in_areai = true;
				break;
			}	
		}
		/*if in one demention not in the area than not in*/
		if (false == in_areai)
			return false;
	}
	return true;
}


/*label_pair: the edge in the query
*we use block nested loop join strategy in edge join algrithm
*/
ERR EdgeJoin::DistanceBasedEdgeJoin(LabelPair *label_pair, PairSet *rs){
	DWORD start_time, end_time;
	start_time = GetTickCount();
	rs->clear();
	int count1 = 0; //count of deleted vertices.
	int count2 = 0;
	int label[2] = {label_pair->first, label_pair->second};
	auto cluster_units_1 = cluster_->group_clusters(label[0]);
	auto cluster_units_2 = cluster_->group_clusters(label[1]);
	set<pair<int, int>> cl;

	for (auto it = cluster_units_1->begin(); it != cluster_units_1->end(); it++){
		ClusterUnit *cluster_unit_1 = &it->second;
		int c1 = cluster_unit_1->c;
		for (auto it2 = cluster_units_2->begin(); it2 != cluster_units_2->end(); it2++){
			ClusterUnit *cluster_unit_2 = &it2->second;
			int c2 = cluster_unit_2->c;

			/*accroding to  theorem 5, jump over*/
			int dist = embedding_->CalculateL(c1, c2);
			if (dist >= (cluster_unit_1->r + cluster_unit_2->r + delta_)){
				//pruned number
				count1 += cluster_unit_1->objs.size();
				count2 += cluster_unit_2->objs.size();
				continue;
			}
			MemoryEdgeJoin(cluster_unit_1, cluster_unit_2, label, &cl, &count1);
		}
	}

	end_time = GetTickCount();
	COUT(<< "	Cluster MemoryEdgeJoin End " << end_time - start_time << " ms" << endl;);
	//COUT(<< "	Delete Count: " << count1 << " "<< count2 << endl;);
#ifndef EDGE_JOIN_TWOHOP_DEBUG
	/*verification by two hop*/
	set<int> g1, g2;
	COUT(<< "	Cadidate Relation size: " << cl.size() << endl;);
	for (auto it = cl.begin(); it != cl.end();){
		int dist_sp = twohop_->CalculateDist(it->first, it->second);
		if (dist_sp <= delta_){
			rs->insert(*it);
			g1.insert(it->first);
			g2.insert(it->second);
			it++;
		}
		else{
			cl.erase(*it++);
		}
	}
	end_time = GetTickCount();
	COUT(<< "	twohop check End " << end_time - start_time << " ms" << endl;);
	COUT(<< "  *******Final Relation size: " << cl.size() << endl;);
	COUT(<< "	Group Count group" << label_pair->first<<":"<<g1.size() << 
		" group"<<label_pair->second<<":"<<g2.size() << endl;);

#endif
	return cl.size();
}

ERR EdgeJoin::MemoryEdgeJoin(ClusterUnit *cluster_unit_1, ClusterUnit * cluster_unit_2, int *label, PairSet *cl, int *count) {
	auto R2 = group_->items(label[1]);
	/*for each p in c2*/
	for (auto it = cluster_unit_2->objs.begin(); it != cluster_unit_2->objs.end(); it++){
		int p = *it;
		if (R2->find(p) == R2->end()){
			continue;
		}
		/*find search space in C1 for p , SP(p) and Can(p), and intersect them*/
		set<int> sp_p, can_p;
		FindSP(cluster_unit_1, p, &sp_p);
		FindCan(cluster_unit_1, p, &can_p);
		for (auto it = sp_p.begin(); it != sp_p.end();){
			if (can_p.find(*it) == can_p.end()){
				sp_p.erase(it++);
				if (NULL != count)
					*count += sp_p.size();
			}
			else{
				it++;
			}
		}

		
		for (auto it = sp_p.begin(); it != sp_p.end(); it++){
			int q = *it;
			if (embedding_->CalculateL(q, p) <= delta_)
				cl->insert({q, p});
		}
	}
	return 0;
}

ERR EdgeJoin::FindSP(ClusterUnit *cluster_unit_1, int p, set<int> *sp_p){
	int c1 = cluster_unit_1->c;
	int r_c1 = cluster_unit_1->r;
	int L_p_c1 = embedding_->CalculateL(c1, p);
	int low = max(L_p_c1 - delta_, 0);
	int high = min(L_p_c1 + delta_, r_c1);
	
	sp_p->clear();
	for (auto it = cluster_unit_1->objs.begin(); it != cluster_unit_1->objs.end(); it++){
		int q = *it;
		int L_q_c1 = embedding_->CalculateL(q, c1);
		if (L_q_c1 >= low && L_q_c1 <= high){
			sp_p->insert(q);
		}
	}
	return 0;
}

ERR EdgeJoin::FindCan(ClusterUnit *cluster_unit_1, int p, set<int> *can_p){
	int bucket_size;
	int emb_k = embedding_->k();

	/*init*/
	int I_max = 0; /*find the max I value*/
	for (auto it = cluster_unit_1->objs.begin(); it != cluster_unit_1->objs.end(); it++){
		int q = *it;
		int I_max_new = embedding_->GetMaxIValue(q);
		if (I_max < I_max_new){
			I_max = I_max_new;
		}
	}

	bucket_size = I_max / delta_ + 1;
	Bucket bucket;
	Buckets buckets(bucket_size, bucket);
	BucketsK buckets_k(emb_k, buckets);
	vector<int> temp;
	/*build buckets*/
	can_p->clear();
	for (auto it = cluster_unit_1->objs.begin(); it != cluster_unit_1->objs.end(); it++){
		int q = *it;
		for (int I = 0; I < emb_k; I++){
			/*put q in to the 3 buckets if I can find 3 buckets*/
			int I_value = embedding_->rks(q)->at(I);
			int bucket_index = I_value / delta_;
			
			int start = max(bucket_index - 1, 0); /*find 3 buckets*/
			int end = min(bucket_index + 1, bucket_size - 1); /* 0 to bucket_size - 1*/
			for (int bucket = start; bucket <= end; bucket++){
				buckets_k[I][bucket].insert(q);
			}
		}
	}

	/*according to p, find can, put result into the k bucket*/
	Cans cans; /*k demention*/
	Rk *rk = embedding_->rks(p);
	for (int I = 0; I < emb_k; I++){
		int bucket_index = rk->at(I) / delta_;
		if (bucket_index < bucket_size) /*if p cannot find bucket*/
			cans.push_back(&buckets_k[I][bucket_index]);
	}

	/*no result*/
	if (0 == cans.size())
		return 0;

	/*get the intersection of all cans*/
	auto fun = [](Bucket *b1, Bucket *b2){
		return b1->size() < b2->size();
	};
	/*sort for finding the shortest buckets in can*/
	sort(cans.begin(), cans.end(), fun);
	Bucket *b0 = cans[0]; /*shortest one, find the intersection with all other buckets*/
	for (auto it = b0->begin(); it != b0->end(); it++){
		int q = *it;
		bool is_intersection = true;
		for (int i = 1; i < (int)cans.size(); i++){
			if (cans[i]->find(q) == cans[i]->end()){
				is_intersection = false;
				break;
			}
		}
		if (true == is_intersection)
			can_p->insert(q);
	}
	
	return 0;
}


ERR EdgeJoin::MultiwayDistanceJoin(int max_loop_num){
	PairSet rs;
	int loop_num = 0;
	bool change = false;
	int total_relation = 0;
	DWORD start_time, end_time;

	if (0 == q_->label_join_orders()->size())
		return 0;

	mrs_.clear();
	mrs_.insert(mrs_.begin(), q_->label_join_orders()->size(), rs);

	start_time = GetTickCount();
	int i_pre = 0;
	LabelPair *label_pair = &q_->label_join_orders(0)->first;
	COUT(<< "LABEL PAIR " << label_pair->first << " " << label_pair->second << endl;)
	DistanceBasedEdgeJoin(label_pair, &mrs_[0]);
	end_time = GetTickCount();	
	COUT (<< "DistanceBasedEdgeJoin " << " mrs num " << mrs_[0].size() << " "<< end_time-start_time<< "ms"<<endl;)
	/*now only do the forward edge*/
	for (int i = 1; i < (int)q_->label_join_orders()->size(); i++){
		/*pruning the Ri*/
		GroupPruningByMrInPair(label_pair, &mrs_[i_pre]);

		label_pair = &q_->label_join_orders(i)->first;
		int type = q_->label_join_orders(i)->second;	

		/*forward edge*/
		
		//if (FORWARD_EDGE == type){
			COUT(<< "FORWARD EDGE ";)
			COUT(<< "LABEL PAIR " << label_pair->first << " " << label_pair->second << endl;)
			DistanceBasedEdgeJoin(label_pair, &mrs_[i]);
			COUT(<< "DistanceBasedEdgeJoin " << " mrs num " << mrs_[i].size() << endl;)

			//PairSet *mr[2] = { &mrs_[i_pre], &mrs_[i] }; /*back edge do nothing*/
			//change = MrJoin(mr);
			//end_time = GetTickCount();
			//COUT(<< "	after MrJoin " << i << " pre mrs num " << mrs_[i_pre].size() << " mrs num " << mrs_[i].size() << " " << end_time - start_time << "ms" << endl;)
			//COUT(<< endl;);
		//}
#if 0
		else{
			COUT(<< "BACKWARD EDGE ";)
			COUT(<< "LABEL PAIR " << label_pair->first << " " << label_pair->second << endl;)

			PairSet *mr[2] = { &mrs_[i_pre], &mrs_[i] }; /*back edge do nothing*/
			change = MrJoin(mr);
			COUT(<< "	after MrJoin " << i << " pre mrs num " << mrs_[i_pre].size() << " mrs num " << mrs_[i].size() << " " << end_time - start_time << "ms" << endl;)
			COUT(<< endl;);
		}
#endif	

		//end_time = GetTickCount();
		//COUT(<< "===MrJoin " << i << " pre mrs num " << mrs_[i_pre].size() << " mrs num " << mrs_[i].size() << " " << end_time - start_time << "ms" << endl;)
		i_pre = i;
	}
#if 1 // these not include in the paper.
	end_time = GetTickCount();
	COUT(<< endl;);
	COUT(<< " ******after D-join: " << end_time - start_time << "ms" << endl;);
	total_relation = 0;
	for(auto it=mrs_.begin(); it!=mrs_.end(); it++){
		total_relation += it->size();
	}
	COUT(<< " *** Total Relation: " << total_relation << endl;)
		COUT(<< endl;);
#if 0

BEGIN:
	change = false;
	loop_num++;
	i_pre = 0;
	
	//for (int i = 1; i < (int)q_->label_join_orders()->size(); i++){
		//int type = q_->label_join_orders(i)->second;
		//if (FORWARD_EDGE == type && 1 == loop_num){
			PairSet *mr[2] = { &mrs_[i_pre], &mrs_[i] };
			change = MrJoin(mr);
			COUT(<< " " << i_pre << " " << i << endl;);
			COUT(<< "MrJoin pruning " << i << " mrs num " << mrs_[i].size() << endl;);
		//}
		i_pre = i;
	//}
	if (true == change && loop_num < max_loop_num)
		goto BEGIN;
#endif
BEGIN:
	change = false;
	loop_num++;
	int order[3][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 } };
	for (int i = 0; i < 3; i++){
		PairSet *mr[2] = { &mrs_[order[i][0]], &mrs_[order[i][1]] };
		if (true == MrJoin(mr))
			change = true;
		COUT(<< "MrJoin pruning <" << order[i][0] << "," << order[i][1] << ">: "<< mrs_[order[i][0]].size() << " " << mrs_[order[i][1]].size() << endl;);
	}
	if (true == change && loop_num <=  max_loop_num)
		goto BEGIN;
#endif
	end_time = GetTickCount();
	COUT(<< endl;);
	COUT(<< " ******after MD-join: " << end_time - start_time << "ms" << endl;);

	total_relation = 0;
	for (auto it = mrs_.begin(); it != mrs_.end(); it++){
		total_relation += it->size();
	}
	COUT(<< " *** Total Relation: " << total_relation << endl;)
		COUT(<< endl;);

	return loop_num;
}

bool EdgeJoin::MrJoin(PairSet *mr[2]){
	bool change = false;
	int dir = 0;
	//FOR_EACH_DIRECTION;
	set<int> myset;
	for (auto it = mr[OPP(dir)]->begin(); it != mr[OPP(dir)]->end();it++){
		if (0 == dir)
			myset.insert(it->first);
		else
			myset.insert(it->second);
	}

	for (auto it = mr[dir]->begin(); it != mr[dir]->end();){
		int vertex;
		if (0 == dir)
			vertex = it->second;
		else
			vertex = it->first;
		if (myset.find(vertex) == myset.end()){
			mr[dir]->erase(it++);
			change = true;
		}
		else{
			it++;
		}
	}
	//END_FOR_EACH_DIRCTION;

	return change;
}

bool EdgeJoin::GroupPruningByMrInPair(LabelPair *label_pair, PairSet *mr){
	bool change = false;

	int label[2] = { label_pair->first, label_pair->second };
	GroupItems *R[2] = { group_->items(label_pair->first), group_->items(label_pair->second) };

	set<int> temp_set[2];
	for (auto it = mr->begin(); it != mr->end(); it++){
		temp_set[0].insert(it->first);
		temp_set[1].insert(it->second);
	}
	FOR_EACH_DIRECTION;
	for (auto it = R[dir]->begin(); it != R[dir]->end();){
		if (temp_set[dir].find(*it) == temp_set[dir].end()){
			R[dir]->erase(it++);
			change = true;
		}
		else{
			it++;
		}
	}
	END_FOR_EACH_DIRCTION;
	return change;
}



int EdgeJoin::Write(const char *filename, char *delim) {
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

	for (int i = 0; i < (int)mrs_.size(); i++) {
		LabelPair label_pair = q_->label_join_orders(i)->first;
		strbuf = "[";
		strbuf += _itoa(label_pair.first, buf, 10);
		strbuf += "-";
		strbuf += _itoa(label_pair.second, buf, 10);
		strbuf += "]";

		PairSet *pair_set = &mrs_[i];
		for (auto it = pair_set->begin(); it != pair_set->end(); it++){
			strbuf += delim;
			strbuf += "[";
			strbuf += _itoa(it->first, buf, 10);
			strbuf += "-";
			strbuf += _itoa(it->second, buf, 10);
			strbuf += "]";
		}
		strbuf += "\n";
		fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
	}

	fclose(fp);
	return 0;
}


