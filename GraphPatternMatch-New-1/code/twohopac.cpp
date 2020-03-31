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
	int arc_map_len = 0;
	for (auto it = arc_map.begin(); it != arc_map.end(); it++){
		arc_map_len += it->second.size();
	}
	float avg_len = float(arc_map_len) / float(arc_map.size());
	if (arc_map.size() > 0)
		COUT(<< "arc-map avg length " << avg_len << "	"<<arc_map_len<<" "<<arc_map.size()<<endl);

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
			COUT(<< " query edge: " << label1 << " " << label2 << endl);
			COUT(<< "  hash join " << count << " vertex deleted!" << endl);
			COUT(<< "  deleted queue size  " << deleted_queue_.size() << endl);
		}
		
	}
	if (2 == stage || 3 == stage) {
		//step 2
		while (true != deleted_queue_.empty())
		{
			int u = deleted_queue_.front();
			deleted_queue_.pop();
		
			int u_label = g_->vertex(u)->label;

			FOR_EACH_DIRECTION;
			auto s = &supports_[dir][u];
			for (auto it = s->begin(); it != s->end(); it++)
			{
				int s_u = *it;
				if (false == g_->vexist(s_u))// be careful, u
					continue;
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
					g_->vertex(s_u)->exist = false; //record delete, this is true delete
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

int TwohopAc::ResultInfo(const char * src_filename){
	//group size
	int total_out_support = 0;
	int total_out_support_2 = 0;
	int total_domain = 0;
	
	FILE *fp = NULL;
	string str, str2;
	int error;
	char strbuf[128] = { 0 };

	if (src_filename != NULL) {
		
		error = fopen_s(&fp, src_filename, "w");
		if (0 != error)
		{
			ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
			return -1;
		}
	}

	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		int group_size = 0;
		int support_size = 0; //out support size
		int support_size_2 = 0;
		int r_size = 0, r_size2 = 0;

		int label = *it;
		auto item = group_->items(label);
		group_size += item->size();
		for (auto it2 = item->begin(); it2 != item->end(); it2++){
			int u = *it2;
			auto s = &supports_[0][u];
			r_size += s->size();
			str = itoa(u, strbuf, 10);
			str += ",";
			for (auto it3 = s->begin(); it3 != s->end(); it3++){
				int s_u = *it3;
				if (g_->vexist(s_u)){
					support_size++;
					str2 = str + itoa(s_u, strbuf, 10);
					str2 += "\n";
					if (NULL != fp){
						fwrite(str2.data(), sizeof(char), str2.length(), fp);
					}
				}
			}
			s = &supports_[1][u];
			support_size_2 += s->size();
			r_size2 += s->size();
		}
		//COUT(<< "	Label "<<label<<":"<<endl);
		//COUT(<< "	result group size " << group_size << endl);
		total_domain += group_size;
		COUT(<< "	out support size " << support_size <<" " << r_size << endl);
		auto c = &counts_new_[0][label]; // new counters
		for (auto it2 = c->begin(); it2 != c->end(); it2++){
			COUT(<< "	- out support " << it2->first << " : " << it2->second << endl);
		}
		//COUT(<< "	in support size " << support_size_2 <<" " << r_size2 << endl);
		total_out_support += support_size;
		total_out_support_2 += support_size_2;
		COUT(<< "	-----" << endl);
	}
	COUT(<< "  ****Total Domain: " << total_domain << " Total outsupport: " << total_out_support << " "<< total_out_support_2 <<endl);
	COUT(<< "	-----" << endl);
	
	if (NULL != fp)
		fclose(fp);
	
	return 0;
}

int TwohopAc::PC(){
	
	//complet graph Q
	Relations *pc_edges = &relations_;
	//PCGenerateQ();
	

	//phase 1:
	PCSupports pc_supports;
	PCCount pc_counts;
	set<pair<int, int>> stack; // <i, x> or <j, y>
	PCSupportValue pc_support_value;
	PCCountValue pc_count_value;
	int del_count = 0;
	int del_r_count = 0;

	set<set<int>> sset;
	COUT(<< "PC phase 1" << endl);
	for (auto it = pc_edges->begin(); it != pc_edges->end(); it++){ //R'ij not virtual edge
		if (it->second.size() == 0) continue;

		int i = it->first.first;
		int j = it->first.second;
		
		//jump over all virtual edges
		if (pc_edges->find({ i, j }) == pc_edges->end() || pc_edges->find({ j, i }) == pc_edges->end())
			continue;

		for (auto it1 = q_->label_set()->begin(); it1 != q_->label_set()->end(); it1++){ //for k = 1 to n
			int k = *it1; //k
			
			if (k == i || k == j) 
				continue;

			//jump over all the virtural edges
			if (!
				((pc_edges->find({ i, k }) != pc_edges->end() && pc_edges->find({ k, j }) != pc_edges->end()) ||
				(pc_edges->find({ i, k }) != pc_edges->end() && pc_edges->find({ j, k }) != pc_edges->end()) ||
				(pc_edges->find({ k, i }) != pc_edges->end() && pc_edges->find({ k, j }) != pc_edges->end()) ||
				(pc_edges->find({ k, i }) != pc_edges->end() && pc_edges->find({ j, k }) != pc_edges->end())
				)
				){
				continue;
			}

			//COUT(<< "	check i:" << i<< " j:" << j << " k:" << k << endl);
			Relation *value = &it->second;
			for (auto it2 = value->begin(); it2 != value->end();){ //for <x, y> in R'ij
				int num = 0;
				int x = it2->first;
				int y = it2->second;
				auto group = group_->items(k);
				for (auto it3 = group->begin(); it3 != group->end(); it3++){  // for each z in Rk
					int z = *it3;
					if (true == PCTri(pc_edges, i, j, k, x, y, z)){
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
					value->erase(it2++);

					if (0 >= --counts_new_[0][x][j]) //remove vertices
					{
						auto g = group_->items(i);// x in x
						g->erase(x);
						g_->vertex(x)->exist = false;
						stack.insert({ i, x });  //*******important
						del_r_count++;

						del_count++;
					}
					
					if (0 >= --counts_new_[1][y][i]){
						auto g = group_->items(j);
						g->erase(y);
						g_->vertex(y)->exist = false;
						stack.insert({ j, y });
						del_r_count++;

						del_count++;
					}
				}
				else{
					it2++;

					if (pc_counts.find({ x, y }) == pc_counts.end()){
						pc_counts.insert({ { x, y }, pc_count_value});
					}
					pc_counts[{x, y}][k] += num;
				}
				
			}
		}
	}

	//phase 2: 
	COUT(<< "PC phase 2" << endl);
	while (false == stack.empty()){
		auto it = stack.begin();
		int k = it->first;
		int z = it->second;
		stack.erase(it);

		for (auto it = pc_supports[z].begin(); it != pc_supports[z].end(); it++){
			int i = it->at(0);
			int j = it->at(1);
			int x = it->at(2);
			int y = it->at(3);

			if (0 >= (--pc_counts[{x, y}][k])){
				pc_edges->at({i, j}).erase({ x, y });

				del_r_count++;
				if (0 == --counts_new_[0][x][j]) //remove vertices
				{
					group_->items(i)->erase(x);
					g_->vertex(x)->exist = false;
					stack.insert({ i, x }); //**********important
					del_count++;
				}
				if (0 == --counts_new_[0][y][i]){
					group_->items(i)->erase(y);
					g_->vertex(x)->exist = false;
					stack.insert({ j, y });//************important
					del_count++;
				}

			}
		}
	}

	
	COUT(<< "PC del vertex count: " << del_count << "  relation count:" << del_r_count << endl);
	int total_relation = 0;
	for (auto it = pc_edges->begin(); it != pc_edges->end(); it++){
		auto e = it->first;
		auto size = it->second.size();
		total_relation += size;
		COUT(<< "	edge: " << e.first << "-" << e.second << " size: " << size << endl);
	}
	COUT(<< "	total relation: " << total_relation);
	COUT(<< endl);
	return 0;
}

int TwohopAc::PCGenerateQ(){
	//generate the complete graph of Query by adding vertual-edge.
	//get the original edges
	int count = 0;
	Relations *pc_edges = &relations_;

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{
		int i = it->first;
		int j = it->second;	
		
		if (pc_edges->find({ i, j }) == pc_edges->end())
		{
			PCEdgeValue pc_edge_value;
			pc_edges->insert({ { i, j }, pc_edge_value });
			//COUT(<< "create original edge: " << i << " " << j << endl);
		}

		auto group0 = group_->items(i);
		auto group1 = group_->items(j);
		for (auto it2 = group0->begin(); it2 != group0->end(); it2++){
			int x = *it2;
			auto support = &supports_[0][x];	//out support
			for (auto it3 = support->begin(); it3 != support->end(); it3++)
			{
				int y = *it3;
				//exist in opposit group
				if (group1->find(y) != group1->end()){
					pc_edges->at({i, j}).insert({ x, y });
				}
			}
		}
		count += pc_edges->at({ i, j }).size();
		COUT(<< " PC edge " << i << ", " << j << " relation: " << pc_edges->at({ i, j }).size()) << endl;;

	}

	COUT(<< "==== Total relation: " << count << endl);
	COUT(<< endl);
	
	return 0;
}

int TwohopAc::PCTri(PCEdges *pc_edges, int i, int j, int k, int x, int y, int z)
{
	/*
	**
	*/
	int a[4][4] = { { i, k, k, j }, { i, k, j, k }, { k, i, k, j }, { k, i, j, k } };
	int b[4][4] = { { x, z, z, y }, { x, z, y, z }, { z, x, z, y }, { z, x, y, z } };
	bool result[4];
	for (int id = 0; id < 4; id++){
		bool Rleft = true;
		bool Rright = true;
		if (pc_edges->find({ a[id][0], a[id][1] }) != pc_edges->end())
		{
			auto e0 = &pc_edges->at({ a[id][0], a[id][1] });
			if (e0->find({ b[id][0], b[id][1] }) == e0->end())
				Rleft = false;
		}
		if (pc_edges->find({ a[id][2], a[id][3] }) != pc_edges->end())
		{
			auto e0 = &pc_edges->at({ a[id][2], a[id][3] });
			if (e0->find({ b[id][2], b[id][3] }) == e0->end())
				Rright = false;
		}
		//if (Rleft && Rright == true) // one of four cases holds.
			//return true;
		result[id] = Rleft && Rright;
	}
	for (int id = 0; id < 4; id++){
		if (false == result[id])
			return false;
	}
	return true;
}


/*relation graph */
int TwohopAc::ReadRelationGraph(const char *filename, const char *delim){
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
		array<int, 4> temp = { value[0], value[1], value[2], value[3] };
		r_edges_.push_back(temp);
		array<int, 2> temp2 = { value[0], value[1]};
		r_vertices_.insert(temp2);
	}
	fclose(fp);
	return 0;
}

int TwohopAc::RelationConstruct(int method)
{
	int count = 0;
	int c = 0;
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{
		//COUT(<< "Relation Construct:" << it->first << " " << it->second << endl);
		switch (method)
		{
		case 1:
			c = RelationConstructEachHash(it->first, it->second);
			break;
		case 2:
			c = RelationConstructEachNaive(it->first, it->second);
			break;
		}

		count += c;
		COUT(<< "	relation " << it->first << ","<< it->second <<": " << c << endl);
	}

	COUT(<< "== Total Relation: " << count << endl);
	COUT(<< endl);
	return 0;
}

int TwohopAc::RelationClear()
{
	for (auto it = relations_.begin(); it != relations_.end(); it++)
	{
		it->second.clear();
	}
	relations_.clear();
	return 0;
}
int TwohopAc::RelationConstructEachHash(int label0, int label1, double *time){
	__int64 t1, t2;
	t1 = MyTimer();

	int count = 0;
	GroupItems *mygroup[2];
	int l, r, l2, r2;

	mygroup[0] = group_->items(label0); //left
	mygroup[1] = group_->items(label1); //right

	l = 0; 	r = 1; l2 = 0; r2 = 1;
	if (mygroup[0]->size() < mygroup[1]->size()){
		swap(mygroup[0], mygroup[1]);
		l = 1; r = 0;
		l2 = 1; r2 = 0;
	}

	if (relations_.find({ label0, label1 }) == relations_.end()){
		Relation relation;
		relations_.insert({ { label0, label1 }, relation });
	}
	else{
		return count;
	}
	auto relation = &relations_.at({ label0, label1 });

	//key: center. value: array[0] center, array[1] dist, ,
	map<int, list<array<int, 2>>> arc_map;
	list <array<int, 2>> arc_map_list; //{vertex, dist}

	/*for left pruning, build arc map for right. dir = 0 left*/
	for (auto it1 = mygroup[r]->begin(); it1 != mygroup[r]->end(); it1++){
		int vertex = *it1;
		auto items = &labelings_[r2][vertex]; //2-hop. 0 out, 1 in.
		for (auto it2 = items->begin(); it2 != items->end(); it2++){
			int center = it2->first;
			int dist = it2->second;
			if (dist > delta_)
				break; //this is pruning method based on the sorted labeling by weight

			array<int, 2> a = { vertex, dist };
			arc_map[center].push_back(a);
		}
	}

#if 0
	//COUT(<< "	arc-map size " << arc_map.size() << endl);
	int arc_map_len = 0;
	for (auto it = arc_map.begin(); it != arc_map.end(); it++){
		arc_map_len += it->second.size();
	}
	float avg_len = float(arc_map_len) / float(arc_map.size());
#endif

	if (arc_map.size() > 0){
		//COUT(<< "	arc-map avg length " << avg_len << "	" << arc_map_len << " " << arc_map.size() << endl);
	}
	else{
		goto MYOUT;
	}

	/*sort arc_map*/
	auto fun = [](array<int, 2> a, array<int, 2> b){
		return a[1] < b[1]; //sort by distance
	};
	for (auto it1 = arc_map.begin(); it1 != arc_map.end(); it1++){
		it1->second.sort(fun);
	}
	
	//iterate left group
	for (auto it1 = mygroup[l]->begin(); it1 != mygroup[l]->end();it1++){
		int vertex = *it1;
		auto item = &labelings_[l2][vertex]; //out labeling
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

		if (false == support_temp.empty()){
			for (auto it3 = support_temp.begin(); it3 != support_temp.end(); it3++)
			{
				array<int, 2> temp = { vertex, *it3 };
				//relation->push_back(temp);
				relation->insert({ vertex, *it3 });
				count++;
			}
			
		}

	}
	arc_map.clear();

	//COUT(<< "relation " << label0 << " " << label1 << "  :" << relation->size() << endl);
MYOUT:
	t2 = MyTimer();
	if (time != NULL)
		*time += MyTime(t1, t2);
	return count;
}

int TwohopAc::RelationConstructEachNaive(int label0, int label1, double *time){
	int count = 0;
	GroupItems *mygroup[2];
	mygroup[0] = group_->items(label0); //left
	mygroup[1] = group_->items(label1); //right

	if (relations_.find({ label0, label1 }) == relations_.end()){
		Relation relation;
		relations_.insert({ { label0, label1 }, relation });
	}
	else{
		return count;
	}
	auto r = &relations_.at({ label0, label1 });

	map<int, int> map;

	/*for left pruning, build arc map for right. dir = 0 left*/
	for (auto it0 = mygroup[0]->begin(); it0 != mygroup[0]->end(); it0++){
		int u0 = *it0;
		for (auto it1 = mygroup[1]->begin(); it1 != mygroup[1]->end(); it1++){
			int u1 = *it1;
			auto items0 = &labelings_[0][u0]; //2-hop. 0 out, 1 in.
			auto items1 = &labelings_[1][u1];

			map.clear();

			for (auto it2 = items1->begin(); it2 != items1->end(); it2++){
				int center = it2->first;
				int dist = it2->second;
				if (dist > delta_)
					break; //this is pruning method based on the sorted labeling by weight
				map.insert({ center, dist });
			}

			if (true == map.empty())
				continue;

			for (auto it3 = items0->begin(); it3 != items0->end(); it3++){
				int center = it3->first;
				int dist = it3->second;
				if (dist > delta_)
					break; //this is pruning method based on the sorted labeling by weight
				if (map.find(center) != map.end())
				{
					if (dist + map[center] <= delta_)
					{
					
						r->insert({ u0, u1 });
						count++;
						break; // find relation break;
					}
				}
			}
		}
	}

	return count;
}
#if 0
int TwohopAc::EqualJoin()
{
	int count = 0;
	int loopnum = 1;
AGAIN:
	bool change = false;
	for (auto it = r_edges_.begin(); it != r_edges_.end(); it++){
		auto a = *it;
		
		auto relation0 = &relations_[{a[0], a[1]}];
		auto relation1 = &relations_[{a[2], a[3]}];
		int l, r;
		GetJoinIndex(a, &l, &r);

		COUT(<< "	equal join  " << a[0] << " "<<a[1]<< " and "<< a[2] << " "<<a[3] <<endl);
		COUT(<< "	left " << l << " right " << r << endl);
		COUT(<< "	   Relation " << relation0->size() << " " << relation1->size() << endl);
	

		//hash join
		set<int> set;
		//right
		for (auto it2 = relation1->begin(); it2 != relation1->end(); it2++)
		{
			set.insert(it2->at(r));
		}
		//left
		for (auto it2 = relation0->begin(); it2 != relation0->end();)
		{
			int v = it2->at(l);
			if (set.find(v) == set.end()){
				relation0->erase(it2++);
				count++;
				change = true;
			}
			else{
				it2++;
			}
		}
		set.clear();
		//left
		for (auto it2 = relation0->begin(); it2 != relation0->end(); it2++)
		{
			set.insert(it2->at(l));
		}
		//right
		for (auto it2 = relation1->begin(); it2 != relation1->end();)
		{
			int v = it2->at(r);
			if (set.find(v) == set.end()){
				relation1->erase(it2++);
				count++;
				change = true;
			}
			else{
				it2++;
			}
		}
		COUT(<< "		Result Relation " << relation0->size() << " " << relation1->size() << endl);
	}
	COUT(<< " equal join loop: " << loopnum );
	COUT(<< "	del relation: " << count << endl);
	if (true == change)
	{
		loopnum++;
		goto AGAIN;
	}

	return loopnum;
}
#endif
int TwohopAc::GetJoinIndex(array<int, 4> a, int *left, int *right){
	int l = -1, r = -1;
	if (a[0] == a[2])
	{
		l = 0;
		r = 0;
	}
	else if (a[0] == a[3])
	{
		l = 0;
		r = 1;
	}
	else if (a[1] == a[2])
	{
		l = 1;
		r = 0;
	}
	else if (a[1] == a[3])
	{
		l = 1;
		r = 1;
	}
	
	*left = l;
	*right = r;

	if (-1 == l || -1 == r)
		return -1;
	else
		return 0;
}
#if 0
int TwohopAc::EqualJoin2()
{
	int count = 0;
	RelationGraphVertices vs;
	RelationGraphVertex v;

	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{
		RelationGraphVertex a = { it->first, it->second };
		
		count += EqualJoin2Check(&vs, &a);
		vs.insert(a);
	}
	COUT(<<endl<< "==== del relation: " << count << endl);
	return 0;
}
#endif
int TwohopAc::EqualJoin3Before()
{
	return 0;
}
int TwohopAc::EqualJoin3()
{	

	NJMR mr(kQueryVertexNum, -1);
	mrs_.clear();
	/*first query edge*/
	auto it = q_->label_join_orders()->begin();
	int v0 = it->first.first;
	int v1 = it->first.second;
	int type = it->second;

	if (FORWARD_EDGE == type){
		Relation *r = &relations_[{v0, v1}];
		if (0 == r->size())
			return 0;
		for (auto it2 = r->begin(); it2 != r->end(); it2++){
			mr[v0] = it2->first;
			mr[v1] = it2->second;
			mrs_.push_back(mr);
		}
		COUT(<< "	equal join on: " << v0 << "," << v1 << " insert: " << r->size() << endl);
	}

	if (q_->label_join_orders()->begin() + 1 == q_->label_join_orders()->end())
		return -1;
	
	for (auto it = q_->label_join_orders()->begin()+1; it != q_->label_join_orders()->end(); it++){
		v0 = it->first.first;
		v1 = it->first.second;
		type = it->second;
		Relation *r = &relations_[{v0, v1}];

		if (FORWARD_EDGE == type){
			auto it2 = mrs_.begin();
			int v = v0; //choosed vertex
			/*
			if (it2 != mrs_.end())
			{
				if (-1 != it2->at(v0))
					v = v0;
				else if (-1 != it2->at(v1))
					v = v1;
			}
			*/
			int count = NaturalJoin(&mrs_, r, v0, v1, v);
			COUT(<< "	equal join on: " << v0 << "," << v1 << " insert: " << count << endl);
		}
		else if (BACKWARD_EDGE == type){ //backward edge
			int count = 0;
			set<pair<int, int>> set;
			for (auto it2 = r->begin(); it2 != r->end();it2++)
			{
				set.insert({ it2->first, it2->second });
			}
			
			for (auto it3 = mrs_.begin(); it3 != mrs_.end();)
			{
				//it3 is mr
				pair<int, int> temp = { it3->at(v0), it3->at(v1) };
				if (set.find(temp) == set.end()){
					mrs_.erase(it3++);
					count++;
				}
				else
				{
					it3++;
				}
			}

			COUT(<< "	equal join on: " << v0 << "," << v1 << " delete: " << count << endl);
		}
	}
	return mrs_.size();
}

int TwohopAc::EqualJoin4(){
	int count = 0;

	NJMR mr;
	for (int i = 0; i < q_->label_set()->size(); i++){
		mr.push_back(-1);
	}
	list<pair<int, int>> forward_edges;
	int v0, v1, v2, v;
	map<int, stack<int>> mstack; //key is group label, value is stack
	
	mrs_.clear();

	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		stack<int> s;
		mstack.insert({ *it, s });
	}

	//only for forward edge
	for (auto it = q_->label_join_orders()->begin(); it != q_->label_join_orders()->end(); it++){
		if (FORWARD_EDGE == it->second){
			forward_edges.push_back({it->first.first, it->first.second});
		}
	}


	//for first edge
	auto it = forward_edges.begin();
	v0 = it->first;
	v1 = it->second;

	Relation *r = &relations_[{v0, v1}];
	set<int> set;
	for (auto it = r->rbegin(); it != r->rend(); it++){
		int u0 = it->first;
		set.insert(u0);
	}
	for (auto it = set.rbegin(); it != set.rend(); it++){
		mstack[v0].push(*it);
	}
	set.clear();
	
	v = v0;
	// 
	int insert_count = 0;
	auto itr = forward_edges.rbegin();
	for (auto it = forward_edges.begin(); it != forward_edges.end();)
	{	
		v0 = it->first;
		v1 = it->second;
		Relation *r = &relations_[{v0, v1}];
		
		if (true == mstack[v].empty()) //the stack is empty
			break;

		if (false == mstack[v0].empty())
		{
			int u0 = mstack[v0].top();
			mstack[v0].pop();
			mr[v0] = u0;

			auto itlow = r->lower_bound({ u0, 0 });
			auto itup = r->upper_bound({ u0, 0x00ffffff });

			for (auto it2 = itlow; it2 != itup; it2++)
			//for (auto it2 = r->begin(); it2 != r->end(); it2++)
			{
				//if (it2->first != u0)
					//continue;
				int u1 = it2->second;
				insert_count++;
				//if (mr.size() == forward_edges.size()){ // find the last v
				if (*itr == *it) //last edge
				{
					mr[v1] = u1;
					if (EqualJoin4CheckFun(&mr)){
						//mrs_.push_back(mr);
						count++;
					}
					
				}
				else
				{
					mstack[v1].push(u1);
				}
			}

			if (*itr != *it){
				it++;
			}
		}
		else{ //is empty, back to forward edge	
			it--;
		}

	}

	COUT(<< "		equal join 4 insert count " << insert_count << endl);
		//check by back edge
		//return mrs_.size();
	return count;
}

int TwohopAc::EqualJoin5()
{
	int count = 0;

	NJMR mr(q_->label_set()->size(), -1);
	list<pair<int, int>> forward_edges, backward_edges_temp, backward_edges;
	int v0, v1, v2, v;
	map<int, stack<int>> mstack; //key is group label, value is stack

	mrs_.clear();

	for (auto it = q_->label_set()->begin(); it != q_->label_set()->end(); it++){
		stack<int> s;
		mstack.insert({ *it, s });
	}

	//only for forward edge
	for (auto it = q_->label_join_orders()->begin(); it != q_->label_join_orders()->end(); it++){
		pair<int, int> a = { it->first.first, it->first.second };
		if (FORWARD_EDGE == it->second){
			forward_edges.push_back(a);
		}
		else if (BACKWARD_EDGE == it->second){
			backward_edges.push_back(a);
		}
	}

	backward_edges_temp = backward_edges;


	//for first edge
	auto it = forward_edges.begin();
	v0 = it->first;
	v1 = it->second;

	Relation *r = &relations_[{v0, v1}];
	set<int> set;
	for (auto it = r->rbegin(); it != r->rend(); it++){
		int u0 = it->first;
		set.insert(u0);
	}
	for (auto it = set.rbegin(); it != set.rend(); it++){
		mstack[v0].push(*it);
	}
	set.clear();

	v = v0;
	// 
	int insert_count = 0;
	auto itr = forward_edges.rbegin();
	for (auto it = forward_edges.begin(); it != forward_edges.end();)
	{
		v0 = it->first;
		v1 = it->second;
		Relation *r = &relations_[{v0, v1}];

		if (true == mstack[v].empty()) //the stack is empty
			break;

		if (false == mstack[v0].empty())
		{
			int u0 = mstack[v0].top();
			mstack[v0].pop();
		
			//renew the backward_edge
			if (v0 == v){
				backward_edges_temp = backward_edges;
				fill(mr.begin(),mr.end(), -1);
			}

			mr[v0] = u0;

			if (false == EqualJoin5CheckFun(&mr, &backward_edges_temp))
				continue;

		
			auto itlow = r->lower_bound({ u0, 0 });
			auto itup = r->upper_bound({ u0, 0x00ffffff });

			for (auto it2 = itlow; it2 != itup; it2++)
			{
				int u1 = it2->second;
				insert_count++;
				if (*itr == *it) //last edge
				{
					mr[v1] = u1;
					if (EqualJoin4CheckFun(&mr)){
						count++;
					}

				}
				else
				{
					mstack[v1].push(u1);
				}
			}

			if (*itr != *it){
				it++;
			}
		}
		else{ //is empty, back to forward edge	
			it--;
		}

	}

	COUT(<< "		equal join 4 insert count " << insert_count << endl);
	//check by back edge
	//return mrs_.size();
	return count;
}

bool TwohopAc::EqualJoin5CheckFun(NJMR *mr, list<pair<int, int>> *backward_edges){
	bool res = true;

	for (auto it = backward_edges->begin(); it != backward_edges->end();)
	{
		int v0 = it->first;
		int v1 = it->second;
	
		if (-1 == mr->at(v0) || -1 == mr->at(v1))
		{
			it++;
			continue;
		}

		Relation *r = &relations_[{v0, v1}];
		backward_edges->erase(it++);
		if (r->find({ mr->at(v0), mr->at(v1) }) == r->end()){
			res = false; // not matched
			return res;
		}
	}

	return res;
}

bool TwohopAc::EqualJoin4CheckFun(NJMR *mr){

	for (auto it = q_->label_join_orders()->begin(); it != q_->label_join_orders()->end(); it++)
	{
		int v0 = it->first.first;
		int v1 = it->first.second;
		int type = it->second;

		if (BACKWARD_EDGE != type){
			continue;
		}

		Relation *r = &relations_[{v0, v1}];
		if (r->find({ mr->at(v0), mr->at(v1) }) == r->end()){
			return false; // not matched
		}
	}

	return true;
}

int TwohopAc::WriteNaturalJoin(const char *filename){
	int error;
	FILE *fp = NULL;
	if (filename == NULL) {
		return -1;
	}
	char buf[128];

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << filename << " " << strerror(error) << endl;);
		return -1;
	}
	for (auto it = mrs_.begin(); it != mrs_.end(); it++){
		string str = itoa(*it->begin(), buf, 10);
		for (auto it2 = it->begin() + 1; it2 != it->end(); it2++)
		{
			str += ",";
			str += itoa(*it2, buf, 10);
		}
		str += "\n";
		fwrite(str.data(), sizeof(char), str.length(), fp);
	}
	if (filename != NULL)
		fclose(fp);
	mrs_.clear();
}
int TwohopAc::NaturalJoin(NJMRs *mrs, Relation *r, int v0, int v1,  int v)
{
	//step 1; built map
	multimap<int, int> mmap;
	mmap.clear();

	for (auto it = r->begin(); it != r->end(); it++){
		if (v == v0)
			mmap.insert({ it->first, it->second });
		else if (v == v1)
			mmap.insert({ it->second, it->first });
	}
	
	NJMRs tempmrs;
	int total_num = 0;
	for (auto it = mrs->begin(); it != mrs->end(); it++){
		int u = it->at(v);
		if (mmap.find(u) == mmap.end())
			continue;
		int num = 0;
		auto ret = mmap.equal_range(u);
		for (auto it2 = ret.first; it2 != ret.second; it2++){
			int u_ = it2->second;
			int v_temp = (v != v0 ? v0 : v1);
			if (0 == num){ //first match
				it->at(v_temp) = u_;
				num++;
			}
			else //another
			{
				NJMR mr = *it;
				mr[v_temp] = u_;
				tempmrs.push_back(mr);
				num++;
			}
		}
	}

	total_num = tempmrs.size();
	mrs->insert(mrs->end(), tempmrs.begin(), tempmrs.end());
	tempmrs.clear();
	return total_num;
}
#if 0
int TwohopAc::EqualJoin2Check(RelationGraphVertices *vs, RelationGraphVertex *v){
	int count = 0;
	array<int, 4> a;
	int l, r;
	int loopnum = 1;
	set<int> set;
AGAIN:
	bool change = false;
	for (auto it = vs->begin(); it != vs->end(); it++){
		a[0] = it->at(0);
		a[1] = it->at(1);
		a[2] = v->at(0);
		a[3] = v->at(1);
	
		if (-1 == GetJoinIndex(a, &l, &r))
			continue; // jump over the not connected ones.

		auto relation0 = &relations_[{a[0], a[1]}];
		auto relation1 = &relations_[{a[2], a[3]}];
		
		//COUT(<< "	equal join  " << a[0] << " " << a[1] << " and " << a[2] << " " << a[3] << endl);
		//COUT(<< "	left " << l << " right " << r << endl);
		//COUT(<< "	   Relation " << relation0->size() << " " << relation1->size() << endl);

		//hash join
		set.clear();
		//right
		for (auto it2 = relation1->begin(); it2 != relation1->end(); it2++)
		{
			set.insert(it2->at(r));
		}
		//left
		for (auto it2 = relation0->begin(); it2 != relation0->end();)
		{
			int v = it2->at(l);
			if (set.find(v) == set.end()){
				relation0->erase(it2++);
				change = true;
				count++;
			}
			else{
				it2++;
			}
		}
		set.clear();
		//left
		for (auto it2 = relation0->begin(); it2 != relation0->end(); it2++)
		{
			set.insert(it2->at(l));
		}
		//right
		for (auto it2 = relation1->begin(); it2 != relation1->end();)
		{
			int v = it2->at(r);
			if (set.find(v) == set.end()){
				relation1->erase(it2++);
				change = true;
				count++;
			}
			else{
				it2++;
			}
		}

		//join
		//COUT(<< "		Result Relation " << relation0->size() << " " << relation1->size() << endl);
		//COUT(<< "==== del relation: " << count << endl);
	}
	if (true == change)
	{
		loopnum++;
		goto AGAIN;
	}
	
	return count;
}

