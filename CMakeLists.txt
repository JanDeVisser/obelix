include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)

cmake_minimum_required(VERSION 3.2)
project(obelix)

# TODO: Make options for:
# - Windows 32 bit build (-w32)
# - Debug build (CMAKE_BUILD_TYPE=Debug)

# Uncomment if you want debug output:
# set(CMAKE_C_FLAGS -m32)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# The version number.
set(obelix_NAME Obelix)
set(obelix_VERSION_MAJOR 1)
set(obelix_VERSION_MINOR 0)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
set(obelix_DATADIR ${CMAKE_INSTALL_PREFIX}/share)
find_package (Threads)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(langinfo.h HAVE_LANGINFO_H)
check_include_file(libintl.h HAVE_LIBINTL_H)
check_include_file(locale.h HAVE_LOCALE_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(regex.h HAVE_REGEX_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(wchar.h HAVE_WCHAR_H)
check_include_file(wctype.h HAVE_WCTYPE_H)
check_include_file(windows.h HAVE_WINDOWS_H)
if (HAVE_WINDOWS_H AND NOT HAVE_SYS_SOCKET_H)
  check_include_file(winsock2.h HAVE_WINSOCK2_H)
  check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)
  check_include_file(io.h HAVE_IO_H)
endif (HAVE_WINDOWS_H AND NOT HAVE_SYS_SOCKET_H)

set(CMAKE_EXTRA_INCLUDE_FILES stdint.h)
check_type_size(intptr_t INTPTR_T)
set(CMAKE_EXTRA_INCLUDE_FILES)

if(HAVE_ALLOCA_H)
  set(ALLOCA_HDR alloca.h)
elseif(HAVE_MALLOC_H)
  set(ALLOCA_HDR malloc.h)
endif(HAVE_ALLOCA_H)
check_symbol_exists(alloca ${ALLOCA_HDR} HAVE_ALLOCA)
check_function_exists(mempcpy HAVE_MEMPCPY)
check_function_exists(putenv HAVE_PUTENV)
if(NOT HAVE_PUTENV)
  check_function_exists(_putenv HAVE__PUTENV)
endif(NOT HAVE_PUTENV)

set(CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_function_exists(itoa HAVE_ITOA)
check_function_exists(_itoa HAVE__ITOA)
check_function_exists(dtoa HAVE_DTOA)
check_function_exists(_dtoa HAVE__DTOA)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
set(CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(vasprintf HAVE_VASPRINTF)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_REQUIRED_DEFINITIONS)

if(HAVE_PTHREAD_H)
  set(CMAKE_REQUIRED_DEFINITIONS, -D_GNU_SOURCE)
  set(CMAKE_EXTRA_INCLUDE_FILES pthread.h)
  check_function_exists(pthread_setname_np HAVE_PTHREAD_SETNAME_NP)
  check_function_exists(pthread_yield HAVE_PTHREAD_YIELD)
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
  set(_SOCKET_HDR sys/socket.h)
  check_symbol_exists(gethostname unistd.h HAVE_GETHOSTNAME)
endif(HAVE_SYS_SOCKET_H)

if(HAVE_WINDOWS_H)
  check_symbol_exists(GetSystemInfo windows.h HAVE_GETSYSTEMINFO)
  check_symbol_exists(GetVersionEx windows.h HAVE_GETVERSIONEX)
  check_symbol_exists(FlushFileBuffers windows.h HAVE_FLUSHFILEBUFFERS)
  check_symbol_exists(CreateSemaphore windows.h HAVE_CREATESEMAPHORE)
  check_symbol_exists(CreateThread windows.h HAVE_CREATETHREAD)
  check_symbol_exists(InitializeCriticalSection windows.h HAVE_INITIALIZECRITICALSECTION)
endif(HAVE_WINDOWS_H)

if(HAVE_IO_H)
  set(CMAKE_EXTRA_INCLUDE_FILES io.h)
  check_function_exists(_findfirst HAVE__FINDFIRST)
endif(HAVE_IO_H)

if(HAVE_WINSOCK2_H)
  set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h)
  check_type_size(SOCKET TYPE_SOCKET)
  set(CMAKE_EXTRA_INCLUDE_FILES)
  set(_SOCKET_HDR winsock2.h)
  check_symbol_exists(gethostname winsock2.h HAVE_GETHOSTNAME)
endif(HAVE_WINSOCK2_H)

check_symbol_exists(SO_REUSEADDR sys/socket.h HAVE_SO_REUSEADDR)
check_symbol_exists(SO_NOSIGPIPE sys/socket.h HAVE_SO_NOSIGPIPE)
check_symbol_exists(MSG_NOSIGNAL sys/socket.h HAVE_MSG_NOSIGNAL)

check_symbol_exists(MAX_PATH stdlib.h HAVE_MAX_PATH)
if(NOT HAVE_MAX_PATH)
  check_symbol_exists(_MAX_PATH stdlib.h HAVE__MAX_PATH)
  if(NOT HAVE__MAX_PATH)
    check_symbol_exists(PATH_MAX limits.h HAVE_PATH_MAX)
  endif(NOT HAVE__MAX_PATH)
endif(NOT HAVE_MAX_PATH)
check_symbol_exists(ECONNRESET errno.h HAVE_ECONNRESET)
check_symbol_exists(fsync unistd.h HAVE_FSYNC)
check_symbol_exists(getline unistd.h HAVE_FSYNC)

set(CMAKE_EXTRA_INCLUDE_FILES string.h)
check_function_exists(_strdup HAVE__STRDUP)
check_function_exists(_stricmp HAVE__STRICMP)
check_function_exists(_strnicmp HAVE__STRNICMP)

set(CMAKE_EXTRA_INCLUDE_FILES strings.h)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strncasecmp HAVE_STRNCASECMP)

set(CMAKE_EXTRA_INCLUDE_FILES wchar.h)
check_function_exists(mbrtowc HAVE_MBRTOWC)
check_function_exists(wcrtomb HAVE_WCRTOMB)
check_function_exists(mbrtowc HAVE_WCSCOLL)


###############################################################################
# Check for integer types
# (The following are used in check.h. Regardless if they are used in
# the project, they will need to be checked in order to use Check).
check_type_size(intmax_t INTMAX_T)
check_type_size(uintmax_t UINTMAX_T)

check_type_size(pid_t PID_T)
if(NOT HAVE_PID_T)
  if(WIN32)
    set(pid_t "int")
  else(WIN32)
    MESSAGE(FATAL_ERROR "pid_t doesn't exist on this platform?")
  endif(WIN32)
endif(NOT HAVE_PID_T)
###############################################################################

if (CMAKE_COMPILER_IS_GNUCC)
  set(_GNU_SOURCE, 1)
endif (CMAKE_COMPILER_IS_GNUCC)

###############################################################################
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

check_library_exists(rt timer_create "" HAVE_LIBRT)

set(SYSLIBS "m")
if(HAVE_LIBRT)
  set(SYSLIBS "rt" ${SYSLIBS})
endif(HAVE_LIBRT)
set(SYSLIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${SYSLIBS})

include_directories("${PROJECT_SOURCE_DIR}/include")

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

add_subdirectory(src)
add_subdirectory(share)
#add_subdirectory(include)

# install(FILES "${PROJECT_BINARY_DIR}/config.h" DESTINATION include)
enable_testing()
add_test(NAME tcore COMMAND tcore)
add_test(NAME tlexer COMMAND tlexer)
