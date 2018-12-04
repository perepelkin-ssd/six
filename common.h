#ifndef COMMON_H_
#define COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#define WNIMPL { \
	fprintf(stderr, "\033[33;1mWARNING:\033[0;1m Code not implemented: \033[34m%s:%d\033[0m\n", __FILE__, __LINE__); \
}

#define NIMPL { \
	fprintf(stderr, "\033[31;1mERROR:\033[0;1m Code not implemented: \033[34m%s:%d\033[0m\n", __FILE__, __LINE__); \
	print_trace(); \
	abort(); \
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void print_trace();

class Printable
{
public:
	virtual ~Printable() {}

	virtual std::string toString() const;
};

std::ostream &operator <<(std::ostream &, const Printable &);

#endif // COMMON_H_
