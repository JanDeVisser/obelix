add_library(oblstdlib SHARED
	builtins.c
	sys.c
	net.c
)

target_link_libraries(oblstdlib PUBLIC oblparser)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblstdlib PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS oblstdlib DESTINATION lib)
