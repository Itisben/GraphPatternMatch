#ifndef QUERY_H_
#define QUERY_H_
#include "graph.h"
typedef  pair<int, int> LabelPair;
typedef set<LabelPair> LabelPairs;
enum{
	FORWARD_EDGE = 1,
	BACKWARD_EDGE = 2,
};
/*int : type */
typedef pair<LabelPair, int> LabelJoinOrder;
typedef vector<pair<LabelPair, int>> LabelJoinOrders;

class Query:public Graph{
private:
	int delta_;
	/*label pair store in set, for one.*/
	LabelPairs label_pairs_;
	/*what labels include in the query*/
	set<int> label_set_;
	/*for edge join: v1, v2, forward edge or backward edge */
	LabelJoinOrders label_join_orders_;

public:
	Query(int max_vertex_num, int delta);
	ERR ConstructLabelPairs();
	ERR ConstructLabelSet();
	ERR ConstructJoinOrder();
	/*index left, label left, index right, label right */
	ERR ReadEdges(const char *filename, char*delim);
	ERR Start(const char *filename, char*delim);

	LabelPairs *label_pairs(){ return &label_pairs_; }
	int delta(){ return delta_; }
	set<int> *label_set(){ return &label_set_; }
	LabelJoinOrders *label_join_orders(){ return &label_join_orders_; }
	LabelJoinOrder *label_join_orders(int i){ return &label_join_orders_[i]; }
	
	//for test the big data
	int ReadLablePairs(const char* filename, char* delim);
	int ReadLables();
};
#endif