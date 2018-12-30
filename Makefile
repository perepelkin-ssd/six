#mkgen generated

six: buffer.o comm.o common.o context.o df_pusher.o df_requester.o factory.o fp_json.o fp_util.o id.o idle_stopper.o jfp.o locator.o logger.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o so_lib.o task.o tasks.o thread_pool.o thread_pool_test.o value.o ucenv/ucenv.o
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include buffer.o comm.o common.o context.o df_pusher.o df_requester.o factory.o fp_json.o fp_util.o id.o idle_stopper.o jfp.o locator.o logger.o main.o mpi_comm.o mpi_comm_test.o printable.o remote_monitor.o rptr.o rts.o so_lib.o task.o tasks.o thread_pool.o thread_pool_test.o value.o ucenv/ucenv.o -o six -I . -ldl ~/local/lib/libdyncall_s.a -lpthread ../lix/lib/lix.a

buffer.o: buffer.cpp ./common.h ./buffer.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o buffer.o buffer.cpp -I .

comm.o: comm.cpp ./printable.h ./common.h ./buffer.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o comm.o comm.cpp -I .

common.o: common.cpp ./common.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o common.o common.cpp -I .

context.o: context.cpp ./printable.h ./common.h ./buffer.h ./serializable.h ./stags.h ./context.h ./factory.h ./id.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o context.o context.cpp -I .

df_pusher.o: df_pusher.cpp ./printable.h ./id.h ./buffer.h ./serializable.h ./thread_pool.h ./df_pusher.h ./common.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o df_pusher.o df_pusher.cpp -I .

df_requester.o: df_requester.cpp ./df_requester.h ./printable.h ./common.h ./buffer.h ./serializable.h ./thread_pool.h ./id.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o df_requester.o df_requester.cpp -I .

factory.o: factory.cpp ./stags.h ./common.h ./buffer.h ./factory.h ./serializable.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o factory.o factory.cpp -I .

fp_json.o: fp_json.cpp ./jfp.h ./remote_monitor.h ./common.h ./rptr.h ./df_requester.h ./fp_json.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./code_lib.h ./stags.h ./context.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o fp_json.o fp_json.cpp -I .

fp_util.o: fp_util.cpp ./locator.h ./printable.h ./common.h ./fp_util.h ./serializable.h ./stags.h ./buffer.h ./context.h ./factory.h ./id.h ./value.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o fp_util.o fp_util.cpp -I .

id.o: id.cpp ./printable.h ./id.h ./buffer.h ./common.h ./serializable.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o id.o id.cpp -I .

idle_stopper.o: idle_stopper.cpp ./idle_stopper.h ./common.h ./printable.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o idle_stopper.o idle_stopper.cpp -I .

jfp.o: jfp.cpp ./jfp.h ./remote_monitor.h ./common.h ./rptr.h ./df_requester.h ./df_pusher.h ./fp_util.h ./serializable.h ./thread_pool.h ./task.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./code_lib.h ./tasks.h ./buffer.h ./stags.h ./context.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o jfp.o jfp.cpp -I .

locator.o: locator.cpp ./buffer.h ./serializable.h ./stags.h ./printable.h ./common.h ./locator.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o locator.o locator.cpp -I .

logger.o: logger.cpp ./logger.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o logger.o logger.cpp -I .

main.o: main.cpp ./idle_stopper.h ./jfp.h ./so_lib.h ./mpi_comm.h ./common.h ./rptr.h ./df_requester.h ./rts.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./factory.h ./value.h ./code_lib.h ./buf_handler.h ./printable.h ./environ.h ./fp_json.h ./logger.h ./tasks.h ./stags.h ./context.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o main.o main.cpp -I .

mpi_comm.o: mpi_comm.cpp ./buffer.h ./thread_pool.h ./mpi_comm.h ./printable.h ./common.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o mpi_comm.o mpi_comm.cpp -I .

mpi_comm_test.o: mpi_comm_test.cpp ./buffer.h ./thread_pool.h ./mpi_comm.h ./printable.h ./common.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o mpi_comm_test.o mpi_comm_test.cpp -I .

printable.o: printable.cpp ./printable.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o printable.o printable.cpp -I .

remote_monitor.o: remote_monitor.cpp ./remote_monitor.h ./buf_handler.h ./common.h ./buffer.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o remote_monitor.o remote_monitor.cpp -I .

rptr.o: rptr.cpp ./buffer.h ./serializable.h ./rptr.h ./stags.h ./printable.h ./common.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o rptr.o rptr.cpp -I .

rts.o: rts.cpp ./idle_stopper.h ./df_requester.h ./code_lib.h ./common.h ./thread_pool.h ./rts.h ./task.h ./buffer.h ./serializable.h ./rptr.h ./df_pusher.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./logger.h ./stags.h ./id.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o rts.o rts.cpp -I .

so_lib.o: so_lib.cpp ./so_lib.h ./code_lib.h ./printable.h ./common.h ./ucenv/ucenv.h ./buffer.h ./serializable.h ./id.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o so_lib.o so_lib.cpp -I .

task.o: task.cpp ./df_pusher.h ./df_requester.h ./buf_handler.h ./printable.h ./common.h ./rptr.h ./environ.h ./code_lib.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./comm.h ./id.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o task.o task.cpp -I .

tasks.o: tasks.cpp ./df_requester.h ./code_lib.h ./common.h ./rptr.h ./df_pusher.h ./buffer.h ./serializable.h ./thread_pool.h ./task.h ./factory.h ./value.h ./buf_handler.h ./printable.h ./environ.h ./tasks.h ./stags.h ./locator.h ./id.h ./comm.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o tasks.o tasks.cpp -I .

thread_pool.o: thread_pool.cpp ./printable.h ./common.h ./thread_pool.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o thread_pool.o thread_pool.cpp -I .

thread_pool_test.o: thread_pool_test.cpp ./printable.h ./thread_pool.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o thread_pool_test.o thread_pool_test.cpp -I .

value.o: value.cpp ./buffer.h ./serializable.h ./stags.h ./printable.h ./common.h ./id.h ./value.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o value.o value.cpp -I .

ucenv/ucenv.o: ucenv/ucenv.cpp ./ucenv/ucenv.h
	MPICH_CXX=g++-5 mpicxx -std=c++11 -Og -g -I ../lix/include -c -o ucenv/ucenv.o ucenv/ucenv.cpp -I .

clean:
	rm -f six
	rm -f buffer.o
	rm -f comm.o
	rm -f common.o
	rm -f context.o
	rm -f df_pusher.o
	rm -f df_requester.o
	rm -f factory.o
	rm -f fp_json.o
	rm -f fp_util.o
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
	rm -f so_lib.o
	rm -f task.o
	rm -f tasks.o
	rm -f thread_pool.o
	rm -f thread_pool_test.o
	rm -f value.o
	rm -f ucenv/ucenv.o
