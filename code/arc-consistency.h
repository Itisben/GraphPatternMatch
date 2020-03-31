#ifndef ARC_CONSISTENCY_H_
#define ARC_CONSISTENCY_H_
#include "stdafx.h"
#include "graph.h"
#include "query.h"
#include "twohop.h"
#include "group.h"
#include "embedding.h"
#include "edge-join.h"

typedef struct{
	int group_label;
	int vertex;
}Support;

typedef list<Support> SupportList;
// 0 for out , 1 for in
typedef vector<SupportList> SupportLists;
typedef vector<int>	SupportCounts;
typedef vector<int> MatchResult;
typedef list<vector<int>> MatchResults;
class ArcConsistency{
private:
	Graph *g_;
	Query *q_;
	Group *group_;
	Embedding *embedding_;

	int delta_;	// the distance of the shortest path.
	int label_num_;

	//key label, this is Ri that group include all the vertex with same label. 
	SupportLists support_lists_[2]; //in and out, out is for construct result.
	SupportCounts support_counts_[2];
	queue<int> deleted_queue_;

	MatchResults match_result_;

	/*iterater the deleted_queue and reduce the support count.*/
	int DeletedPruning();
	int SupportPruning();

	int debug_;
public:
	ArcConsistency(Graph *g, Query *q, Group *group);
	~ArcConsistency();
	
	/*the main method*/
	int TwohopArcPruning(Twohop *twohop);
	int TwohopArcPruning2(Twohop *twohop);

	/*use the dfs and stack for the final result.*/
	MatchResult *GetMatchResult();

	/*label, center, right support label, and vertex*/
	int write(const char *filename, char *delim);


	SupportLists *support_lists(){ return support_lists_; }
	/*only return the out one*/
	SupportList *support_lists(int i){ return &support_lists_[0][i]; }


	/***********for my created new method*************/
	void set_embedding(Embedding *embedding){ embedding_ = embedding; }
	/*return deleted number*/
	int NeighborAreaPruning();
	bool IsInNeighborArea(Area *area, Rk *rk, int k);
	/*build support using embedding. just like FindCan*/
	ERR EmbeddingArcPruning();
	ERR EmbeddingArcPruningByGroup(LabelPair label_pair);
	/*new for directed graph*/
	int EmbeddingArcPruningByGroup2(LabelPair label_pair);
	
	int NeighborAreaPruningByGroup(LabelPair label_pair);
	/*the L of embedding is < real d. so we need verify by dijkstra*/
	/*return the dikjstra number*/
	int EmbeddingArcPruningVerify();
	ERR EmbeddingArcPruningVerifyBySupport(int vertex, SupportList *support_list);
	ERR ClearSupports();
	//int EmbeddingACPruning(Embedding *embedding, bool with_support);

	void set_debug(int i){ debug_ = i; }
};

typedef unordered_set<int> SupportSet;
typedef vector<SupportSet> SupportSets;
class EmbeddingAC{
private:
	Graph *g_;
	Query *q_;
	Group *group_;
	Embedding *embedding_;
	int label_num_;
	int delta_;
	int debug_;

	SupportSets support_sets_[2]; /*0 out , 1 in*/
	queue<int> deleted_queue_;

public:
	EmbeddingAC(Graph *g, Query *q, Group *group);
	ERR EmbeddingArcPruning(int method, bool neighbor_area_pruning, DWORD start_time);
	ERR EmbeddingArcPruningByGroup(LabelPair label_pair);
	/*naive method without hash join for comparing*/
	ERR EmbeddingArcPruningByGroup2(LabelPair label_pair);
	/*hash join without bucket, just new design*/
	ERR EmbeddingArcPruningByGroup3(LabelPair label_pair);
	ERR FindIntersection(Cans *cans, vector<int> *result);
	/*for undireted graph*/
	ERR EmbeddingArcPruningByGroupUndirect(LabelPair label_pair);
	ERR EmbeddingArcPruningByGroupUndirect2(LabelPair label_pair);

	int EmbeddingArcPruningVerify();
	ERR EmbeddingArcPruningVerifyBySupport(int s, SupportSet *support_set);
	int DeletedPruning();
	int NeighborAreaPruning();
	int NeighborAreaPruningByGroup(LabelPair label_pair);
	bool IsInNeighborArea(Area *area, Rk *rk, int k);
	int write(const char *filename, char* delim);
	void set_debug(int i){ debug_ = i; }
	void set_embedding(Embedding *emb){ embedding_ = emb; }
	int deleted_queue_size(){ return deleted_queue_.size(); };

};

#endif
