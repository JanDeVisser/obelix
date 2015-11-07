if (NOT HAVE_REGEX_H)
	add_subdirectory(regex)
endif (NOT HAVE_REGEX_H)

add_subdirectory(lib)
add_subdirectory(parser)
add_subdirectory(bin)
#add_subdirectory(stdlib)
