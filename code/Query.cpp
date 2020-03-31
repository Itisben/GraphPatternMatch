#include "query.h"

Query::Query(int max_vertex_num, int delta) :Graph(max_vertex_num, true, true){
	max_vertex_num_ = max_vertex_num;
	delta_ = delta;
}

ERR Query::ConstructLabelPairs(){
	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		if (false == it->exist) continue;
		for (auto it_e = it->edges.begin(); it_e != it->edges.end(); it_e++){
			label_pairs_.insert({ it->label, vertices_[it_e->vertex].label });
		}
	}
	return 0;
}

ERR Query::ConstructLabelSet(){
	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		if (false == it->exist) continue;
		label_set_.insert(it->label);
	}
	return 0;
}

ERR Query::ConstructJoinOrder(){

	/*fill forward edge*/
	LabelPairs label_pairs;
	auto fun = [](Vertex *u, int argc, void **argv){
		LabelJoinOrders *join_orders = (LabelJoinOrders*)argv[0];
		LabelPairs *label_pairs = (LabelPairs*)argv[1];
		Vertices *vertices = (Vertices *)argv[2];

		if (-1 != u->pai){
			int label_0 = vertices->at(u->pai).label;
			int label_1 = u->label;
			join_orders->push_back({ { label_0, label_1 }, FORWARD_EDGE });
			label_pairs->insert({ label_0, label_1});
		}
		return 0;
	};

	int argc = 3;
	void *argv[3] = { &label_join_orders_, &label_pairs, vertices()};
	DfsInTopoOrder(fun, argc, argv);

	/*fill backward edge*/
	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		Vertex *u = &*it;
		for (auto it2 = u->edges.begin(); it2 != u->edges.end(); it2++){
			LabelPair pair = { u->label, vertices_[it2->vertex].label };
			if (label_pairs.end() == label_pairs.find(pair)){
				label_join_orders_.push_back({ pair, BACKWARD_EDGE });
			}
		}
	}

	return 0;
}

ERR Query::ReadEdges(const char* filename, char* delim){
	int err = 0;
	Datas *datas = new Datas;
	ReadFile(datas, filename, delim);

	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		it->exist = false; //initial
	}

	for (auto it = datas->begin(); it != datas->end(); it++)
	{
		if (it->size() < 4)
			continue;
		int index_left = (*it)[0];
		int label_left = (*it)[1];
		int index_right = (*it)[2];
		int label_right = (*it)[3];

		vertices_[index_left].exist = true;
		vertices_[index_left].label = label_left;
		vertices_[index_right].exist = true;
		vertices_[index_right].label = label_right;

		Edge e = { index_right, -1 }; //weight useless here.
		vertices_[index_left].edges.push_back(e);
	}

	delete(datas);
	return err;
}

ERR Query::Start(const char *filename, char*delim){
	ReadEdges(filename, delim);
	ConstructLabelPairs();
	ConstructLabelSet();
	ConstructJoinOrder();
	return 0;
}


int Query::ReadLablePairs(const char* filename, char* delim){
	int err = 0;
	Datas *datas = new Datas;
	ReadFile(datas, filename, delim);

	for (auto it = vertices_.begin(); it != vertices_.end(); it++){
		it->exist = false; //initial
	}

	for (auto it = datas->begin(); it != datas->end(); it++)
	{
		if ('*' == (*it)[0])
			break;
		if ('#' == (*it)[0])
			continue;
		
		if (it->size() < 2)
			continue;

		int label_left = (*it)[0];
		int label_right = (*it)[1];

		label_pairs_.insert({ label_left, label_right });
		label_set_.insert(label_left);
		label_set_.insert(label_right);

		if (datas->size() >= 3){
			int type = (*it)[2];
			label_join_orders_.push_back({ { label_left, label_right }, type });
		}
	}

	delete(datas);
	return err;
}

int Query::ReadLables(){
	for (auto it = label_pairs_.begin(); it != label_pairs_.end(); it++){
		label_set_.insert(it->first);
		label_set_.insert(it->second);
	}
	return 0;
}