#endif
int TwohopAc::AC(int method, double *relation_construct_time){
	int count = 0;
	int count2 = 0;
	Relation *r;
	multimap<int, int> mmap;
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++){
		
		int label[2] = { it->first, it->second };

		if (2 == method)
		{
			RelationConstructEachHash(label[0], label[1], relation_construct_time);
		}
		Relation *r = &relations_[{label[0], label[1]}];
		GroupItems *mygroup[2];
		mygroup[0] = group_->items(label[0]); //left
		mygroup[1] = group_->items(label[1]); //right

		//create support
		FOR_EACH_DIRECTION;
		mmap.clear();
		for (auto it2 = r->begin(); it2 != r->end(); it2++){
			if (0 ==dir)
				mmap.insert({ it2->first, it2->second }); // 0 , 1
			else
				mmap.insert({ it2->second, it2->first }); // 0 , 1
		}

		for (auto it3 = mygroup[dir]->begin(); it3 != mygroup[dir]->end();){ // 0
			int v = *it3;
			if (mmap.find(v) != mmap.end()){
				auto ret = mmap.equal_range(v);
				for (auto it4 = ret.first; it4 != ret.second; it4++){
					int v1 = it4->second;
					supports_[dir][v].push_back(v1); // 0
					counts_new_[dir][*it3][label[OPP(dir)]]++;
				}

				it3++;
			}
			else
			{
				mygroup[dir]->erase(it3++); // 0
				deleted_queue_.push(v);
				count++;
				/****delete here*****/
				//count += ACDeleted();
			}
		}
		END_FOR_EACH_DIRCTION;

		if (2 == method){
			relations_[{label[0], label[1]}].clear();
			relations_.erase({ label[0], label[1] });
		}

	}

	count += ACDeleted();

	return count;
}

