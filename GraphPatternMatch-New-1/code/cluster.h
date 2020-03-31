#ifndef CLUSTER_H_
#define CLUSTER_H_
#include "stdafx.h"
#include "embedding.h"
#include "group.h"


typedef set<int> Objs;
//typedef vector<int> Objs;
class RandObj{
private:
	Objs objs_rep_;
	Objs objs_nonrep_;
	Objs objs_rep_swap_;
	int size_;

	vector<int> Ri_;
public:
	RandObj(GroupItems *group_items);
	/*return the obj vertex*/
	int SelectRepObj();
	int SelectNonrepObj();
	void InsertRep(int o);
	void InsertNonrep();
	void EraseRep(int o);

	/**/
	Objs *GetSwapedObjs();
	void SetSwapedObjs();

	/**/
	set<int> *objs_rep(){ return &objs_rep_; }
};

typedef struct {
	//int index;
	int c; //center, key
	int r; //radius, the fast node dist to center
	set<int> objs;
}ClusterUnit;

/*key: center*/
typedef map<int, ClusterUnit> ClusterUnits;
typedef  vector<ClusterUnits> GroupClusters;
class Cluster{
private:
	Embedding *embedding_;
	Group *group_;
	int k_;
	int max_iterations_;

	/*key:group label, every label has one clusterItems*/
	GroupClusters group_clusters_;
	int size_;
	
	int debug_;
public:
	Cluster(Embedding *embedding, Group *group);
	~Cluster();
	
	/*k-medoids algorithm, when k = -1, we calculate k by cluster size*/
	ERR KMedoids(int k, int max_iterationsm, int cluster_size = 1);

	ERR KMedoidsGroup(int label, int k, int max_iterations);
	/*return the E*/
	int AssignObj(GroupItems *group_items, Objs *objs, map<int, int> *assignment);
	int CalculateObjDist(int o1, int o2);
	int CalculateRadius(ClusterUnit *cluster_unit);

	/*label,center,r,obj in cluster*/
	ERR Read(const char *filename, const char* delim);
	ERR Write(const char *filename, const char* delim);

	/*k-means*/
	int KMeans();


	ClusterUnits* group_clusters(int label) { return &group_clusters_[label]; }
	vector<ClusterUnits> *group_clusters(){ return &group_clusters_; }
	void set_debug(int i){ debug_ = i; }
};



#endif