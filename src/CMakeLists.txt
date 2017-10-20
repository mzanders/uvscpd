find_package (Threads)
add_executable (uvscpd controller.c syserror.c tcpserver.c uvscpd.c)
target_link_libraries (uvscpd ${CMAKE_THREAD_LIBS_INIT})

