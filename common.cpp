#include "common.h"

#include <execinfo.h>

#include <cassert>
#include <sstream>
#include <typeinfo>

void print_trace(FILE *out)
{
    assert(out != NULL);
    const size_t max_depth = 100;
    size_t stack_depth;
    void *stack_addrs[max_depth];
    char **stack_strings;

    stack_depth = backtrace(stack_addrs, max_depth);
    stack_strings = backtrace_symbols(stack_addrs, stack_depth);

    fprintf(out, "Call stack:\n");

    for (size_t i = 1; i < stack_depth; i++) {
		  std::string addr(stack_strings[i]);
		  addr = addr.substr(addr.find('[')+1);
		  addr = addr.substr(0, addr.find(']'));
		  addr = std::string() + "addr2line " + addr + " -e six";
		  std::cout << i << " " << std::flush;
		  if(system(addr.c_str())<0) abort();
    }
    free(stack_strings); // malloc()ed by backtrace_symbols
    fflush(out);
	abort();
}


void print_trace2()
 {
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    int child_pid = fork();
    if (!child_pid) {           
        dup2(2,1); // redirect output to stderr
        fprintf(stdout,"stack trace for %s pid=%s\n",name_buf,pid_buf);
        execlp("gdb", "gdb", "--batch", /*"-n",*/ "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
        abort(); /* If gdb failed to start */
    } else {
        waitpid(child_pid,NULL,0);
    }
}

