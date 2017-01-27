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

add_executable(panoramix panoramix.c)
target_link_libraries(panoramix oblgrammar obllexer oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS panoramix oblgrammar
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

if(CHECK_FOUND)
  add_subdirectory(test)
endif(CHECK_FOUND)
