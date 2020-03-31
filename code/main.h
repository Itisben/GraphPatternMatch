#ifndef MAIN_H_
#define MAIN_H_

//#define EDGE_JOIN_TWOHOP_DEBUG
#define DEBUG_AC_EMBEDDING_NEW

//#define DEBUG_EMBEDDING_AC_HAS_NEIGHBOR_AREA_PRUNING

//#define DEBUG_NEIGHBOR_AREA_PRUNING

#define DEBUG_LARGE_GRAPH

#define COUT(str) cout str

#define ROOT_PATH "G:\\MyExpeirment\\GraphPatternMaching-4\\dataset\\"
#define TESTDATA 1011
const int kPivotNum = 20;
#if TESTDATA == 0
#define PATH  "..\\dataset\\data-test\\"
const int kGraphVertexNum = 16;
const int kQueryVertexNum = 3;
const int kDelta = 1;
const int kClusterK = 10;
const int KClusterMaxIteration = 1000;

const int kStartLable = 0;
const int kEndLabel = 2;
const int kMaxDelta = 60;
const int kRandWeight = -1; //1

#endif
#if TESTDATA == 11
#define PATH "..\\dataset\\data-ER\\"
const int kGraphVertexNum = 100000;
const int kQueryVertexNum = 7;
const int kDelta = 200;
const int kClusterK = 40;
const int KClusterMaxIteration = 1000;
//for saving the memory
//label 0 - 12
const int kStartLable = 0;
const int kEndLabel = 9;
const int kMaxDelta = 400;
const int kRandWeight = -1; //1
#endif
#if TESTDATA == 12
#define PATH "..\\dataset\\data-yeast-undirected\\"
const int kGraphVertexNum = 2362;
const int kQueryVertexNum = 7;
const int kDelta = 3;
const int kClusterK = 40;
const int KClusterMaxIteration = 1000;
//for saving the memory
//label 0 - 12
const int kStartLable = 0;
const int kEndLabel = 12;
const int kMaxDelta = 60;
const int kRandWeight = -1; //1
#endif
#if TESTDATA == 1
#define PATH "..\\dataset\\data-yeast-undirected\\"
const int kGraphVertexNum = 2362;
const int kQueryVertexNum = 7;
const int kDelta = 2;
const int kClusterK = 40;
const int KClusterMaxIteration = 1000;
//for saving the memory
//label 0 - 12
const int kStartLable = 0;
const int kEndLabel = 12;
const int kMaxDelta = 60;
const int kRandWeight = -1; //1
#endif
#if TESTDATA == 2
#define PATH "..\\dataset\\data-wiki-vote-undirected\\"
const int kGraphVertexNum = 7116;
const int kQueryVertexNum = 30;
const int kDelta = 200;
const int kClusterK = 100; //10;
const int KClusterMaxIteration = 1000;
//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 29;
const int kMaxDelta = 100;
const int kRandWeight = 30; //0-100
#endif
#if TESTDATA == 21
#define PATH "..\\dataset\\data-hepph\\"
const int kGraphVertexNum = 12008;
const int kQueryVertexNum = 4;
const int kDelta = 5;
const int kClusterK = 100; //10;
const int KClusterMaxIteration = 1000;
//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 10;
const int kMaxDelta = 0x0fffffff;
const int kRandWeight = -1; //no weight for random weight
#endif
#if TESTDATA == 3
#define PATH "..\\dataset\\data-citeseer-undirected\\";
const int kGraphVertexNum = 384413;
const int kQueryVertexNum = 6;
const int kDelta = 300;
const int kClusterK = 10;
const int KClusterMaxIteration = 1000;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 5;
const int kMaxDelta = 300;
const int kRandWeight = -1; //0-100 //when no weight, this is useful.
#endif
#if TESTDATA == 4
#define PATH "..\\dataset\\data-patent-citation-undirected\\";
const int kGraphVertexNum = 3774770;
const int kQueryVertexNum = 10;
const int kDelta = 1000;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 3;
const int kMaxDelta = 600;
const int kRandWeight = -1; //1

#define DEFAULT_EDGE_WEIGHT 
#endif
#if TESTDATA == 5
#define PATH "..\\dataset\\data-test\\";
const int kGraphVertexNum = 11;
const int kQueryVertexNum = 3;
const int kDelta = 1;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 2;
const int kMaxDelta = 6;
const int kRandWeight = -1; //1

#define DEFAULT_EDGE_WEIGHT 
#endif
#if TESTDATA == 101
#define PATH "G:\\MyExpeirment\\synthitic-graph\\ER\\";
const int kGraphVertexNum = 100000;
const int kQueryVertexNum = 4;
const int kDelta = 1;
const int kClusterK = 4;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 4;
const int kMaxDelta = 4;
const int kRandWeight = -1; //1

#define DEFAULT_EDGE_WEIGHT 
#endif
#if TESTDATA == 1011
#define PATH "G:\\MyExpeirment\\synthitic-graph\\SF\\";
const int kGraphVertexNum = 100000;
const int kQueryVertexNum = 4;
const int kDelta = 1;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 4;
const int kMaxDelta = 4;
const int kRandWeight = -1; //1

#define DEFAULT_EDGE_WEIGHT 
#endif

#if TESTDATA == 102
#define PATH ROOT_PATH"data-wiki-vote\\";
const int kGraphVertexNum = 7115;
const int kQueryVertexNum = 5;
const int kDelta = 1;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 99;
const int kMaxDelta = 0x0fffffff;
const int kRandWeight = -1; //1

#define DEFAULT_EDGE_WEIGHT 
#endif

#if TESTDATA == 103
#define PATH ROOT_PATH"real-graph\\cit-hepph\\";
const int kGraphVertexNum = 34546;
const int kQueryVertexNum = 4;
const int kDelta = 4;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 11;
const int kMaxDelta = 0x0fffffff;
const int kRandWeight = 1; //1
#define DEFAULT_EDGE_WEIGHT 
#endif
#if TESTDATA == 104
#define PATH ROOT_PATH"real-graph\\webStanford\\";
const int kGraphVertexNum = 281903;
const int kQueryVertexNum = 4;
const int kDelta = 4;
const int kClusterK = 100;
const int KClusterMaxIteration = 10000;
const int kDirected = true;

//for saving the memory
//label 0 - 29
const int kStartLable = 0;
const int kEndLabel = 9;
const int kMaxDelta = 0x0fffffff;
const int kRandWeight = -1; //1
#define DEFAULT_EDGE_WEIGHT 
#endif
enum{
	NAIVE_AC_JOIN = 1,
	HASH_AC_JOIN,
};

#endif