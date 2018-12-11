#mkgen generated

six: buffer.o comm.o common.o df_pusher.o df_requester.o factory.o fp_json.o id.o idle_stopper.o jfp.o locator.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o task.o tasks.o thread_pool.o thread_pool_test.o value.o
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og buffer.o comm.o common.o df_pusher.o df_requester.o factory.o fp_json.o id.o idle_stopper.o jfp.o locator.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o task.o tasks.o thread_pool.o thread_pool_test.o value.o -o six -I .

buffer.o: buffer.cpp ./buffer.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o buffer.o buffer.cpp -I .

comm.o: comm.cpp ./buffer.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o comm.o comm.cpp -I .

common.o: common.cpp ./common.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o common.o common.cpp -I .

df_pusher.o: df_pusher.cpp ./buffer.h ./serializable.h ./thread_pool.h ./df_pusher.h ./printable.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o df_pusher.o df_pusher.cpp -I .

df_requester.o: df_requester.cpp ./df_requester.h ./buffer.h ./serializable.h ./thread_pool.h ./printable.h ./id.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o df_requester.o df_requester.cpp -I .

factory.o: factory.cpp ./stags.h ./buffer.h ./factory.h ./serializable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o factory.o factory.cpp -I .

fp_json.o: fp_json.cpp ./jfp.h ./remote_monitor.h ./id.h ./thread_pool.h ./df_requester.h ./environ.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./fp_json.h ./stags.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o fp_json.o fp_json.cpp -I .

id.o: id.cpp ./printable.h ./id.h ./buffer.h ./common.h ./serializable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o id.o id.cpp -I .

idle_stopper.o: idle_stopper.cpp ./idle_stopper.h ./common.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o idle_stopper.o idle_stopper.cpp -I .

jfp.o: jfp.cpp ./jfp.h ./remote_monitor.h ./common.h ./thread_pool.h ./df_requester.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./tasks.h ./stags.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o jfp.o jfp.cpp -I .

locator.o: locator.cpp ./buffer.h ./serializable.h ./stags.h ./printable.h ./common.h ./locator.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o locator.o locator.cpp -I .

main.o: main.cpp ./idle_stopper.h ./jfp.h ./mpi_comm.h ./common.h ./rptr.h ./df_requester.h ./rts.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./fp_json.h ./tasks.h ./stags.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o main.o main.cpp -I .

mpi_comm.o: mpi_comm.cpp ./mpi_comm.h ./comm.h ./common.h ./buffer.h ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o mpi_comm.o mpi_comm.cpp -I .

mpi_comm_test.o: mpi_comm_test.cpp ./mpi_comm.h ./buffer.h ./thread_pool.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o mpi_comm_test.o mpi_comm_test.cpp -I .

printable.o: printable.cpp ./printable.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o printable.o printable.cpp -I .

remote_monitor.o: remote_monitor.cpp ./remote_monitor.h ./buf_handler.h ./buffer.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o remote_monitor.o remote_monitor.cpp -I .

rptr.o: rptr.cpp ./buffer.h ./serializable.h ./rptr.h ./stags.h ./printable.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o rptr.o rptr.cpp -I .

rts.o: rts.cpp ./idle_stopper.h ./df_requester.h ./common.h ./rptr.h ./rts.h ./task.h ./buffer.h ./serializable.h ./thread_pool.h ./df_pusher.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./stags.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o rts.o rts.cpp -I .

task.o: task.cpp ./df_requester.h ./buf_handler.h ./printable.h ./common.h ./thread_pool.h ./environ.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./value.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o task.o task.cpp -I .

tasks.o: tasks.cpp ./df_requester.h ./common.h ./thread_pool.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./tasks.h ./stags.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o tasks.o tasks.cpp -I .

thread_pool.o: thread_pool.cpp ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o thread_pool.o thread_pool.cpp -I .

thread_pool_test.o: thread_pool_test.cpp ./thread_pool.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o thread_pool_test.o thread_pool_test.cpp -I .

value.o: value.cpp ./stags.h ./printable.h ./buffer.h ./serializable.h ./value.h
	MPICH_CXX=g++5 /home/perepelkin/local/bin/mpicxx -std=c++11 -lpthread -g -Og -c -o value.o value.cpp -I .

clean:
	rm -f six
	rm -f buffer.o
	rm -f comm.o
	rm -f common.o
	rm -f df_pusher.o
	rm -f df_requester.o
	rm -f factory.o
	rm -f fp_json.o
	rm -f id.o
	rm -f idle_stopper.o
	rm -f jfp.o
	rm -f locator.o
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
