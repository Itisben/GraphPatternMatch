#include "twohopac.h"
#include "main.h"
#include <Windows.h>
TwohopAc::TwohopAc(Graph *g, Query *q, Group *group){
	g_ = g;
	q_ = q;
	group_ = group;
	delta_ = q_->delta();

	Labeling lab;
	NewSupport s;
	Count c = 0;
	FOR_EACH_DIRECTION;
	labelings_[dir].insert(labelings_[dir].begin(), g_->max_vertex_num(), lab);
	supports_[dir].insert(supports_[dir].begin(), g_->max_vertex_num(), s);
	//counts_[dir].insert(counts_[dir].begin(), g_->max_vertex_num(), c);
	CountNew cnew;
	counts_new_[dir].insert(counts_new_[dir].begin(), g_->max_vertex_num(), cnew);
	END_FOR_EACH_DIRCTION;
}
TwohopAc::~TwohopAc(){

}
int TwohopAc::ReadLabeling(const char *filename, const char *delim){
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
		
		if ('*' == strbuf[0])
			break;
		else if ('#' == strbuf[0])
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

		int dir = value[1];
		int vertex = value[0];

		/*for test, pruning method is not necessary here.
		this is for saving space. */
#if 1
		if (value[3] > kDelta)
			continue; //pruning method
#endif	
		labelings_[dir][vertex].push_back({ value[2], value[3] }); //2:center, 
	}
	//sort all labelings for pruning.
	auto fun = [](pair<int, int> a, pair<int, int> b){
		return a.second < b.second;
	};
	FOR_EACH_DIRECTION
		for (auto it = labelings_[dir].begin(); it != labelings_[dir].end(); it++){
			it->sort(fun);
		}
	END_FOR_EACH_DIRCTION
	fclose(fp);

	return 0;
}

int TwohopAc::SortLabeling(){
	typedef pair<int, int> PAIR;
	auto fun = [](PAIR p1, PAIR p2){
		return p1.second < p2.second;
	};
	FOR_EACH_DIRECTION;
	for (auto it = labelings_[dir].begin(); it != labelings_[dir].end(); it++){
		if (false == it->empty())
			it->sort(fun);
	}
	END_FOR_EACH_DIRCTION;

	return 0;
}

int TwohopAc::HashJoin(int label1, int label2, int dir){
	int count = 0;
	GroupItems *mygroup[2];
	mygroup[0] = group_->items(label1);
	mygroup[1] = group_->items(label2);
		
	//key: center. value: array[0] center, array[1] dist, ,
	map<int, list<array<int, 2>>> arc_map;
	list <array<int, 2>> arc_map_list;
	vector<byte> right(mygroup[OPP(dir)]->size(), 0);

	/*for left pruning, build arc map for right*/
	for (auto it1 = mygroup[OPP(dir)]->begin(); it1 != mygroup[OPP(dir)]->end(); it1++){
		int vertex = *it1;
		auto items = &labelings_[OPP(dir)][vertex];
		for (auto it2 = items->begin(); it2 != items->end(); it2++){
			int center = it2->first;
			int dist = it2->second;
			if (dist > delta_)
				break; //this is pruning method based on the sorted labeling by weight

			if (arc_map.find(vertex) != arc_map.end()){
				arc_map.insert({ vertex, arc_map_list });
			}
			array<int, 2> a = { vertex, dist };
			arc_map[vertex].push_back(a);
		}
		arc_map[vertex].sort(); // sort for pruning
	}

	/*build support for left*/
	for (auto it1 = mygroup[(dir)]->begin(); it1 != mygroup[(dir)]->end();){
		int vertex = *it1;
		auto item = &labelings_[OPP(dir)][vertex];
		set<int> support; //key: center in the arc_map
		for (auto it2 = item->begin(); it2 != item->end(); it2++){
			int center = it2->first;
			int dist = it2->second;
			if (dist > delta_)
				break; //this is pruning method based on the sorted labeling by weight

			for (auto it3 = arc_map[center].begin(); it3 != arc_map[center].end(); it3++){
				int map_vertex = (*it3)[0];
				int map_dist = (*it3)[1];
				if (map_dist + dist > delta_)
					break; //this is the second pruning method.
				//find the support
				support.insert(map_vertex);
				
			}
		}

		if (true == support.empty()){
			mygroup[(dir)]->erase(it1++);
			count++;
			deleted_queue_.push(vertex);
			continue;
		}
		else{
			it1++;
		}

		/*build support for right*/
		int i = 0;
		for (auto it4 = support.begin(); it4 != support.end(); it4++){
			int s = *it4;
			/*left*/
			supports_[dir][vertex].push_back(s);
			counts_[dir][vertex]++;
			/*right, just opposite*/
			supports_[OPP(dir)][s].push_back(vertex);
			counts_[OPP(dir)][s]++;
			right[i++] = 1;
		}
	}

	/*fine the right side one with no support and delete it*/
	int i = 0;
	for (auto it1 = mygroup[OPP(dir)]->begin(); it1 != mygroup[OPP(dir)]->end();){
		int vertex = *it1;
		if (right[i++] == 0){
			mygroup[OPP(dir)]->erase(it1++);
			count++;
			deleted_queue_.push(vertex);
		}
		else{
			it1++;
		}
	}
	
	return count;
}

