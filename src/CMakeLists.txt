find_package (Threads)
add_executable (uvscpd controller.c syserror.c tcpserver.c uvscpd.c tcpserver_worker.c cmd_interpreter.c tcpserver_commands.c vscp.c vscp_buffer.c)
target_link_libraries (uvscpd ${CMAKE_THREAD_LIBS_INIT})

set(CMAKE_BUILD_TYPE Debug)
