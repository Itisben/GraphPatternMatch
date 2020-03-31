#ifndef IO_H_
#define IO_H_

#define DATA_LEN 4

typedef vector<int> Data;
typedef list<Data> Datas;


int ReadFile(Datas *datas,const char* filename, char* delim);
int WriteFile(char* filename,const char* delim, Datas *datas);
int WriteLog(char* filename, const char *str_data);

typedef int(*IoCallBackFun)(vector<int> *data, void **argv);
int ReadFileCallBack(const char* filename, const char* delim, IoCallBackFun fun, void **argv);
int WriteileCallBack(const char* filename, const char* delim, IoCallBackFun fun, void **argv);
#endif