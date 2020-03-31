// stdafx.cpp : source file that includes just the standard includes
// GraphPatternMatch-New-1.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

/*num: the number of random number that we create.*/
int MyRandRange(pair<int, int> range, int num, vector<int> *result){
	if (range.second < range.first)
		return -1;

	int range_size = range.second - range.first + 1;
	vector<int> temp_vector;

	temp_vector.reserve(range_size);
	result->reserve(num);
	result->clear();

	for (int i = 0; i < range_size; i++){
		temp_vector.push_back(range.first + i);
	}
	for (int i = 0; i < range_size; i++){
		int index = MyRand() % range_size;
		swap(temp_vector[i], temp_vector[index]);
	}
	for (int i = 0; i < num; i++){
		result->push_back(temp_vector[i]);
	}
	return range_size;
}

int MyRandRange(vector<int> *range, int num){
	int range_size = range->size();
	for (int i = 0; i < num; i++){
		int index = rand() % range_size;
		swap(range->at(i), range->at(index));
	}
	return range_size;
}