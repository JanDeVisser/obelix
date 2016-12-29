find_package(Sqlite3)
find_package(Pgsql)

set(SOURCES sql.c)
set(LIBS oblnet oblvm oblcore)
set(INCLUDES "")
set(LIBDIRS "")
if(SQLITE3_FOUND)
    set(SOURCES ${SOURCES} sqlite.c)
    set(INCLUDES ${INCLUDES} ${SQLITE3_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})
    set(LIBDIRS ${LIBDIRS} ${SQLITE3_LIBRARY_DIRS})
endif(SQLITE3_FOUND)
if(PGSQL_FOUND)
    set(SOURCES ${SOURCES} pgsql.c)
    set(LIBS ${LIBS} ${PGSQL_LIBRARIES})
    set(INCLUDES ${INCLUDES} ${PGSQL_INCLUDE_DIRS})
    set(LIBDIRS ${LIBDIRS} ${PGSQL_LIBRARY_DIRS})
endif(PGSQL_FOUND)
if(MYSQL_FOUND)
    set(SOURCES ${SOURCES} mysql.c)
    set(LIBS ${LIBS} ${MYSQL_LIBRARIES})
    set(INCLUDES ${INCLUDES} ${MYSQL_INCLUDE_DIRS})
    set(LIBDIRS ${LIBDIRS} ${MYSQL_LIBRARY_DIRS})
endif(MYSQL_FOUND)

add_library(oblsql SHARED
    ${SOURCES}
)

target_link_libraries(oblsql PUBLIC ${LIBS})
target_include_directories(oblsql PRIVATE ${INCLUDES})
link_directories(oblsql ${LIBDIRS})

install(TARGETS oblsql
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
