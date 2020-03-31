#include "group.h"
Group::Group(Graph *g){
	g_ = g;
	label_num_ = g->label_num()+1; //for 0
	Init();
}

int Group::Init(){
	items_.reserve(label_num_);
	
	GroupItems item;
	items_.insert(items_.begin(), label_num_, item);

	//init all the group
	for (auto it = g_->vertices()->begin(); it != g_->vertices()->end(); it++){
		if (!g_->vexist(it->index)){ //jump over the not exist vertices
			continue;
		}
		if (it->label < 0){
			cout<< "Group init error" << endl;
			continue;
		}
		if (it->label >= (int)items_.size()){
			cout << "label_num " << label_num_ << endl;
			cout << "insert vertex  " << it->index << " to label " << it->label << endl;
		}
		items_[it->label].insert(it->index);
	}
	return 0;

}

int Group::Show()
{
	cout << endl;
	cout << "group info: " << endl;
	for (int i = 0; i < items_.size(); i++){
		cout << " group " << i << " size: " << items_[i].size() <<endl;
	}
	cout << endl;
	return 0;
}