#ifndef COMMON_H_
#define COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#define ENABLE_NOTES false
extern std::string note_prefix;
#define NOTE(msg) if (ENABLE_NOTES) printf("%s%s\033[0m " \
	"\033[2m%s:%d\033[0m\n", \
	note_prefix.c_str(), std::string(msg).c_str(), __FILE__, __LINE__)

#define WARN(msg) { \
	fprintf(stderr, "\033[33;1mWARNING:\033[0m %s \033[34m%s:%d\033[0m\n",\
	std::string(msg).c_str(), __FILE__, __LINE__); \
}

#define ABORT(msg) { \
	fprintf(stderr, "\033[31;1mERROR:\033[0;1m %s \033[34m%s:%d\033[0m\n",\
	std::string(msg).c_str(), __FILE__, __LINE__); \
	print_trace(stderr); \
	abort(); \
}

#define WNIMPL { \
	fprintf(stderr, "\033[33;1mWARNING:\033[0;1m Code not implemented: \033[34m%s:%d\033[0m\n", __FILE__, __LINE__); \
}

#define NIMPL { \
	fprintf(stderr, "\033[31;1mERROR:\033[0;1m Code not implemented: \033[34m%s:%d\033[0m\n", __FILE__, __LINE__); \
	abort(); \
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void print_trace(FILE *);

#endif // COMMON_H_
