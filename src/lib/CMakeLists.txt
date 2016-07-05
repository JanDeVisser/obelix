set(SOURCES 
    any.c
    array.c
    core.c
    data.c
    datalist.c
    datastack.c
    dict.c
    exception.c
    file.c
    float.c
    fsentry.c
    function.c
    int.c
    list.c
    logging.c
    method.c
    mutex.c
    name.c
    nvp.c
    pointer.c
    range.c
    re.c
    set.c
    resolve.c
    socket.c
    str.c
    thread.c
    typedescr.c
)
# disabled: user.c

if (NOT HAVE_REGEX_H)
  set(SOURCES ${SOURCES} regex.c)
endif (NOT HAVE_REGEX_H)

add_library(oblcore SHARED ${SOURCES})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblcore PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
if (NOT HAVE_REGEX_H)
  target_compile_definitions(oblcore PRIVATE BUILD_REGEX_DLL;HAVE_CONFIG_H)
endif (NOT HAVE_REGEX_H)
if(HAVE_WINSOCK2_H)
  target_link_libraries(oblcore PUBLIC ws2_32)
endif(HAVE_WINSOCK2_H)

install(TARGETS oblcore DESTINATION lib)

add_subdirectory(test)