int TwohopAc::HashJoinNew(int label1, int label2){
	int count = 0;
	GroupItems *mygroup[2];
	mygroup[0] = group_->items(label1); //left
	mygroup[1] = group_->items(label2); //right

	//key: center. value: array[0] center, array[1] dist, ,
	map<int, list<array<int, 2>>> arc_map;
	list <array<int, 2>> arc_map_list; //{vertex, dist}

	/*for left pruning, build arc map for right. dir = 0 left*/
	for (auto it1 = mygroup[1]->begin(); it1 != mygroup[1]->end(); it1++){
		int vertex = *it1;
		auto items = &labelings_[1][vertex]; //2-hop. 0 out, 1 in.
		for (auto it2 = items->begin(); it2 != items->end(); it2++){
			int center = it2->first;
			int dist = it2->second;
			if (dist > delta_)
				break; //this is pruning method based on the sorted labeling by weight

			array<int, 2> a = { vertex, dist };
			arc_map[center].push_back(a);
		}
	}
	COUT(<< "arc-map size " << arc_map.size() << endl);
	/*sort arc_map*/
	auto fun = [](array<int, 2> a, array<int, 2> b){
		return a[1] < b[1]; //sort by distance
	};
	for (auto it1 = arc_map.begin(); it1 != arc_map.end(); it1++){
		it1->second.sort(fun);
	}

	/*clear right group color, this is delete right group*/
	for (auto it1 = mygroup[1]->begin(); it1 != mygroup[1]->end();it1++)
	{
		g_->vertex(*it1)->color = kWhite;
	}

	//iterate left group
	for (auto it1 = mygroup[0]->begin(); it1 != mygroup[0]->end();){
		int vertex = *it1;
		auto item = &labelings_[0][vertex]; //out labeling
		set<int> support_temp; //avoid to insert repeatly.
		
		/*build support for left*/
		for (auto it2 = item->begin(); it2 != item->end(); it2++){
			int center = it2->first;
			int dist = it2->second;
			if (dist > delta_)
				break; //this is pruning method based on the sorted labeling by weight

			for (auto it3 = arc_map[center].begin(); it3 != arc_map[center].end(); it3++){
				int map_vertex = (*it3)[0];
				int map_dist = (*it3)[1];
				if (map_dist + dist > delta_)
					break; //this is the second pruning method.
				//find the support
				support_temp.insert(map_vertex);

			}
		}

		if (true == support_temp.empty()){
			mygroup[0]->erase(it1++); //left
			count++;
			/*record delete and append into deleted_queue*/
			g_->vertex(vertex)->exist = false;
			if (supports_[0][vertex].size() > 0 || supports_[1][vertex].size() > 0)
				deleted_queue_.push(vertex);
			continue;
		}
		else{
			it1++;
			//add the support, and the counter for left
			auto support = &supports_[0][vertex]; //out-support
			support->insert(support->end(),support_temp.begin(), support_temp.end());
			auto count = &counts_new_[0][vertex];
			if (count->find(label2) == count->end()){
				count->insert({ label2, 0 });
			}
			count->at(label2) += (int)support_temp.size();
			//COUT(<<label1 << " "<<label2 <<": left u"<< vertex <<endl );
		}

		/*build support for right*/
		for (auto it4 = support_temp.begin(); it4 != support_temp.end(); it4++){
			int s = *it4;
			/*left*/
			supports_[1][s].push_back(vertex); //in-support
			g_->vertex(s)->color = kBlack;
			auto count = &counts_new_[1][s]; //in-counter
			if (count->find(label1) == count->end())
			{
				count->insert({ label1, 0 });
			}
			count->at(label1)++;
			//COUT(<<label1 << " "<<label2 <<": left u"<< vertex <<endl );
		}
	}

	/*fine the right side one with no support and delete it*/
	for (auto it1 = mygroup[1]->begin(); it1 != mygroup[1]->end();){
		int vertex = *it1;
		if (kWhite == g_->vertex(vertex)->color){ //no support should be delete
			mygroup[1]->erase(it1++);
			g_->vertex(vertex)->exist = false;
			count++;
			g_->vertex(vertex)->exist = false;
			if (supports_[0][vertex].size() > 0 || supports_[1][vertex].size() > 0)
				deleted_queue_.push(vertex);
		}
		else{
			it1++;
		}
	}
	arc_map.clear();
	return count;
}
/*without hash join*/
int TwohopAc::HashJoinNew2(int label1, int label2, int dir){
	int count = 0;
	GroupItems *mygroup[2];
	mygroup[0] = group_->items(label1); //left
	mygroup[1] = group_->items(label2); //right
	map< int, int> hop_map;
	set<int> support_temp;

	//iterate left group' 2 hop
	for (auto it1 = mygroup[0]->begin(); it1 != mygroup[0]->end();it1++){
		int u0 = *it1;
		//build map for left item
		auto item = &labelings_[0][u0]; // for out direction
		hop_map.clear();
		for (auto it3 = item->begin(); it3 != item->end(); it3++){
			if (it3->second > delta_)
				break;
			hop_map.insert({ it3->first, it3->second });
		}

		//iterate right group
		for (auto it2 = mygroup[1]->begin(); it2 != mygroup[1]->end();it2++){
			int u1 = *it2;
			auto item2 = &labelings_[1][u1]; // for in direction
			for (auto it4 = item2->begin(); it4 != item2->end(); it4++){
				int key = it4->first;
				if (hop_map.find(key) == hop_map.end())
					continue;
				
				if (hop_map[key] + it4->second <= delta_){
					supports_[0][u0].push_back(u1);//out supports for u0
					supports_[1][u1].push_back(u0);//in supports for u1
					counts_new_[0][u0].find(u1);
					break;
				}
				
			}
		}
	}
	return count;
}
int TwohopAc::Sjoin(int method){
	int count = 0;

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		int label1 = it->first;
		int label2 = it->second;
		if (HASH_JOIN == method)
			count += HashJoin(label1, label2);

	}
	
	NewSupport *s[2];
	Count *c[2];

	while (true != deleted_queue_.empty()){
		int u = deleted_queue_.front();
		deleted_queue_.pop();

		s[0] = &supports_[0][u]; //left
		s[1] = &supports_[1][u]; //right
		c[0] = &counts_[0][u];   //left	
		c[1] = &counts_[1][u];   //right

		FOR_EACH_DIRECTION;
		for (auto it = s[dir]->begin(); it != s[dir]->end(); it++){
			c[OPP(dir)]--;
			if (0 == c[OPP(dir)]){
				int label = g_->vertices(*it)->label;
				g_->vertex(*it)->exist = false;
				group_->items(label)->erase(*it);
				deleted_queue_.push(*it);
				count++;
			}
		}
		END_FOR_EACH_DIRCTION;
	}
	return count;
}

