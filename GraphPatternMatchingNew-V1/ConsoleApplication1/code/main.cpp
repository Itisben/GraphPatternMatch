// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "graph2.h"
using namespace std;
string g_path = "E:\\MyExpeirment\\synthitic-graph\\ER\\";
string g_graph_path = g_path + "graph.txt";
string g_label_path = g_path + "label.txt";

const int kMaxVNum = 100000;
void CreateTransitiveClosure();

int _tmain(int argc, _TCHAR* argv[])
{
	//G transitive closure

	return 0;
}

void CreateTransitiveClosure()
{
	using namespace TC;
	Graph *g = new Graph(kMaxVNum, false);
	g->ReadEdges(g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	
	delete(g);
}