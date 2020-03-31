// GraphPatternMatch-New-1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <Windows.h>
#include "main.h"
#include "graph.h"
#include "twohop.h"
#include "io.h"
#include "query.h"
#include "embedding.h"
#include "cluster.h"
#include "edge-join.h"
#include "twohopac.h"
#include "tool.h"

char *delime = ",";
string g_path = PATH;
string g_graph_path = g_path + "graph-10.txt";
string g_org_graph_path = g_path + "graph.csv";
string g_label_path = g_path + "label-40.txt";
string g_org_label_path = g_path + "label-40.txt";
string g_twohop_path = g_path + "twohop.csv";
string g_twohop_path_2 = g_path + "twohop-no-pruning.csv";
string g_embedding_path = g_path + "embedding.csv";
string g_query_path = g_path + "query.csv";
string g_cluster_path = g_path + "cluster.csv";
string g_edge_join_path = g_path + "r-edge-join.csv";
string g_twohopac_path = g_path + "r-twohopac.csv";
string g_query_equal_join_path = g_path + "query-equal-join.csv";
string g_mr_ac_path = g_path + "mr-ac.csv";
string g_mr_nj_path = g_path + "mr-nj.csv";
string g_mr_acpc_nj_path = g_path + "mr-acpc-nj.csv";
string g_relation_path = g_path + "relaton.csv";
string g_relation_acpc_path = g_path + "relation-ac-pc.csv";

void OffLineTwohop(){
	cout << "OffLineTwohop" << endl;

	Graph *g = new Graph(kGraphVertexNum, /*diredted graph*/kDirected, /*rgraph*/true);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels((char*)g_label_path.data(), ",");
	Group *group = new Group(g);

	cout << "init graph complete..." << endl;
	DWORD start_time, end_time;
	cout << "twohop start..." << endl;
	start_time = GetTickCount();

	Twohop *twohop = new Twohop(g);
	twohop->set_debug(1);
#if 0 //paper's method 
	twohop->Start(false, 5);
#else //simpler method
#if 0
	VertexSet v_set;
	twohop->GetDag(&v_set);
	cout<< "twohop:: get dag complete!" << endl;
	twohop->ConstructClusters(&v_set);
#endif
	twohop->NaiveConstructCluster(group);
#endif
	end_time = GetTickCount();
	twohop->WriteCluster(g_twohop_path.data(), ",");
	cout << "twohop finish without pruning." << end_time - start_time << " ms" << endl;

	delete(g);
	delete(group);
	delete(twohop);
	cout << endl;
}

void OffLineEmbedding(){
	cout << "OffLineEmbedding" << endl;
	DWORD start_time, end_time;

	/*the embedding graph should be undirected graph*/
	Graph *g = new Graph(kGraphVertexNum, /*diredted graph*/kDirected);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	cout << "init graph complete..." << endl;

	Group *group = new Group(g);
	cout << "init group complete..." << endl;

	cout << "embedding start..." << endl;
	start_time = GetTickCount();

	Embedding *embedding = new Embedding(g, group);
	embedding->set_debug(1);
	embedding->ConstrctRks();

	end_time = GetTickCount();
	embedding->Write(g_embedding_path.data(), ",");
	cout << "embedding end." << end_time - start_time << " ms" << endl;

	delete(group);
	delete(embedding);
	delete(g);
	cout << endl;
}

void OffLineCluster(){
	cout << "OffLineCluster" << endl;
	DWORD start_time, end_time;

	Graph *g = new Graph(kGraphVertexNum, /*diredted graph*/true, /*rgraph*/true);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	Group *group = new Group(g);

	Embedding *embedding = new Embedding(g, group);
	embedding->Read(g_embedding_path.data(), delime);
	cout << "read embedding comlete..." << endl;

	
	cout << "construct cluster begin..." << endl;
	start_time = GetTickCount();
	/*start timer*/
	Cluster *cluster = new Cluster(embedding, group);
	cluster->set_debug(1);
	cluster->KMedoids(kClusterK, KClusterMaxIteration);
	/*timer end*/
	end_time = GetTickCount();
	cluster->Write(g_cluster_path.data(), delime);
	cout << "construct cluster end. " << end_time - start_time << " ms" << endl;
	
	delete(group);
	delete(embedding);
	delete(g);
	delete(cluster);
	cout << endl;
}

