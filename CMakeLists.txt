include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

cmake_minimum_required (VERSION 2.6)
project(obelix)

# The version number.
set(obelix_VERSION_MAJOR 1)
set(obelix_VERSION_MINOR 0)
set(obelix_DATADIR ${PROJECT_BINARY_DIR}/share)

check_include_file(langinfo.h HAVE_LANGINFO_H)
check_include_file(locale.h HAVE_LOCALE_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(regex.h HAVE_REGEX_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(winsock2.h HAVE_WINSOCK2_H)
check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)

set(CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_function_exists(itoa HAVE_ITOA)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_REQUIRED_DEFINITIONS, -D_GNU_SOURCE)
set(CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(vasprintf HAVE_VASPRINTF)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_REQUIRED_DEFINITIONS)

if(HAVE_PTHREAD_H)
	set(CMAKE_REQUIRED_DEFINITIONS, -D_GNU_SOURCE)
	set(CMAKE_EXTRA_INCLUDE_FILES pthread.h)
	check_function_exists(pthread_setname_np HAVE_PTHREAD_SETNAME_NP)
	set(CMAKE_EXTRA_INCLUDE_FILES)
	set(CMAKE_REQUIRED_DEFINITIONS)
endif(HAVE_PTHREAD_H)

if(HAVE_SIGNAL_H)
	set(CMAKE_EXTRA_INCLUDE_FILES signal.h)
endif(HAVE_SIGNAL_H)
check_function_exists(kill HAVE_KILL)
set(CMAKE_EXTRA_INCLUDE_FILES)

if(HAVE_SYS_SOCKET_H)
	set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
	check_type_size(socklen_t SOCKLEN_T)
	set(CMAKE_EXTRA_INCLUDE_FILES)
endif(HAVE_SYS_SOCKET_H)

if(HAVE_WINSOCK2_H)
	set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h)
	check_type_size(SOCKET TYPE_SOCKET)
	set(CMAKE_EXTRA_INCLUDE_FILES)
endif(HAVE_WINSOCK2_H)

check_symbol_exists(ECONNRESET errno HAVE_ECONNRESET)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}/include")

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

add_subdirectory(src)
#add_subdirectory(share)

install(FILES "${PROJECT_BINARY_DIR}/config.h" DESTINATION include)