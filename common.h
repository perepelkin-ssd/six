#ifndef COMMON_H_
#define COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#define WNIMPL { \
	fprintf(stderr, "WARNING: Code not implemented: %s:%d\n", __FILE__, __LINE__); \
}

#define NIMPL { \
	fprintf(stderr, "ERROR: Code not implemented: %s:%d\n", __FILE__, __LINE__); \
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
