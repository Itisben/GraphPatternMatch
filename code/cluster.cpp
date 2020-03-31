#include "cluster.h"

Cluster::Cluster(Embedding *embedding, Group *group){
	embedding_ = embedding;
	group_ = group;
	size_ = group_->label_num();
	debug_ = 0;

	ClusterUnits cluster_units;
	group_clusters_.insert(group_clusters_.begin(), size_, cluster_units);
}
Cluster::~Cluster(){ }

ERR Cluster::KMedoids(int k, int max_iterations, int cluster_size){
	int err = 0;
	int k_new = k;
	for (int i = 0; i < group_->label_num(); i++){
		int group_size = (int)group_->items(i)->size();
		if (-1 == k){ //default value
			k_new = group_size / cluster_size;
		}
		/*init, the k cannot be 0*/
		if (k > group_size){
			k_new = group_size;
		}
		/*k cannot be 0*/
		if (0 == k_new){
			ERR_COUT(<< "the k cannot be 0 in K-medoids" << endl;);
			return -1;
		}

		err  += KMedoidsGroup(i, k_new, max_iterations);
	}
	return err;
}

ERR Cluster::KMedoidsGroup(int label, int k, int max_iterations){
	GroupItems *group_items = group_->items(label);
	Rk *rk = embedding_->rks(label);
	ClusterUnits *cluster_units = group_clusters(label);
	ClusterUnit cluster_unit = {0};

	RandObj rand_obj(group_items);

	/*arbitrarily choose k object as seed*/
	vector<int> rand_range;
	for (auto it = group_items->begin(); it != group_items->end(); it++){
		rand_range.push_back(*it);
	}
	MyRandRange(&rand_range, k);
	for (int i = 0; i < k; i++){
		int c = rand_range[i];
		rand_obj.InsertRep(c);
	}
	/*fill the nonrep according to the rep*/
	rand_obj.InsertNonrep(); 
	
	/*repeat max iterations*/
	map<int, int> assignment_cur, assignment_new; // key:vertex,  value:rep
	/*assign each obj to the cluster with the nearest reprentitive obj*/
	int E_cur = AssignObj(group_items, rand_obj.objs_rep(), &assignment_cur);
	for (int i = 1; i < max_iterations; i++){
		/*randomly, select a nonrepresentitive obj_rand*/
		Objs *objs = rand_obj.GetSwapedObjs();
		if (NULL == objs)
			break;
		int E_new = AssignObj(group_items, objs, &assignment_new);
		/*swap*/
		//COUT(<< "label: " << label << " swap in iteration: " << i << " with E_new " << E_new << " E_cur " << E_cur << endl;)
		if (E_new < E_cur){
			COUT(<< "label: " << label << " swap in iteration: " << i << " with E_new " << E_new << " E_cur " << E_cur << endl;)
			COUT(<<"	so swap"<< endl;)
			assignment_cur = assignment_new;
			E_cur = E_new;
		}
		if (0 == E_new)
			break;
	}

	/*insert to the cluster*/
	for (auto it = assignment_cur.begin(); it != assignment_cur.end(); it++){
		int vertex = it->first;
		int rep = it->second;
		if (cluster_units->find(rep) == cluster_units->end()){
			ClusterUnit cluster_unit;
			cluster_unit.c = rep;
			cluster_unit.r = 0;
			cluster_units->insert({ rep, cluster_unit });
		}
		cluster_units->at(rep).objs.insert(vertex);
	}

	/*calculate radius*/
	for (auto it = cluster_units->begin(); it != cluster_units->end(); it++){
		ClusterUnit *cluster_unit = &it->second;
		it->second.r = CalculateRadius(cluster_unit);
	}

	return 0;
}
/*return E: absolute-error criterion*/
int Cluster::AssignObj(GroupItems *group_items, Objs *objs, map<int, int> *assignment){
	int choose_rep;
	int L;
	int E = 0;

	assignment->clear();
	for (auto it = group_items->begin(); it != group_items->end(); it++){
		int vertex = *it;
		L = kInfinity;
		choose_rep = *objs->begin();
		for (auto it2 = objs->begin(); it2 != objs->end(); it2++){
			int obj_rep = *it2;
			
			/*assign the vertex to itself is not good because it loos the meaning of clustering.*/
			//if (vertex == obj_rep){ //assign the vertex to itself
			//	choose_rep = obj_rep;
			//	L = 0;
			//	break;
			//}
			/*calculate the dist*/
			int L_new = CalculateObjDist(vertex, obj_rep);
			if (L_new < L){
				L = L_new;
				choose_rep = obj_rep;
			}
		}
		
		if (L < kInfinity/2) //not include the sigle vertex **
		{
			E += L;
			assignment->insert({ vertex, choose_rep });
		}
		
	}
	return E;
}

