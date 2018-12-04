#include "common.h"

#include <sstream>
#include <typeinfo>

void print_trace()
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


std::ostream &operator <<(std::ostream &s, const Printable &obj)
{
	s << obj.toString();
	return s;
}

std::string Printable::toString() const
{
	std::ostringstream os;

	os << "Object<" << typeid(*this).name() << ">";

	return os.str();
}

