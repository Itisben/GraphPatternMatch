#ifndef ASTAR_AC_H
#define ASTAR_AC_H

#include "stdafx.h"
#include "graph.h"
#include "group.h"
#include "query.h"
#include "embedding.h"

typedef vector<int> Pivots;
typedef vector<vector<int>> PivotDistances;
typedef vector<list<int>> ACCSupports;
typedef vector<int> ACCCounts;
enum PivotSelectStrategy{
	AAC_RANDOM_SELECT = 1,
	AAC_FASTEST_SELECT,
};

typedef vector<pair<int, int>> PivotDemension; //(vertex, dist)
typedef vector<PivotDemension> PivotDemensions;
typedef vector<byte> ACCBitmap;
typedef queue<int> Queue;
class AACGroup{
private:
	int label_num_;
	Graph *g_;
	vector<list<int>> items_;
public:
	AACGroup(Graph *g);
	vector<list<int>> *items(){ return &items_; };
	list<int> *items(int label){ return &items_[label]; }
	int label_num(){ return label_num_; }
};

class AstarAC{
private:
	//outer
	Graph *g_;
	AACGroup *group_;
	Query *query_;
	int delta_;
	
	//inner
	int pivot_num_;
	Pivots pivots_;
	PivotDistances pivot_distances_; //vertex, pivot
	
	//support and count for undirected graph
	ACCSupports acc_supports_;
	ACCCounts acc_counts_;
	Queue queue_;

	//temp use
	PivotDemensions pivot_dimensions_;
	ACCBitmap acc_bitmap_;
public:
	AstarAC(Graph *g, AACGroup *group, Query *query, int pivot_num);
	ERR ChoosePivots(int strategy);
	ERR ConstructPivotsDist();
	ERR WritePivotsDists(const char *filename, const char *delim);
	ERR ReadPivotsDists(const char *filename, const char* delim);

	/*future modify*/
	pair<int, int> FirstPrune(); //return pruned vertex
	int BinarySearch(int d, vector<int> *area); //return the nearest value

	ERR AStar(int s, int label);
	int HeuristicFuntion();
	ERR ArcConsistencyVerify();
	int inline EstimatDist(int u1, int u2);

	pair<int, int> EmbeddingPrune(Embedding *embedding); //for comparation
	int pivot_num(){ return pivot_num_; }
	Pivots* pivots(){ return &pivots_; }

};

#endif