int Cluster::CalculateObjDist(int obj_0, int obj_1){
	return embedding_->CalculateL(obj_0, obj_1);
}

int Cluster::CalculateRadius(ClusterUnit *cluster_unit){
	int L = 0;
	int L_new;
	for (auto it = cluster_unit->objs.begin(); it != cluster_unit->objs.end();
		it++){
		if (cluster_unit->c == *it){
			continue;
		}
		L_new = CalculateObjDist(cluster_unit->c, *it);
		if (L_new > L){
			L = L_new;
		}
	}
	return L;
}

ERR Cluster::Write(const char *filename, const char *delim){

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

	for (int label = 0; label < (int)group_clusters_.size(); label++)
	{	
		ClusterUnits *cluster_units = &group_clusters_[label];
		for (auto it2 = cluster_units->begin(); it2 != cluster_units->end(); it2++)
		{
			// label, c, r, objs
			int c = it2->second.c;
			int r = it2->second.r;
			strbuf = _itoa(label, buf, 10);
			strbuf += delim;
			strbuf += _itoa(c, buf, 10);
			strbuf += delim;
			strbuf += _itoa(r, buf, 10);
			for (auto it3 = it2->second.objs.begin(); it3 !=
				it2->second.objs.end(); it3++){
				int obj = *it3;
				strbuf += delim;
				strbuf += _itoa(obj, buf, 10);
			}
			strbuf += "\n";
			fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
		}
	}

	fclose(fp);
	return 0;

}

ERR Cluster::Read(const char *filename, const char* delim){
	int error;
	FILE *fp;
	int kBufSize = 1024*1024;
	char *strbuf = new char[kBufSize];
	char *p;
	vector<int> data;

	char *ptr = strbuf;

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

		data.clear();

		p = strtok(strbuf, delim); /*change the strbuf pointer.*/
		if (NULL == p)
			continue;

		data.push_back(atoi(p));
		for (int i = 1; (p = strtok(NULL, delim)) != NULL; i++){
			data.push_back(atoi(p));
		}
		/*get data*/
		int label = data[0];
		int c = data[1];
		int r = data[2];

		ClusterUnits *cluster_units = &group_clusters_[label];
		ClusterUnit cluster_unit;
		cluster_unit.c = c;
		cluster_unit.r = r;
		
		for (int i = 3; i < (int)data.size(); i++){
			int obj = data[i];
			cluster_unit.objs.insert(obj);
		}
		cluster_units->insert({ c, cluster_unit });
	}

	fclose(fp);
	delete[](ptr);
	return 0;
}

RandObj::RandObj(GroupItems *group_items){
	size_ = group_items->size();
	Ri_.reserve(size_);
	for (auto it = group_items->begin(); it != group_items->end(); it++){
		Ri_.push_back(*it);
	}
	
}

/*if rep obj is NULL, return -1*/
int RandObj::SelectRepObj(){
	if (0 == objs_rep_.size())
		return -2;

	int index = MyRand() % objs_rep_.size();
	int i = 0;
	for (auto it = objs_rep_.begin(); it != objs_rep_.end(); it++){
		if (i == index)
			return *it;
		else
			i++;
	}
	return -1;
}


int RandObj::SelectNonrepObj(){
	if (0 == objs_nonrep_.size())
		return -2;

	int index = MyRand() % objs_nonrep_.size();
	int i = 0;
	for (auto it = objs_nonrep_.begin(); it != objs_nonrep_.end(); it++){
		if (i == index)
			return *it;
		else
			i++;
	}
	return -1;
}

void RandObj::InsertRep(int o){
	objs_rep_.insert(o);
}

void RandObj::InsertNonrep(){
	for (auto it = Ri_.begin(); it != Ri_.end(); it++){
		int vertex = *it;
		if (objs_rep_.find(vertex) == objs_rep_.end()){
			objs_nonrep_.insert(vertex);
		}
	}
}

void RandObj::EraseRep(int o){
	objs_rep_.erase(o);
}

Objs *RandObj::GetSwapedObjs(){
	int rand_nonrep = SelectNonrepObj();
	int rand_rap = SelectRepObj();
	if (rand_nonrep < 0 || rand_rap < 0)
		return NULL;

	objs_rep_swap_ = objs_rep_;
	objs_rep_swap_.erase(rand_rap);
	objs_rep_swap_.insert(rand_nonrep);
	return &objs_rep_swap_;
}

void RandObj::SetSwapedObjs(){
	objs_rep_ = objs_rep_swap_;
}