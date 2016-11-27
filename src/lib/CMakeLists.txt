set(SOURCES
    any.c
    array.c
    core.c
    data.c
    datalist.c
    datastack.c
    dict.c
    dictionary.c
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
    str.c
    strutils.c
    thread.c
    typedescr.c
)
# disabled: user.c

if (NOT HAVE_REGEX_H)
  set(SOURCES ${SOURCES} regex.c)
endif (NOT HAVE_REGEX_H)

add_library(oblcore SHARED ${SOURCES})
add_library(soblcore STATIC ${SOURCES})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblcore PRIVATE _GNU_SOURCE)
  target_compile_definitions(soblcore PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
if (NOT HAVE_REGEX_H)
  target_compile_definitions(oblcore PRIVATE BUILD_REGEX_DLL;HAVE_CONFIG_H)
  target_compile_definitions(soblcore PRIVATE BUILD_REGEX_DLL;HAVE_CONFIG_H)
endif (NOT HAVE_REGEX_H)
if(HAVE_WINSOCK2_H)
  target_link_libraries(oblcore PUBLIC ws2_32)
endif(HAVE_WINSOCK2_H)

install(TARGETS oblcore soblcore
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
