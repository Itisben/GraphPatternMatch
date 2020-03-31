#ifndef TWOHOPAC_H_
#define TWOHOPAC_H_

#include "stdafx.h"
#include "Graph.h"
#include "group.h"
#include "query.h"

#include <bitset>
class TwohopAc{
	enum{
		HASH_JOIN = 1,
		BUCKET_JOIN,
	};
	typedef list<pair<int, int>> Labeling; //first, vertex, second, weight.
	typedef list<int> NewSupport;
	typedef int Count;
	//the old count has problems, 
	// the counter should be distinguised by list R.
	typedef map<int, int> CountNew;

	typedef list<pair<int, int>> NewBucket;
	typedef vector<NewBucket> NewBuckets;
	typedef list<list<int>> MR;
	
	

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

	/*for testing the equal-join time*/
	typedef set<pair<int, int>> Relation;
	typedef map<pair<int, int>,Relation> Relations; //each edge hava relation
	Relations relations_;

	typedef array<int, 2> RelationGraphVertex;
	typedef set<array<int, 2>> RelationGraphVertices;
	typedef list<array<int, 4>> RelationGraphEdges;
	RelationGraphVertices r_vertices_;
	RelationGraphEdges r_edges_;

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
	int ResultInfo(const char *src_filename = NULL);


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

	//PCEdges pc_edges_;
	int PC(); //path consistency
	int PCGenerateQ(); //generate the complete graph
	/**/
	int PCTri(PCEdges *pc_edges, int i, int j, int k, int x, int y, int z);
	
	/*for testing the equal-join time*/
	//read the relation graph edges and vertices
	int ReadRelationGraph(const char *filename, const char *delim); 
	/*method 1 hash, method 2 naive*/
	int RelationConstruct(int method); 
	int RelationClear();
	int RelationConstructEachHash(int label0, int label1, double *time=NULL); //2-hop + hash
	int RelationConstructEachNaive(int label0, int label1, double *time=NULL); // naive join n1* n2 computations.

#if 0
	/*naive*/
	int EqualJoin();
	/*using in MD-join, MR(Q'i+1) = MR(Q'i) join MR(e)*/
	int EqualJoin2();
	/*new dedesign for new MDJOIN*/
#endif
	typedef vector<int> NJMR; //match result, key: label, value: 
	typedef list<NJMR> NJMRs;
	NJMRs mrs_;
	int EqualJoin3Before();
	int EqualJoin3(); // exectlly like paper description.
	/*this is not very important, I put it back
	no join order selection*/
	int EqualJoin4(); // new designed for sapce saving using DFS
	bool EqualJoin4CheckFun(NJMR *mr);
	/*like equaljoin4 but with join order selection.*/
	int EqualJoin5();
	bool EqualJoin5CheckFun(NJMR *mr, list<pair<int, int>> *backward_edges_temp);

	/*v0 v1 is query edge, v is the key in the mmap*/
	int NaturalJoin(NJMRs *mrs, Relation *r, int v1, int v2, int v);
	int WriteNaturalJoin(const char *filename);

	int EqualJoin2Check(RelationGraphVertices *vs, RelationGraphVertex *v);
	int GetJoinIndex(array<int, 4> a, int *left, int *right);
	int EqualJoinStart();
	/*1:naive one geneerate relation offline, 2:generate relation dynamically using hash*/
	/*relation construct time for method 2*/
	int AC(int method, double *relation_construct_time=NULL);
	int ACDeleted();
	int AfterACPCRationConstruct(); //join based on ac's support

	//debug
	int WriteRelation(const char *filename);
};


#endif