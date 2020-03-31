#ifndef EDGE_JOIN_H_
#define EDGE_JOIN_H_
#include "embedding.h"
#include "twohop.h"
#include "group.h"
#include "query.h"
#include "cluster.h"
/*every group has a area, for k dimention*/
typedef set<pair<int, int>> AreaI;
typedef vector<set<pair<int, int>>> Area; //for every k dimention
typedef vector<Area> Areas; //for every group
typedef set<pair<int, int>> PairSet;
typedef vector<PairSet> Mrs;

typedef unordered_set<int> Bucket; //bucket item
typedef vector<Bucket> Buckets; // for one demention, bucket size
typedef vector<Buckets> BucketsK; //k-demention
typedef vector<Bucket*> Cans;
typedef array<Bucket*, 2> BucketArray;
typedef vector<array<Bucket*,2>> NewCans; //put all qulified bucket in cans.

typedef vector<int> NBucket;
typedef vector<NBucket> NBuckets;
typedef vector<NBuckets> NBucketsK;
typedef vector<NBucket*> NCans;

class EdgeJoin{
private:
	Graph *g_;
	Query *q_;
	Group *group_;
	Embedding *embedding_;
	Twohop *twohop_;
	Cluster *cluster_;
	int delta_;

	//set of result, corresponding to q_.label_join_orders
	Mrs mrs_;
	int debug_;
public:
	EdgeJoin(Graph *g, Query *q, Group *group, Embedding* embedding, 
		Twohop *twohop, Cluster *cluster);
public:
	/*neighbor area pruning*/
	ERR NeighborAreaPruning(int max_loop_num, int *loop_num, int *delete_num);
	bool IsInNeighborArea(Area *area, Rk *rk, int k);
	AreaI *BuildArea(AreaI *areai, GroupItems *group_items, Rk *rk, int I);
	
	/*distance based edge join*/
	ERR DistanceBasedEdgeJoin(LabelPair *label_pair, PairSet *rs);

	/*memory edge join*/
	ERR MemoryEdgeJoin(ClusterUnit *cluster_unit_1, ClusterUnit * cluster_unit_2, int *label, PairSet *cl, int *count);
	ERR FindSP(ClusterUnit *cluster_unit_1, int p, set<int> *sp_p);		//return value sp_p, can_p
	ERR FindCan(ClusterUnit *cluster_unit_1, int p, set<int> *can_p);
	ERR FindIntersection(vector<set<int>*> datas);
	
	/*multiway distance join*/
	ERR MultiwayDistanceJoin(int max_loop_num);
	bool MrJoin(PairSet *mr[2]);
	/*label_pair according to the mr*/
	bool GroupPruningByMrInPair(LabelPair *label_pair, PairSet *mr);
	//bool GroupPruningByMrNotInPair(LabelPair *label_pair[2], PairSet *mr[2]);

	/*label pair [L1-L2], mrs [v1-v2]...*/
	int Write(const char *filename, char *delim);

	void set_debug(int i){ debug_ = i; }
};

#endif