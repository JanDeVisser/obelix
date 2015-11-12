check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(libintl.h HAVE_LIBINTL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(wchar.h HAVE_WCHAR_H)
check_include_file(wctype.h HAVE_WCTYPE_H)

if(HAVE_ALLOCA_H)
	set(ALLOCA_HDR alloca.h)
elseif(HAVE_MALLOC_H)
	set(ALLOCA_HDR malloc.h)
endif(HAVE_ALLOCA_H)
check_symbol_exists(alloca ${ALLOCA_HDR} HAVE_ALLOCA)
check_function_exists(mempcpy HAVE_MEMPCPY)
set(CMAKE_EXTRA_INCLUDE_FILES wchar.h)
check_function_exists(wcrtomb HAVE_WCRTOMB)
check_function_exists(mbrtowc HAVE_MBRTOWC)
check_function_exists(mbrtowc HAVE_WCSCOLL)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config-re.h"
)

add_library(regex SHARED
	regex.c
)
target_compile_definitions(regex PRIVATE BUILD_REGEX_DLL;HAVE_CONFIG_H)

install(TARGETS regex DESTINATION lib)
install(FILES "regex.h" DESTINATION include)