#include "result.h"
#if 0
Result::Result(Group *group, Query *q){
	group_ = group;
	q_ = q;
}
int Result::Construct(int u, LabelPair edge){
	

}
int Result::Start(){
	/*set visited to false*/
	for (int i = 0; i < q_->max_vertex_num(); i++){
		q_->vertex(i)->exist = 0;
	}

	//dfs

}
#endif