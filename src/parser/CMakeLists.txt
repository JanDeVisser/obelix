add_library(oblparser SHARED
	boundmethod.c
	bytecode.c
	closure.c
	generator.c
	grammar.c
	grammarparser.c
	instruction.c
	lexer.c
	namespace.c
	object.c
	parser.c
	script.c
	stacktrace.c
	token.c
	vm.c)

add_executable(panoramix panoramix.c)
target_link_libraries(panoramix oblcore oblparser)

install(TARGETS oblparser DESTINATION lib)
install(TARGETS panoramix DESTINATION bin)