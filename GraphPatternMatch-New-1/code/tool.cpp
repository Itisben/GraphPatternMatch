#include "stdafx.h"
#include "tool.h"
int GraphEdgeTransform(const char* src_filename, char* delim, const char* dest_filename, 
	const char* src_filename2, const char* dest_filename2){
	int err = 0;


	int error;
	FILE *fp;
	char strbuf[128] = { 0 };
	char *p;

	map<int, int> dic;
	vector<pair<int, int>> data;
	pair<int, int> edge;
	int index = 0;

	error = fopen_s(&fp, src_filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 128, fp) != NULL)
	{
		if (strlen(strbuf) < 4)
			continue;
		if (strbuf[0] == '#')
			continue;

		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;

		int i = atoi(p);
		if (dic.find(i) == dic.end()){
			dic.insert({ i, index });
			edge.first = index;
			index++;
		}
		else
		{
			edge.first = dic.at(i);
		}

		p = strtok(NULL, delim);
		if (NULL == p)
			continue;

		i = atoi(p);
		if (dic.find(i) == dic.end()){
			dic.insert({ i, index });
			edge.second = index;
			index++;
		}
		else
		{
			edge.second = dic.at(i);
		}

		data.push_back(edge);

	}
	fclose(fp);

	//write to file.
	error = fopen_s(&fp, dest_filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}
	string str;
	for (auto it = data.begin(); it != data.end(); it++)
	{
		str = "";
		str += itoa(it->first,strbuf, 10);
		str += ",";
		str += itoa(it->second, strbuf, 10);
		str += "\n";
		fwrite(str.data(), sizeof(char), str.length(), fp);
	}
	fclose(fp);

	cout << "graph has create" << endl;

	//read for label
	map<int, int> label_dic;
	pair<int, int> pair;
	data.clear();
	index = 1;
	error = fopen_s(&fp, src_filename2, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 128, fp) != NULL)
	{
		if (strlen(strbuf) < 4)
			continue;
		if (strbuf[0] == '#')
			continue;

		p = strtok(strbuf, ",");
		if (NULL == p)
			continue;

		int i = atoi(p);
		if (dic.find(i) != dic.end()){
			pair.first = dic.at(i);
		}

		p = strtok(NULL, delim);
		if (NULL == p)
			continue;

		i = atoi(p);
		if (label_dic.find(i) == label_dic.end()){
			label_dic.insert({ i, index });
			pair.second = index;
			index++;
		}
		else
		{
			pair.second = label_dic.at(i);
		}

		data.push_back(pair);
		//dic.erase(dic.find(i));
	}
#if 0
	for (auto it = dic.begin(); it != dic.end(); it++){
		pair.first = it->second;
		pair.second = 0;
		data.push_back(pair);
	}
#endif
	fclose(fp);

	//write to label.
	error = fopen_s(&fp, dest_filename2, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}
	for (auto it = data.begin(); it != data.end(); it++)
	{
		str = "";
		str += itoa(it->first, strbuf, 10);
		str += ",";
		str += itoa(it->second, strbuf, 10);
		str += "\n";
		fwrite(str.data(), sizeof(char), str.length(), fp);
	}
	fclose(fp);
	return 0;
}

__int64 MyTimer(){
	LARGE_INTEGER  large_interger;
	QueryPerformanceCounter(&large_interger);
	return large_interger.QuadPart;
}
double g_dff;


double MyTime(__int64 t1, __int64 t2){
	double dff;
	LARGE_INTEGER  large_interger;
	QueryPerformanceFrequency(&large_interger);
	dff = large_interger.QuadPart;
	return abs(t2 - t1) * 1000 / dff;
}
