add_library(oblcore SHARED 
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

install(TARGETS oblcore DESTINATION lib)
