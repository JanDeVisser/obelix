add_library(oblgrammar SHARED
  grammar_action.c
  grammar_element.c
  grammar_variable.c
  grammar.c
  nonterminal.c
  rule.c
  rule_entry.c
  grammarparser.c
)

target_link_libraries(oblgrammar PUBLIC oblcore obllexer)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblgrammar PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(panoramix panoramix.c)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(panoramix PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
target_link_libraries(panoramix oblgrammar obllexer oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS panoramix oblgrammar
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

add_subdirectory(test)
