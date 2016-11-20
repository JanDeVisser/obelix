set(SOURCES
  grammar_action.c
  grammar_element.c
  grammar_variable.c
  grammar.c
  nonterminal.c
  rule.c
  rule_entry.c
  grammarparser.c
)
add_library(oblgrammar SHARED ${SOURCES})
add_library(soblgrammar STATIC ${SOURCES})

target_link_libraries(oblgrammar PUBLIC oblcore obllexer)

add_executable(panoramix panoramix.c)
target_compile_definitions(panoramix PRIVATE OBL_STATIC)
if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_definitions(panoramix PRIVATE _GNU_SOURCE)
    target_compile_definitions(oblgrammar PRIVATE _GNU_SOURCE)
    target_compile_definitions(soblgrammar PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
target_link_libraries(panoramix soblgrammar sobllexer soblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS panoramix oblgrammar soblgrammar
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

add_subdirectory(test)
