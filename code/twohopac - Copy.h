#ifndef TWOHOPAC_H_
#define TWOHOPAC_H_

#include "stdafx.h"
#include "Graph.h"
#include "group.h"
#include "query.h"

#include <bitset>
typedef list<pair<int, int>> Labeling; //first, vertex, second, weight.
typedef list<int> NewSupport;
typedef int Count;
//the old count has problems, 
// the counter should be distinguised by list R.
typedef map<int, int> CountNew; 

typedef list<pair<int, int>> NewBucket;
typedef vector<NewBucket> NewBuckets;
typedef list<list<int>> MR;
enum{
	HASH_JOIN = 1,
	BUCKET_JOIN,
};
class TwohopAc{
	Graph *g_;
	Query *q_;
	Group *group_;
	int delta_;
	queue<int> deleted_queue_;

	vector<Labeling> labelings_[2];
	vector<NewSupport> supports_[2]; //0 out-support, 1 in-support
	vector<Count> counts_[2]; //0 out-count, 1 in-count
	vector<CountNew> counts_new_[2];

	NewBuckets buckets_;

	MR mr_;

public:
	TwohopAc(Graph *g, Query *q, Group *gorup);
	~TwohopAc();

	int ReadLabeling(const char *filename, const char *delim);
	int SortLabeling();

	int HashJoin(int label1, int label2, int dir = 0);

	//hash join with new counter.
	int HashJoinNew(int label1, int label2);
	//without hash table, using the simple 2-hop to do this for comparation.
	int HashJoinNew2(int label1, int label2, int dir = 0);
	//without pruning
	int HashJoinNew3(int label1, int label2, int dir = 0);

	int Sjoin(int method); 
	//sjoin with new counter
	int SjoinNew(int method, int stage);
	//without back proporgation
	int SjoinNew2(int method, int stage);
	

	int WriteMatches(const char *filename, char *delim);
	
	//for construct the result
	int ResultConstruct(int start_u, int end_u);
	int ResultStart();
	int ResultInfo();


	/******PC*****/
	/*1.generate the complete graph.
	**2.phase1
	**3.phase2
	*/
	typedef set<pair<int, int>> PCEdgeValue;
	typedef map<pair<int, int>, PCEdgeValue> PCEdges;

	typedef list<array<int, 4>> PCSupportValue;
	typedef map<int, PCSupportValue>  PCSupports; //for z.S.append(<i, j><x, y>)
	
	typedef map<int, int> PCCountValue;
	typedef map<pair<int, int>, PCCountValue> PCCount; // <x,y>.C[k];

	int PC(); //path consistency
	int PCGenerateCompleteQ(PCEdges *pc_edg); //generate the complete graph
	int PCJoin(PCEdgeValue *pcv0, PCEdgeValue *pcv1, PCEdgeValue *pcvnew);
	/**/
	int PCTri(PCEdges *pc_edges, int i, int j, int k, int x, int y, int z);
};


#endif