void OnLineEdgeJoin(){
	cout << "OnLineEdgeJoin" << endl;
	DWORD start_time, end_time;

	Graph *g = new Graph(kGraphVertexNum, false);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	cout << "read graph complete... with label " << g->label_num() << endl;

	Query *q = new Query(kQueryVertexNum, kDelta);
	q->Start(g_query_path.data(), ",");
	cout << "read q complete..." << endl;

	Twohop *twohop = new Twohop(g);
	twohop->ReadCluster(g_twohop_path.data(), delime);
	cout << "read twohopcluster complete..." << endl;


	Group *group = new Group(g);

	Embedding *embedding = new Embedding(g, group);
	embedding->Read(g_embedding_path.data(), delime);
	cout << "read embedding comlete..." << endl;
	
	
	Cluster *cluster = new Cluster(embedding, group);

	cluster->Read(g_cluster_path.data(), delime);

	//cluster->KMedoids(kClusterK, KClusterMaxIteration);
	cout << "read cluster complete..." << endl;
	cout << ">>>>>>>>>>start edge join" << endl;
	
	start_time = GetTickCount();
	/*timer start*/
	EdgeJoin *edge_join = new EdgeJoin(g, q, group, embedding, twohop, cluster);
	int max_loop_num = 50;
	int loop_num = 0;
	int delete_num = 0;
	edge_join->set_debug(1);
	edge_join->NeighborAreaPruning(max_loop_num, &loop_num, &delete_num);

	end_time = GetTickCount();
	cout << "NeighborAreaPruning complete... loop num " << loop_num << " delete num " \
		<< delete_num << " time "<<end_time - start_time << " ms"<<endl;

	loop_num = edge_join->MultiwayDistanceJoin(max_loop_num);
	cout << "MultiwayDistanceJoin complete... loop num " << loop_num << endl;

	/*timer end*/
	end_time = GetTickCount();
	edge_join->Write(g_edge_join_path.data(), delime);
	cout << ">>>>>>>>>>>Edge Join end." << end_time - start_time << " ms" << endl;
	
	cout << endl;

	delete(edge_join);
	delete(embedding);
	delete(group);
	delete(q);
	delete(g);
	delete(twohop);

}
void OnlineTwohopAcNew()
{
	LARGE_INTEGER  large_interger;
	double dff;
	__int64  c1, c2;
	QueryPerformanceFrequency(&large_interger);
	dff = large_interger.QuadPart;


	cout << "OnLine New TwoHopAC" << endl;
	DWORD start_time, end_time;

	Graph *g = new Graph(kGraphVertexNum, true/*is directed graph*/);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	cout << "read graph complete... with label " << g->label_num() << endl;

	Query *q = new Query(kQueryVertexNum, kDelta); //delta
	q->Start(g_query_path.data(), ",");
	cout << "read q complete..." << endl;


	cout << "New twohop AC start..." << endl;
	Group *group = new Group(g);
	int group_label = 0;
	for (auto it = group->items()->begin(); it != group->items()->end(); it++){
		//cout <<"*group " << group_label << ": " <<it->size() << endl;
		group_label++;
	}
	TwohopAc *tac = new TwohopAc(g, q, group);
	tac->ReadLabeling(g_twohop_path.data(), ",");
	
	/*info*/
	tac->ResultInfo();

	//tac->SortLabeling(); 

	start_time = GetTickCount();

	QueryPerformanceCounter(&large_interger);
	c1 = large_interger.QuadPart;

	/*start timer*/
	int method = 1;
#define METHOD 1
#if METHOD == 1
	tac->SjoinNew(method /*method*/, 1/*stage*/);
#endif
#if METHOD == 2
	tac->SjoinNew2(method /*method*/, 1);
	/*end timer*/
#endif

	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("**twohop AC stage 1 end %lf ms \n", (c2 - c1) * 1000 / dff);
	tac->ResultInfo();
	/*stage 2*/
#if METHOD == 1
	tac->SjoinNew(method /*method*/, 2);
#endif
#if METHOD == 2
	tac->SjoinNew2(method /*method*/, 2);
#endif	
	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("****twohop AC stage 2 end %lf ms \n", (c2 - c1) * 1000 / dff);

	/*info*/
	tac->ResultInfo();

	/*PC begin*/
	cout << endl <<"PC begin......" << endl;
	tac->PC();
	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("****PC end %lf ms \n\n", (c2 - c1) * 1000 / dff);
	tac->ResultStart();
	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("result construction end %lf ms \n", (c2 - c1) * 1000 / dff);

	/*info*/
	tac->ResultInfo();
	//tac->WriteMatches(g_twohopac_path.data(), ",");
	//printf("twohop AC write matces end %lf ms \n", (c2 - c1) * 1000 / dff);

	cout << endl;

	delete(group);
	delete(q);
	delete(g);
	delete(tac);
}

