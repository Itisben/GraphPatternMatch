#ifndef TOOL_H_
#define TOOL_H_

int GraphEdgeTransform(const char* src_filename, char* delim, const char* dest_filename,
	const char* src_filename2, const char* dest_filename2);

__int64 MyTimer();

double MyTime(__int64 t1, __int64 t2);

#endif