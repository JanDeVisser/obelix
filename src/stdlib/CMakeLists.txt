add_library(oblstdlib MODULE
	builtins.c
	sys.c
	net.c
)

target_link_libraries(oblstdlib PUBLIC oblparser oblvm oblnet)

install(TARGETS oblstdlib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
