#ifndef RESULT_H_
#define RESULT_H_
#include "stdafx.h"
#include "group.h"
#include "query.h"
typedef list<list<int>> MR;
class Result
{
private:
	Group *group_;
	Query *q_;
	MR mr_; //return all match result MR

public:
	Result(Group *group, Query *q);
	int Construct(int u, LabelPair edge);
	int Start();
	MR *mr(){ return &mr_; }
};

#endif