int TwohopAc::ACDeleted(){
	int count = 0;
	while (true != deleted_queue_.empty())
	{
		int u = deleted_queue_.front();
		deleted_queue_.pop();

		int u_label = g_->vertex(u)->label;

		FOR_EACH_DIRECTION;
		auto s = &supports_[dir][u];
		for (auto it = s->begin(); it != s->end(); it++)
		{
			int s_u = *it;
			if (false == g_->vexist(s_u))// be careful, u
				continue;
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
				g_->vertex(s_u)->exist = false; //record delete, this is true delete
				group_->items(label)->erase(s_u); //delete this support from group
				deleted_queue_.push(s_u);
				count++;
			}
		}
		END_FOR_EACH_DIRCTION;
	}
	return count;
}

int TwohopAc::AfterACPCRationConstruct()
{
	int count = 0;
	int count2 = 0;

	relations_.clear();
	for (auto it = q_->label_pairs()->begin(); it != q_->label_pairs()->end(); it++)
	{

		int v0 = it->first;
		int v1 = it->second;

		if (relations_.find({ v0, v1 }) == relations_.end()){
			Relation relation;
			relations_.insert({ { v0, v1 }, relation });
		}
		auto r = &relations_.at({ v0, v1 });

		auto g0 = group_->items(v0);
		auto g1 = group_->items(v1);
		
		count2 = 0;

		//iterater all the out support of g0
		for (auto it2 = g0->begin(); it2 != g0->end(); it2++){
			int u0 = *it2;
			auto s = &supports_[0][u0];
			for (auto it3 = s->begin(); it3 != s->end();it3++){
				int u1 = *it3;
				if (g1->find(u1) != g1->end()){
					r->insert({u0, u1});
					count2++;
				}
			}
		}
		COUT(<< "	Relation " << v0 << " " << v1 << ": " << r->size() << endl);
		count += count2;

	}
	COUT(<< "==== total relation " << count << endl);
	return count;
}


int TwohopAc::WriteRelation(const char *filename){
	int error;
	FILE *fp = NULL;
	if (filename == NULL) {
		return -1;
	}
	char buf[128];

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << filename << " " << strerror(error) << endl;);
		return -1;
	}
	for (auto it = relations_.begin(); it != relations_.end(); it++){
		string str = itoa(it->first.first, buf, 10);
		str += ",";
		str += itoa(it->first.second, buf, 10);
		for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++){
			string str2 = str;
			str2 += ",";
			str2 += itoa(it2->first, buf, 10);
			str2 += ",";
			str2 += itoa(it2->second, buf, 10);
			str2 += "\n";
			fwrite(str2.data(), sizeof(char), str2.length(), fp);
		}
		
		
	}
	if (filename != NULL)
		fclose(fp);
}