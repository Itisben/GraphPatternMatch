#ifndef GROUP_H_
#define GROUP_H_
#include "stdafx.h"
#include "graph.h"

typedef set<int> GroupItems;
class Group{
private:
	int label_num_;
	Graph *g_;
	vector<GroupItems> items_;
	
	int Init();
public:
	Group(Graph *g);
	vector<GroupItems> *items(){ return &items_; };
	GroupItems* items(int label){ return &items_[label]; }
	int label_num(){ return label_num_; }
	int Show();
};
#endif