void OnlineFjoin(){
	cout << endl;
	cout << "online Filter join" << endl;
	__int64 t1, t2;
	double relation_construct_time = 0;

	Graph *g = new Graph(kGraphVertexNum, false);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	cout << "read graph complete... with label " << g->label_num() << endl;

	Query *q = new Query(kQueryVertexNum, kDelta);
	q->ReadLablePairs(g_query_path.data(), ",");
	q->ReadLables();
	cout << "read q complete... query edge: " << q->label_pairs()->size()<<endl;


	Group *group = new Group(g);
	//group->Show();
	TwohopAc *tc = new TwohopAc(g, q, group);
	tc->ReadLabeling(g_twohop_path.data(), ",");
	cout << "read labeling complete ...  begin AC" << endl;

	cout << "AC begin " << endl;
	t1 = MyTimer();
	int ac_count = tc->AC(2, &relation_construct_time);
	t2 = MyTimer();
	double t;
	cout << ">>>>>>>>>>>AC join time: " << (t = MyTime(t1, t2)) << " node delete: " << ac_count <<endl;
	cout << "	only relation Construct time: " << relation_construct_time << " ms" << endl;
	cout << "	only AC time: " << (t - relation_construct_time) << " ms"<<endl;
	//group->Show();
	cout << endl;

	

	
	tc->PCGenerateQ();
	int mr_size = 0;
#if 1
	cout << endl;
	t1 = MyTimer();
	mr_size = tc->EqualJoin5();
	t2 = MyTimer();
	cout << ">>>>>>>>>>>After AC nature join time: " << MyTime(t1, t2) << "ms, mr size: " << mr_size << endl;
	cout << endl;
#endif

	cout << "PC begin " << endl;
	t1 = MyTimer();
	tc->PC();
	t2 = MyTimer();
	cout << ">>>>>>>>>>>PC join time: " << MyTime(t1, t2) << " ms" << endl;
	//group->Show();
	cout << endl;

	//tc->WriteRelation(g_relation_acpc_path.data());

	cout << endl;
	t1 = MyTimer();
	mr_size = 0;
	mr_size  = tc->EqualJoin5();
	t2 = MyTimer();
	cout << ">>>>>>>>>>>After AC PC nature join time: " << MyTime(t1, t2) << "ms, mr size: "<< mr_size <<endl;
	cout << endl;

	//tc->WriteNaturalJoin(g_mr_acpc_nj_path.data());
	//tc->ResultInfo(g_mr_ac_path.data());

	delete(group);
	delete(q);
	delete(g);
	delete(tc);
}

void OnlineEMRJoin(){
	cout << endl;
	cout << "online EMR join" << endl;
	__int64 t1, t2, t3, t4, t;
	double relation_construct_time = 0;

	Graph *g = new Graph(kGraphVertexNum, false);
	g->ReadEdges((char*)g_graph_path.data(), ",");
	g->ReadLabels(g_label_path.data(), ",");
	cout << "read graph complete... with label " << g->label_num() << endl;

	Query *q = new Query(kQueryVertexNum, kDelta);
	q->ReadLablePairs(g_query_path.data(), ",");
	cout << "read q complete..." << endl;

	Group *group = new Group(g);

	TwohopAc *tc = new TwohopAc(g, q, group);
	tc->ReadLabeling(g_twohop_path.data(), ",");
	cout << "read labeling compelte... begin relation construct" << endl;
	t1 = MyTimer();
	tc->RelationConstruct(2); /*naive method*/
	t2 = MyTimer();
#if 0 //debug
	tc->RelationClear(); 
	tc->RelationConstruct(1); /*hash  method debug*/
#endif
	
	//tc->WriteRelation(g_relation_path.data());
	cout << "== only relation Construct time: " << MyTime(t1, t2) << " ms" << endl;
	int mr_size = 0;
#if 0  //equal join 3 not necessary
	t3 = MyTimer();
	mr_size = tc->EqualJoin3();
	t4 = MyTimer();

	
	cout << "	only equi join time: " << MyTime(t3, t4) << " ms" << endl;
	t = MyTime(t1, t2) + MyTime(t3, t4);
	cout << ">>>>>>>>>>>  equal join 3 time: " << t << " ms  MR size:" << mr_size << endl;
	cout << endl;
#endif 
	t3 = MyTimer();
	mr_size = tc->EqualJoin5();
	t4 = MyTimer();
	cout << "	only equi join time: " << MyTime(t3, t4) << " ms" << endl;
	cout << ">>>>>>>>>>>  equal join 3  MR size:" << mr_size << endl;
	cout << endl;


	tc->WriteNaturalJoin(g_mr_nj_path.data());

	delete(group);
	delete(q);
	delete(g);
	delete(tc);
}
void TestTimer(){
	__int64 t1, t2, t3, t4, t;
	t3 = MyTimer();
	t4 = MyTimer();
	cout << endl;
	cout << "= test timer: " << MyTime(t3, t4) << " ms" << endl;
	cout << endl;
}
int _tmain(int argc, _TCHAR* argv[]){
	TestTimer();
	cout <<"********* " <<g_path.data() <<" **********" <<endl;
	cout << "kGraphVertexNum " << kGraphVertexNum << endl;
	cout << "kQueryVertexNum " << kQueryVertexNum << endl; 
	cout << "kDelta " << kDelta << endl;
	cout << "kClusterK " << kClusterK << endl; 
	cout << "KClusterMaxIteration " << KClusterMaxIteration << endl;
	cout << endl;
#if 0
	//tool 
	GraphEdgeTransform(g_org_graph_path.data(), "\t", g_graph_path.data(), 
		g_org_label_path.data(), g_label_path.data());
#endif
	/*init rand seed*/
	srand((unsigned int)time(NULL));
#if 1
	OffLineTwohop();

	//OffLineEmbedding();
	
	//OffLineCluster();
#endif 
#if 1

	OnlineEMRJoin();

	//OnlineFjoin();

	//OnLineEdgeJoin();
#endif

	cout << "end!" << endl;
	int test;
	cin >> test;
	return 0;
}