int TwohopAc::SjoinNew(int method, int stage){
	int count = 0;

	if (1 == stage || 3 == stage){
		//step 1
		for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
			int label1 = it->first;
			int label2 = it->second;
			if (1 == method) {
				count += HashJoinNew(label1, label2);
			}
			else if (2 == method)
			{
				count += HashJoinNew2(label1, label2);
			}
		}
		COUT(<< "hash join " << count << " vertex deleted!" << endl);
		COUT(<< "deleted queue size  " << deleted_queue_.size() << endl);
	}
	if (2 == stage || 3 == stage) {
		//step 2
		while (true != deleted_queue_.empty())
		{
			int u = deleted_queue_.front();
			int u_label = g_->vertex(u)->label;
			deleted_queue_.pop();

			FOR_EACH_DIRECTION;
			auto s = &supports_[dir][u];
			for (auto it = s->begin(); it != s->end(); it++)
			{
				int s_u = *it;
				bool flag = false;
				//support count
				auto c = &counts_new_[OPP(dir)][s_u];
				if (c->find(u_label) != c->end()){
					c->at(u_label)--;
					if (0 == c->at(u_label)){
						flag = true;
					}
				}
				else{
					flag = true;
				}
				if (true == flag){
					int label = g_->vertex(s_u)->label;
					g_->vertex(s_u)->exist = false;//record delete
					group_->items(label)->erase(s_u); //delete this support from group
					deleted_queue_.push(s_u);
					count++;
				}
			}
			END_FOR_EACH_DIRCTION;
		}
		
	}
	COUT(<< "-----total delete " << count << endl;)
	return count;
}
/*without back proporgation for comparation */
int TwohopAc::SjoinNew2(int method, int stage){
	int count = 0;
	COUT(<<endl <<"************S-join 2 *************" << endl);
	if (1 == stage || 3 == stage){
		//step 1
		for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
			int label1 = it->first;
			int label2 = it->second;
			if (1 == method) {
				count += HashJoinNew(label1, label2);
			}
			else if (2 == method)
			{
				count += HashJoinNew2(label1, label2);
			}
		}
		COUT(<< "hash join " << count << " vertex deleted!" << endl);
		COUT(<< "deleted queue size  " << deleted_queue_.size() << endl);
	}
	//step2 iterate all the items in all other groups (list Ri)
	//for estimate iterate all the 
	if (2 == stage || 3 == stage){
		/*
		while (true != deleted_queue_.empty())
		{
			int u = deleted_queue_.front();
			int u_label = g_->vertex(u)->label;
			deleted_queue_.pop();

			FOR_EACH_DIRECTION;
			for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++)
			{
				auto group = group_->items(*it);
				for (auto it2 = group->begin(); it2 != group->end(); it2++){
					int u = *it2;
					for (auto it3 = supports_[dir][u].begin(); it3 != supports_[dir][u].end(); it3++)
					{
						int s_u = *it3;
					}
				}
			}
			
			END_FOR_EACH_DIRCTION;
		}
		*/
		//step 1
		
	BEGIN:
		count = 0;
		for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
			int label1 = it->first;
			int label2 = it->second;
			if (1 == method) {
				count += HashJoinNew(label1, label2);
			}
			else if (2 == method)
			{
				count += HashJoinNew2(label1, label2);
			}
		}
		if (count > 0){
			COUT(<< endl << "loop for checking!" << endl;)
			goto BEGIN;
		}

		COUT(<< "total delete " << count << endl;)
	}
	return count;
}
int TwohopAc::WriteMatches(const char *filename, char* delim){
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
			int vertex = *it;

			auto support_out = supports_[0][vertex];
			auto support_in = supports_[1][vertex];
			if (0 == support_out.size() && 0 == support_in.size())
				continue;
			// label, vertex, right support
			strbuf = _itoa(label, buf, 10); //label
			strbuf += delim;
			strbuf += _itoa(vertex, buf, 10); //center

			int c = counts_[0][vertex];
			strbuf += delim;
			strbuf += _itoa(c, buf, 10); //center
			for (auto it2 = support_out.begin(); it2 != support_out.end(); it2++){
				strbuf += delim;
				strbuf += "[";
				strbuf += _itoa(g_->vertices(*it2)->label, buf, 10); //support label
				strbuf += "-";
				strbuf += _itoa(*it2, buf, 10); //support vertex
				strbuf += "]";
			}

			c = counts_[1][vertex];
			strbuf += delim;
			strbuf += _itoa(c, buf, 10); //center
			for (auto it2 = support_in.begin(); it2 != support_in.end(); it2++){
				strbuf += delim;
				strbuf += "(";
				strbuf += _itoa(g_->vertices(*it2)->label, buf, 10); //support label
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

int TwohopAc::ResultConstruct(int start_u, int end_label){
	stack<int> stack;
	set<pair<int, int>> s_set; //in order to check re-visited supports.
	list<int> result;

	int mr_size = 0;

	stack.push(start_u);
	while (!stack.empty()){
		int u = stack.top();
		stack.pop();
		
		int lable = g_->vertex(u)->label;
		if (lable == end_label){
			mr_size++;
		}
		auto s = &supports_[0][u];
		//for each out-support
		for (auto it = s->rbegin(); it != s->rend(); it++){	
			int s_u = *it;
			//each edge for one visit
			if (s_set.find({ u, s_u })== s_set.end()){
				s_set.insert({ u, s_u });
				stack.push(s_u);
			}
		}
	}
	return mr_size;
}
int TwohopAc::ResultStart(){
	int count = 0;
	int start_label = q_->label_pairs()->begin()->first;
	int end_label = q_->label_pairs()->rbegin()->second;
	auto item = group_->items(start_label);
	for (auto it = item->begin(); it != item->end(); it++){
		int start_u = *it;
		count += ResultConstruct(start_u, end_label);
		//COUT(<< "result visit" << start_u << endl;)
	}
	COUT(<< "MR size " << count << endl);
	return 0;
}

int TwohopAc::ResultInfo(){
	//group size
	
	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		int group_size = 0;
		int support_size = 0; //out support size
		int support_size_2 = 0;

		int label = *it;
		auto item = group_->items(label);
		group_size += item->size();
		for (auto it2 = item->begin(); it2 != item->end(); it2++){
			int u = *it2;
			auto s = &supports_[0][u];
			support_size += s->size();
			s = &supports_[1][u];
			support_size_2 += s->size();
		}
		COUT(<< "	Label "<<label<<":"<<endl);
		COUT(<< "	result group size " << group_size << endl);
		COUT(<< "	out support size " << support_size << endl);
		COUT(<< "	in support size " << support_size_2 << endl);
		COUT(<< "	-----" << endl);
	}
	
	return 0;
}

int TwohopAc::PC(){
	
	PCEdges pc_edges;

	//complet graph Q
	PCGenerateCompleteQ(&pc_edges);
	
	//phase 1:
	PCSupports pc_supports;
	PCCount pc_counts;
	stack <pair<int, int>> stack; // <i, x> or <j, y>
	PCSupportValue pc_support_value;
	PCCountValue pc_count_value;
	int del_count = 0;

	COUT(<< "PC phase 1" << endl);
	for (auto it = pc_edges.begin(); it != pc_edges.end(); it++){ //R'ij
		int i = it->first.first;
		int j = it->first.second;
		for (auto it1 = q_->label_set()->begin(); it1 != q_->label_set()->end(); it1++){ //for k = 1 to n
			int k = *it1; //k
			if (k == i || k == j) continue;

			auto value = it->second;
			for (auto it2 = value.begin(); it2 != value.end(); it2++){ //for <x, y> in R'ij
				int num = 0;
				int x = it2->first;
				int y = it2->second;
				auto group = group_->items(k);
				for (auto it3 = group->begin(); it3 != group->end(); it3++){  // for each z in Rk
					int z = *it3;
					if (true == PCTri(&pc_edges, i, j, k, x, y, z)){
						if (pc_supports.find(z) == pc_supports.end())
						{
							pc_supports.insert({ z, pc_support_value });
						}
						array<int, 4> a = { i, j, x, y };
						pc_supports[z].push_back(a);
						num++;
					}
				}

				if (0 == num){
					pc_edges[{i, j}].erase({ x, y });
					stack.push({ i, x });
					stack.push({ j, y });
					if (0 == --counts_new_[0][x][j]) //remove vertices
					{
						group_->items(i)->erase(x);
						g_->vertex(x)->exist = false;
						del_count++;
					}
					if (0 == --counts_new_[0][y][i]){
						group_->items(i)->erase(y);
						g_->vertex(x)->exist = false;
						del_count++;
					}
				}
				else{
					if (pc_counts.find({ x, y }) == pc_counts.end()){
						pc_counts.insert({ { x, y }, pc_count_value});
					}
					pc_counts[{x, y}][k] = num;
				}
				
			}
		}
	}

	//phase 2: 
	COUT(<< "PC phase 2" << endl);
	while (false == stack.empty()){
		int k = stack.top().first;
		int z = stack.top().second;
		stack.pop();

		for (auto it = pc_supports[z].begin(); it != pc_supports[z].end(); it++){
			int i = it->at(0);
			int j = it->at(1);
			int x = it->at(2);
			int y = it->at(3);

			if (0 == (--pc_counts[{x, y}][k])){
				pc_edges[{i, j}].erase({ x, y });
				stack.push({ i, x });
				stack.push({ j, y });
				if (0 == --counts_new_[0][x][j]) //remove vertices
				{
					group_->items(i)->erase(x);
					g_->vertex(x)->exist = false;
					del_count++;
				}
				if (0 == --counts_new_[0][y][i]){
					group_->items(i)->erase(y);
					g_->vertex(x)->exist = false;
					del_count++;
				}

			}
		}
	}

	COUT(<< "PC del count: " << del_count << endl);
	return 0;
}

int TwohopAc::PCGenerateCompleteQ(PCEdges *pc_edges){
	//generate the complete graph of Query by adding vertual-edge.
	//get the original edges
	PCEdgeValue pc_edge_value;
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{
		int i = it->first;
		int j = it->second;	
	
		auto group0 = group_->items(i);
		auto group1 = group_->items(j);
		for (auto it2 = group0->begin(); it2 != group0->end(); it2++){
			int x = *it2;
			auto support = supports_[0][x];	//out support
			for (auto it3 = support.begin(); it3 != support.end(); it3++)
			{
				int y = *it3;
				//exist in opposit group
				if (group1->find(y) != group1->end()){
					if (pc_edges->find({ i, j }) == pc_edges->end())
					{		
						pc_edges->insert({ { i, j }, pc_edge_value });
						COUT(<< "create original edge: " << i << " " << j << endl);
					}
					pc_edges->at({i, j}).insert({ x, y });
					/*
					if (pc_edges->find({ j, i }) == pc_edges->end())
					{
						pc_edges->insert({ { j, i }, pc_edge_value });
						COUT(<< "create original edge: " << j << " " << i << endl);
					}
					pc_edges->at({ j, i }).insert({ y, x });
					*/
				}
			}
		}

	}

	//get the complete Query Graph
	multimap<int, int> edge_map; //one key may have more than one value.
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{
		edge_map.insert({ it->first, it->second});
		edge_map.insert({ it->second, it->first });
	}

	while (TRUE){
		bool has_new_edge = false;
		for (auto it = edge_map.begin(); it != edge_map.end(); it++){  // i k, k j ()
			int i = it->first;
			int k = it->second;
			if (edge_map.find(k) == edge_map.end()){
				continue;
			}
			 
			auto ret = edge_map.equal_range(k);
			for (auto it2 = ret.first; it2 != ret.second; it2++){
				int j = it2->second;

				if (i == j) continue;
				if (pc_edges->find({ i, j }) == pc_edges->end()){
					pc_edges->insert({ { i, j }, pc_edge_value });
					PCJoin(&pc_edges->at({ i, k }), &pc_edges->at({k,j}), &pc_edges->at({i, j}));
					edge_map.insert({ i, j });
					has_new_edge = true;
					COUT(<< "create original edge: " << i << " " << j << endl);
				}
				/*
				if (pc_edges->find({ j, i }) == pc_edges->end()){
					pc_edges->insert({ { j, i }, pc_edge_value });
					PCJoin(&pc_edges->at({ j, k }), &pc_edges->at({ k, i }), &pc_edges->at({ j, i }));
					edge_map.insert({ j, i });
					has_new_edge = true;
					COUT(<< "create original edge: " << j << " " << i << endl);
				}
				*/
			}
		}
		if (false == has_new_edge) //no new edge added to complete Q, then quit.
			break;
	}

	return 0;
}

int TwohopAc::PCJoin(PCEdgeValue *pcv0, PCEdgeValue *pcv1, PCEdgeValue *pcvnew){
	multimap<int, int> s;

	//create hash table
	for (auto it = pcv1->begin(); it != pcv1->end(); it++){
		s.insert({ it->first, it->second });
	}

	//creat pcvnew by hash table
	for (auto it = pcv0->begin(); it != pcv0->end(); it++){
		int key = it->second;
		if (s.find(key) != s.end()){
			auto ret = s.equal_range(key);
			for (auto it2 = ret.first; it2 != ret.second; it2++){
				pcvnew->insert({ it->first, it2->second });
			}
		}
	}

	s.clear();
	return 0;
}

int TwohopAc::PCTri(PCEdges *pc_edges, int i, int j, int k, int x, int y, int z)
{
	/*
	**
	*/
	int a[4][4] = { { i, k, k, j }, { i, k, j, k }, { k, i, k, j }, { k, i, j, k } };
	int b[4][4] = { { x, z, z, y }, { x, z, y, z }, { z, x, z, y }, { z, x, y, z } };
	
	for (int id = 0; id < 4; id++){
		if (pc_edges->find({ a[id][0], a[id][1] }) != pc_edges->end() &&
			pc_edges->find({ a[id][2], a[id][3] }) != pc_edges->end())
		{
			auto e0 = &pc_edges->at({ a[id][0], a[id][1] });
			auto e1 = &pc_edges->at({ a[id][2], a[id][3] });
			if (e0->find({ b[id][0], b[id][1] }) != e0->end() && 
				e1->find({ b[id][2], b[id][3] }) != e1->end()){
				return true;
			}
		}
	}
	return false;
}
