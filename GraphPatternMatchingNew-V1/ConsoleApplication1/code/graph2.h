#ifndef GRAPH_H_
#define GRAPH_H_
//#include "stdafx.h"


#include <map>
#include <stack>
#include <algorithm>
#include <iostream>

#include <list>
#include <set>
#include <vector>
using namespace std;
namespace TC{
	enum Color {
		kWhite,
		kGray,
		kBlack,
		kColorNotExist,
	};
	enum Direction {
		kOut = 0,
		kIn = 1,
	};
	typedef struct{
		int vertex; //next vertex index
		int weight;
	}Edge;
	typedef struct{
		int index; //key
		int label;
		list<Edge> edges;
		int pai;
		int d;
		int f;
		int dist;
		Color color;
		bool exist; //is this vertex exist or not
	}Vertex;

	class Graph{
	private:
		typedef set<int> Scc;
		typedef list<set<int>> Sccs;
		//common callback function for dijkstra. if it return -1, than then the dijkstra end;  
		//data is the return value.
		typedef int(*DijkstraFun)(int s, int dir, Vertex *u, int argc, void **argv);
		typedef int(*HeuristicFun)(int dir, Vertex *u, int argc, void **argv);
		typedef vector<Vertex> Vertices;
		//typedef list<pair<int, int>> EdgePairs;
		typedef int(*DfsFun)(Vertex *u, int argc, void **argv);
		const int kInfinity = 0x0fffffff;
		const char kIgnoreCurrent = '#';
		const char kIgnoreAfter = '*';
#define COUT(s) cout s;

	private:
		vector<Vertex> r_vertices_;
		int label_num_;
		Sccs sccs_;
		bool has_rgraph_;
		bool directed_;
	protected:
		vector<Vertex> vertices_;
		vector<Vertex*> topo_sorted_v_;
		int max_vertex_num_;
		int vertx_num_after_delete_;
		int debug_;

	public:
		/*if it is directed graph, we should decide whether we have rgraph */
		Graph(int max_vertex_num, bool directed = true, bool has_rgraph = false);
		~Graph();
		int ReadEdges(const char* filename, const char* delim);
		int ReadLabels(const char* filename, const char* delim);
		/*default direction is DOUT and the call back function is NULL */
		int AStar(int s, int direction = 0, DijkstraFun dikstraFun = NULL, int argc = 0, void **argv = NULL,
			HeuristicFun heuristicFun = NULL, int argc2 = 0, void **argv2 = NULL);
		int Dijkstra(int s, int direction = 0, DijkstraFun dikstraFun = NULL, int argc = 0, void **argv = NULL);
		int Dfs(DfsFun dfsFun = NULL, int argc = 0, void **argv = NULL);
		int Graph::DfsInTopoOrder(DfsFun dfsFun, int argc, void **argv);
		//vector<Vertex*>  *TopoSort();
		Sccs *GetSccs();

		/*when all vertices that should be deleted more then shreshold, the delete operation are fired. */
		int RemoveVertex(int i, int threshold);

		void TextDijkstra();

		void set_debug(int debug){ debug_ = debug; }
		int debug() const{ return debug_; }
		int max_vertex_num() const { return max_vertex_num_; }
		Vertex* vertex(int i) { return &vertices_[i]; }
		vector<Vertex>* const vertices(){ return &vertices_; }
		Vertex * vertices(int i){ return &vertices_[i]; }
		vector<Vertex*>* topo_sorted_v(){ return &topo_sorted_v_; }
		Vertex* topo_sorted_v(int i){ return topo_sorted_v_[i]; }
		int label_num(){ return label_num_; }
		int set_label_num(int label_num){ label_num_ = label_num; }
		int vertx_num_after_delete(){ return vertx_num_after_delete_; }
		int get_label(int v){ return vertices_[v].label; }
		bool directed(){ return directed_; }
		bool vexist(int u){ return vertex(u)->exist; }
	};
}
#endif
