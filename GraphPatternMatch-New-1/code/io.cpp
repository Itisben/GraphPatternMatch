#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "io.h"
#define IGNORE_CUR_LINE '#'
#define IGNORE_BELOW_LINE '*'

int ReadFile(Datas *datas,const char* filename, char* delim){
	int error;
	FILE *fp;
	char strbuf[64] = { 0 };
	char *p;
	Data data;

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 64, fp) != NULL)
	{
		if (0 == strlen(strbuf))
			continue;
		if (strbuf[0] == IGNORE_CUR_LINE)
			continue;
		if (strbuf[0] == IGNORE_BELOW_LINE)
			break;
		data.clear();
		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;

		data.push_back(atoi(p));
		for (int i = 1; (p = strtok(NULL, delim)) != NULL && i < DATA_LEN; i++){
			data.push_back(atoi(p));
		}		
		datas->push_back(data);
	}
	fclose(fp);
	return 0;
}

int WriteFile(char* filename, const char* delim, Datas *datas){
	int error;
	FILE *fp;
	string strbuf;
	Data data = { -1 };
	string str;
	char buf[16];

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	for (auto it = datas->begin(); it != datas->end(); it++)
	{
		strbuf = "";
		for (auto it2 = it->begin(); it2 != it->end(); it2++)
		{
			if (it2 != it->begin())
				strbuf += delim;
			strbuf += _itoa(*it2, buf, 10);		
		}
		fwrite(str.data(), sizeof(char), str.length(), fp);
	}
	
	fclose(fp);
	return 0;
}

int WriteLog(char* filename, const char *str_data){
	int error;
	FILE *fp;
	Data data = { -1 };

	error = fopen_s(&fp, filename, "w+");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	fwrite(str_data, sizeof(char), strlen(str_data), fp);
	
	fclose(fp);
	return 0;
}


int ReadFileCallBack(const char* filename, const char* delim, IoCallBackFun fun, void **argv){
	int error;
	FILE *fp;
	char strbuf[64] = { 0 };
	char *p;
	vector<int> data;

	error = fopen_s(&fp, filename, "r");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	while (fgets(strbuf, 64, fp) != NULL)
	{
		if (0 == strlen(strbuf))
			continue;
		if (strbuf[0] == IGNORE_CUR_LINE)
			continue;
		if (strbuf[0] == IGNORE_BELOW_LINE)
			break;

		data.clear();
		p = strtok(strbuf, delim);
		if (NULL == p)
			continue;
		data.push_back(atoi(p));
		for (int i = 1; (p = strtok(NULL, delim)) != NULL && i < DATA_LEN; i++){
			data.push_back(atoi(p));
		}

		fun(&data, argv);
		
	}
	fclose(fp);
	return 0;
}

int WriteFileCallBack(const char* filename, const char* delim, IoCallBackFun fun, void **argv){
	int error;
	FILE *fp;
	string strbuf;
	vector<int> data;
	string str;

	error = fopen_s(&fp, filename, "w");
	if (0 != error)
	{
		ERR_COUT(<< "open file failed! " << strerror(error) << endl;);
		return -1;
	}

	
	fwrite(str.data(), sizeof(char), str.length(), fp);
	

	fclose(fp);
	return 0;
}