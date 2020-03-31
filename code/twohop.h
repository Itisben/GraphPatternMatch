/*for s-join algorithm in paper*/
#ifndef TWOHOP_H_
#define TWOHOP_H_

#include "stdafx.h"
#include "Graph.h"
#include "group.h"

/*
* highly cohesive and low coupling.
* so do not include anyother things in this twohop modual.
*/

typedef pair<int, int> ClusterItem;
typedef map<int, int> ClusterItems; //key = vertex, value = dist
typedef struct{
	int center; //key in vector
	ClusterItems items[2];
}TwohopCluster;

typedef vector<TwohopCluster> TwohopClusters;
/*vertex belong to which cycle*/
typedef set<int> Cycle; // value: cycle_index
typedef map<int, set<int>> Cycles;//key: vertex, value:cycleIndex list
typedef set<int> VertexSet;

class Twohop{
private:
	Graph *g_;
	TwohopClusters clusters_;
	int max_num_;
	int debug_;
	bool pruning_;

	int GetDagInScc(Scc *scc, VertexSet *vs);
	int RemoveVertex(int center);
	int AddClusterItem(int center, int dir, AuxGraph *sp_graph);
	int ReduceSPGraph(AuxGraph *spg, int pre_center, int dir);
public:
	Twohop(Graph *g);
	~Twohop();

	VertexSet* GetDag(VertexSet *v_set);
	int ConstructClusters(VertexSet *v_set);

	typedef struct { int start;  int end; }Range;
	int FixedStrategy(int min_vertex_num);
	int FixedStrategyExtractVertex(Range range, VertexSet *vw);

	void Start(bool pruning, int min_vertex_num);
	int WriteCluster(const char *filename, const char *delim);
	int ReadCluster(const char *filename, char *delim);
	int CalculateDist(int v1, int v2);

	/*this is naive method*/
	int NaiveConstructCluster(Group *group);

	/*get set*/
	TwohopClusters *const clusters() { return &clusters_; }
	TwohopCluster *const clusters(int i){ return &clusters_[i]; }
	int debug(){ return debug_; }
	void set_debug(int debug){debug_ = debug;}
	int pruning(){ return pruning_; }
	void set_pruning(bool pruning){ pruning_ = pruning; }
};
#endif