#include "embedding.h"
#include "main.h"
/*
*k is the dimention value. if k <= 0, set k to default value.
*/
Embedding::Embedding(Graph *g, Group *group, int k){
	g_ = g;
	max_vertex_num_ = g->max_vertex_num();
	debug_ = 0;
	group_ = group;
	vector<int> rk_i;

	/*init the rk*/
	k_sub_ = (int)log2(max_vertex_num_)-1;
	if (k<=0)
		k_ = k_sub_ * k_sub_;
	else
		k_ = k; /*the k set by myseft may be < k_sub^2*/

	rk_i.reserve(k_);
	/*default dist is infinity*/
	rk_i.insert(rk_i.begin(), k_, kInfinity);
	rks_.reserve(max_vertex_num_);
	rks_.insert(rks_.begin(), max_vertex_num_, rk_i);
}

int Embedding::ConstrctRks(){

	/*call back function to fill the vector space rk*/
	auto dijkstraFun = [](int s, int dir, Vertex *u, int argc, void **paras){
		Rks *rks = (Rks*)paras[0];
		Subset *subset = &(*(Subsets*)paras[1])[u->index];
		int *count = (int *)paras[2];
		int k = *(int *)paras[3];

		for (auto it = subset->begin(); it != subset->end(); it++){
			int subset_index = *it;
			if (kInfinity == (*rks)[s][subset_index]){
				(*rks)[s][subset_index] = u->dist;
				(*count)++;
				if ((*count) == k)
					return -1; //return -1, the dijkstra return.
			}		
		}
		return 0;
	};

	RandomSubset();

	/*for not all label in query*/
	for (int label = kStartLable; label <= kEndLabel; label++){
		auto item = group_->items(label);
		COUT(<< "****** label " << label << " size " << item->size() << endl);
		int id = 1;
		for (auto it = item->begin(); it != item->end(); it++){
			int i = *it;
			
			int count = 0;
			int k_demention = k();
			void *paras[4] = { &rks_, &vertices_in_subsets_, &count, &k_demention };
			g_->Dijkstra(i, 0, dijkstraFun, 4, paras); /*ONLY OUT*/
			/*for debug*/
			static int embedding_count = 0;	
			if (0 == embedding_count % 100){
				COUT(<<"*****label:" << label << " size:"<< item->size() <<" id:"<< id<< "  Embedding:: Dijkstra " << embedding_count  << " "<< i << endl;)			
			}
			embedding_count++;
			id++;
			/*for debug end*/
		}
	}

	RandomSubsetClear();
	return 0;
}

int Embedding::RandomSubset(){
	/*init the subset*/
	set<int> s;
	vertices_in_subsets_.insert(vertices_in_subsets_.begin(), max_vertex_num_, s);
	vector<int> my_range;
	my_range.reserve(max_vertex_num_);
	
	for (int i = 0; i < max_vertex_num_; i++){
		my_range.push_back(i);
	}
	
	/*random select the subset*/
	for (int i = 0; i < k_sub_; i++){ // row
		for (int j = 0; j < k_sub_; j++){ //column
			auto subset_index = i*k_sub_ + j;
			if (subset_index >= k_) //k may be small than maximum value
				return -1;
			//according to the paper, this is right.
			int subset_size =(int)pow(2, i+1);
			
			/*get the rand range by swap*/
			MyRandRange(&my_range, subset_size);
			for (int i = 0; i < subset_size; i++){
				int index = my_range[i];
				vertices_in_subsets_[index].insert(subset_index);
			}

		}
	}
	
	return 0;
}

void Embedding::RandomSubsetClear(){
	vertices_in_subsets_.clear();
}

/*for direct graph and undirect graph are different*/
int Embedding::CalculateL(int v0, int v1){
	if (v0 == v1){ /*same vertex*/
		return 0;
	}

	int L = 0;
	for (int I = 0; I < k_; I++){
		/*
		* for undirected graph, it will lost most of information. so we can only use directed graph for create embedding.
		 */
		int L_new;

		L_new = abs(rks_[v0][I] - rks_[v1][I]);
		/*
		if (L_new > kDelta) // for debug. do not have this in paper, so we donot need this.
			return L_new;
		*/
		if (L_new > L)
			L = L_new;
	}
	return L;
}

/*we do not return infinity*/
int Embedding::GetMaxIValue(int v){
	int value = 0;
	auto rk = rks(v);
	for (int I = 0; I < k_; I++){
		int value_new = rk->at(I);
		if (value_new != kInfinity && value_new > value){
			value = value_new;
		}
		
	}
	return value;
}


int Embedding::Write(const char *filename, char* delim){
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

	for (int i = 0; i < max_vertex_num_; i++)
	{

		strbuf = _itoa(i, buf, 10); //vertex index
		for (auto it2 = rks_[i].begin(); it2 != rks_[i].end(); it2++)
		{
			strbuf += delim;
			int value = *it2;
			if (kInfinity == value){
				value = -1;
				goto NEXT;	/*when kInfinity is useless, so we donot put into file.*/
			}
			strbuf += _itoa(value, buf, 10); // I value in k demention.
		}
		strbuf += "\n";
		fwrite(strbuf.data(), sizeof(char), strbuf.length(), fp);
	NEXT:;
	}

	fclose(fp);
	return 0;
}

int Embedding::Read(const char *filename, char* delim){
	int error;
	FILE *fp;
	const int kBufSize = 1024 * 64;
	char strbuf[kBufSize] = { 0 };
	char *p;
	vector<int> data;

	data.reserve(k_+1);

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
		Rk* rk = &rks_[data[0]];
		
		for (int i = 1; i < k_+1; i++){
			if (i >= (int)data.size())
				break;
			int I = i - 1;
			int value = data[i];
			/*here is very import
			*-1 means infinity
			*/
			if (-1 == value) {
				value = kInfinity;
			}
			rk->at(I) = value;
		}
	}

	fclose(fp);
	return 0;
}