// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <list>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>
#include <iostream>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <utility>
#include <windows.h>
#include <unordered_set>
#include "tool.h"

using namespace std;


typedef int ERR;
const int kInfinity = 10000000;

enum Dir{
	DOUT = 0,
	AIN = 1,
};
#define FOR_EACH_DIRECTION for (int dir = 0; dir <= 1; dir++){
#define END_FOR_EACH_DIRCTION }
int inline OPP(int i){ if (i == 0) return 1; else return 0; }

#define IGNORE_CUR_LINE '#'
#define IGNORE_BELOW_LINE '*'
#define DELIMITER		","

int inline MAX(int i, int j) { return i > j ? i : j; }

#define ERROR_INFO		cout << __FILE__ << " " << __FUNCTIONW__ << " " << __LINE__ << ": ";

#define COUT(str) if(debug_==1) {cout str}
#define ERR_COUT(str) ERROR_INFO cout str
//#define ERR_COUT(str)

int inline MyRand(){
	srand((unsigned int)time(NULL));
	return rand();
}

/*num: the number of random number that we create.*/
int MyRandRange(pair<int, int> range, int num, vector<int> *result);
int MyRandRange(vector<int> *range, int num);

// TODO: reference additional headers your program requires here
