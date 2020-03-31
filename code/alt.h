#ifndef ALT_H_
#define ALT_H_
#include "stdafx.h"
#include "graph.h"
#include "group.h"
#include "query.h"
#include "embedding.h"

typedef vector<int> Landmarks;
typedef vector<vector<int>> LandmarkDists;
typedef vector<int> AltSupportCount;
typedef vector<list<int>> AltSupport;
enum LandmarkSelectStrategy{
	START_SELECT = 0,
	DEFINE_SELECT,
	RANDOM_SELECT,
	FASTEST_SELECT,
};
class AltGroup{
private:
	int label_num_;
	Graph *g_;
	vector<list<int>> items_;
public:
	AltGroup(Graph *g);
	vector<list<int>> *items(){ return &items_; };
	list<int> *items(int label){ return &items_[label]; }
	int label_num(){ return label_num_; }
};

class Alt{
private:
	Graph *g_;
	AltGroup *group_;
	Query *query_;
	int delta_;
	int landmark_num_;
public:
	Landmarks landmarks_;
	LandmarkDists l_vertex_dists_; //key:vertex, value landmarks value.
	//LandmarkDists l_group_min_dists_; //key:label, value landmarks.
	//LandmarkDists l_group_max_dists_;
	
	AltSupportCount support_count_;
	AltSupport support_;
public:
	Alt(Graph *g, AltGroup *group, Query *query, int landmark_num);
	
	ERR ChooseLandmark(int num, int strategy);
	ERR ConstructLandmarkDist();
	ERR WriteLandmark(const char *filename, const char *delim);
	ERR ReadLandmark(const char *filename, const char* delim);

	ERR UpdateLandmarkGroupDist(bool is_in_query = false);
	ERR AStar(int s, int label);
	int HeuristicFuntion();
	ERR ArcConsistency();
	int EstimatDist(int u1, int u2);
	int EstimatDistToGroup(int u, vector<vector<int>> *area_dists);
	int BinarySearch(int d, vector<int> *area);
	int FirstPrune();

	int EmbeddingPrune(Embedding *embedding);
	int landmark_num(){ return landmark_num_; }
	Landmarks* landmarks(){ return &landmarks_; }
};

#endif