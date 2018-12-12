#mkgen generated

six: buffer.o comm.o common.o context.o df_pusher.o df_requester.o factory.o fp.o fp_json.o id.o idle_stopper.o jfp.o locator.o logger.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o task.o tasks.o thread_pool.o thread_pool_test.o value.o
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror buffer.o comm.o common.o context.o df_pusher.o df_requester.o factory.o fp.o fp_json.o id.o idle_stopper.o jfp.o locator.o logger.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o task.o tasks.o thread_pool.o thread_pool_test.o value.o -o six -I .

buffer.o: buffer.cpp ./buffer.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o buffer.o buffer.cpp -I .

comm.o: comm.cpp ./buffer.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o comm.o comm.cpp -I .

common.o: common.cpp ./common.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o common.o common.cpp -I .

context.o: context.cpp ./printable.h ./common.h ./buffer.h ./serializable.h ./stags.h ./json.h ./context.h ./factory.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o context.o context.cpp -I .

df_pusher.o: df_pusher.cpp ./printable.h ./id.h ./buffer.h ./serializable.h ./thread_pool.h ./df_pusher.h ./json.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o df_pusher.o df_pusher.cpp -I .

df_requester.o: df_requester.cpp ./df_requester.h ./printable.h ./id.h ./buffer.h ./serializable.h ./thread_pool.h ./json.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o df_requester.o df_requester.cpp -I .

factory.o: factory.cpp ./stags.h ./buffer.h ./factory.h ./serializable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o factory.o factory.cpp -I .

fp.o: fp.cpp ./fp.h ./printable.h ./common.h ./buffer.h ./serializable.h ./stags.h ./json.h ./context.h ./factory.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o fp.o fp.cpp -I .

fp_json.o: fp_json.cpp ./jfp.h ./remote_monitor.h ./common.h ./thread_pool.h ./df_requester.h ./environ.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./json.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./fp_json.h ./stags.h ./context.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o fp_json.o fp_json.cpp -I .

id.o: id.cpp ./printable.h ./id.h ./buffer.h ./common.h ./serializable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o id.o id.cpp -I .

idle_stopper.o: idle_stopper.cpp ./idle_stopper.h ./common.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o idle_stopper.o idle_stopper.cpp -I .

jfp.o: jfp.cpp ./jfp.h ./remote_monitor.h ./common.h ./rptr.h ./df_requester.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./json.h ./factory.h ./value.h ./fp.h ./buf_handler.h ./printable.h ./environ.h ./tasks.h ./stags.h ./context.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o jfp.o jfp.cpp -I .

locator.o: locator.cpp ./buffer.h ./serializable.h ./stags.h ./printable.h ./common.h ./locator.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o locator.o locator.cpp -I .

logger.o: logger.cpp ./logger.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o logger.o logger.cpp -I .

main.o: main.cpp ./idle_stopper.h ./jfp.h ./mpi_comm.h ./common.h ./thread_pool.h ./df_requester.h ./rts.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./json.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./fp_json.h ./logger.h ./tasks.h ./stags.h ./context.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o main.o main.cpp -I .

mpi_comm.o: mpi_comm.cpp ./mpi_comm.h ./comm.h ./common.h ./buffer.h ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o mpi_comm.o mpi_comm.cpp -I .

mpi_comm_test.o: mpi_comm_test.cpp ./mpi_comm.h ./buffer.h ./thread_pool.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o mpi_comm_test.o mpi_comm_test.cpp -I .

printable.o: printable.cpp ./printable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o printable.o printable.cpp -I .

remote_monitor.o: remote_monitor.cpp ./remote_monitor.h ./buf_handler.h ./buffer.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o remote_monitor.o remote_monitor.cpp -I .

rptr.o: rptr.cpp ./buffer.h ./serializable.h ./rptr.h ./stags.h ./printable.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o rptr.o rptr.cpp -I .

rts.o: rts.cpp ./idle_stopper.h ./df_requester.h ./common.h ./thread_pool.h ./rts.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./json.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./logger.h ./stags.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o rts.o rts.cpp -I .

task.o: task.cpp ./df_requester.h ./buf_handler.h ./printable.h ./common.h ./rptr.h ./environ.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./json.h ./comm.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o task.o task.cpp -I .

tasks.o: tasks.cpp ./df_requester.h ./common.h ./thread_pool.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./json.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./tasks.h ./stags.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o tasks.o tasks.cpp -I .

thread_pool.o: thread_pool.cpp ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o thread_pool.o thread_pool.cpp -I .

thread_pool_test.o: thread_pool_test.cpp ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o thread_pool_test.o thread_pool_test.cpp -I .

value.o: value.cpp ./buffer.h ./serializable.h ./json.h ./stags.h ./printable.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -Wall -Werror -c -o value.o value.cpp -I .

clean:
	rm -f six
	rm -f buffer.o
	rm -f comm.o
	rm -f common.o
	rm -f context.o
	rm -f df_pusher.o
	rm -f df_requester.o
	rm -f factory.o
	rm -f fp.o
	rm -f fp_json.o
	rm -f id.o
	rm -f idle_stopper.o
	rm -f jfp.o
	rm -f locator.o
	rm -f logger.o
	rm -f main.o
	rm -f mpi_comm.o
	rm -f mpi_comm_test.o
	rm -f printable.o
	rm -f remote_monitor.o
	rm -f rptr.o
	rm -f rts.o
	rm -f task.o
	rm -f tasks.o
	rm -f thread_pool.o
	rm -f thread_pool_test.o
	rm -f value.o
