add_library(oblsqllib SHARED
    sql.c
)
if(WITH_SQLITE3)
    set(SOURCES ${SOURCES} sqlite.c)
endif(WITH_SQLITE3)
if(WITH_POSTGRESQL)
    set(SOURCES ${SOURCES} pgsql.c)
endif(WITH_POSTGRESQL)
if(WITH_MYSQL)
    set(SOURCES ${SOURCES} mysql.c)
endif(WITH_MYSQL)

target_link_libraries(oblsqllib PUBLIC oblnet oblvm oblcore)

install(TARGETS oblstdlib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
