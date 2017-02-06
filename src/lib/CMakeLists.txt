set(SOURCES
    any.c
    application.c
    arguments.c
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
    hash.c
    hierarchy.c
    int.c
    list.c
    logging.c
    method.c
    mutex.c
    name.c
    nvp.c
    pointer.c
    range.c
    set.c
    resolve.c
    str.c
    strutils.c
    thread.c
    threadonce.c
    timer.c
    typedescr.c
    user.c
)

if (WITH_RE)
  set(SOURCES ${SOURCES} re.c)
endif (WITH_RE)

add_library(oblcore SHARED ${SOURCES})
if(PCRE2_FOUND)
  target_link_libraries(oblcore PUBLIC ${PCRE2_LIBRARIES})
endif(PCRE2_FOUND)
if(HAVE_GETUSERNAME)
  target_link_libraries(oblcore PUBLIC "secur32")
endif(HAVE_GETUSERNAME)

install(TARGETS oblcore
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

if(CHECK_FOUND)
  add_subdirectory(test)
endif(CHECK_FOUND)
