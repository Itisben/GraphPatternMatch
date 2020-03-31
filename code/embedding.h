#ifndef EMBEDDING_H_
#define EMBEDDING_H_
#include "stdafx.h"
#include "graph.h"
#include "group.h"
#include <math.h>
typedef vector<int> Rk;  //k size
typedef vector<Rk> Rks;
typedef vector<set<int>> Subsets;
typedef set<int> Subset;
class Embedding{
private:
	Graph *g_;
	Rks rks_; //key1: vertex, key2: R vector
	int max_vertex_num_;
	Group *group_;

	/*demention, the default is log2(max_vertex_num_)^2 but we can use smaller value*/
	int k_; 
	int k_sub_; /*2 * [(0...k_sub_-1) +1] item in the subset*/
	Subsets vertices_in_subsets_; /*vertex belong to what subset*/
	int debug_;

	int RandomSubset();
	void RandomSubsetClear();
public:
	/*k is the dimention value. if k = -1, set k to default value.*/
	Embedding(Graph *g, Group *group, int k = -1);

	int ConstrctRks();
	int CalculateL(int v0, int v1);
	/*we do not return infinity*/
	int GetMaxIValue(int v);
public:
	int Write(const char *filename, char* delim);
	int Read(const char *filenam, char* delim);

	inline Rks *rks(){ return &rks_; }
	inline Rk *rks(int i){ return &rks_[i]; }
	int k_sub(){ return k_sub_; }
	int k(){ return k_; }
	void set_debug(int i){ debug_ = i; }
};

#endif