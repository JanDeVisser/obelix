/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <config.h>
#include <core/Format.h>
#include <core/Resolve.h>
#include <cstdio>
#include <cstdlib>
#include <gtest/gtest.h>

namespace Obelix {

extern "C" size_t test_function_in_program_image(const char* text)
{
    return strlen(text);
}

TEST(Resolve, get_resolver)
{
    static char cwd[1024];
    getcwd(cwd, 1024);
    if (!strstr(cwd, "src/core/test"))
        putenv(const_cast<char*>("OBL_DIR=src/core/test"));
    [[maybe_unused]] Resolver& resolver = Resolver::get_resolver();
}

TEST(Resolve, OpenProgramImage)
{
    Resolver& resolver = Resolver::get_resolver();

    EXPECT_EQ(resolver.open("").errorcode, 0);
}

TEST(Resolve, ResolveInProgramImage)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("test_function_in_program_image");
    EXPECT_EQ(result.errorcode, 0);
    EXPECT_NE(result.function, nullptr);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInProgramImageWithParamList)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("test_function_in_program_image(const char*)");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInProgramImageWithReturnType)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("size_t test_function_in_program_image");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInProgramImageWithParamListAndReturnType)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("size_t test_function_in_program_image(const char*)");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, OpenSharedLibraryWithoutExtension)
{
    Resolver& resolver = Resolver::get_resolver();

    EXPECT_EQ(resolver.open("libtestlib").errorcode, 0);
}

TEST(Resolve, OpenSharedLibraryWithMacOSExtension)
{
    Resolver& resolver = Resolver::get_resolver();

    EXPECT_EQ(resolver.open("libtestlib.dylib").errorcode, 0);
}

TEST(Resolve, OpenSharedLibraryWithLinuxExtension)
{
    Resolver& resolver = Resolver::get_resolver();

    EXPECT_EQ(resolver.open("libtestlib.so").errorcode, 0);
}

TEST(Resolve, OpenSharedLibraryWithWindowsExtension)
{
    Resolver& resolver = Resolver::get_resolver();

    EXPECT_EQ(resolver.open("libtestlib.dll").errorcode, 0);
}

TEST(Resolve, ResolveInSharedLibrary)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("libtestlib:testlib_function");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInSharedLibraryWithParamList)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("libtestlib:testlib_function(const char*)");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInSharedLibraryWithReturnType)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("size_t libtestlib:testlib_function");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInSharedLibraryWithParamListAndReturnType)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("size_t libtestlib:testlib_function(const char*)");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInSharedLibraryWithMacOSExtension)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("libtestlib.dylib:testlib_function");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

TEST(Resolve, ResolveInSharedLibraryWithLinuxExtension)
{
    Resolver& resolver = Resolver::get_resolver();
    auto result = resolver.resolve("libtestlib.so:testlib_function");
    EXPECT_EQ(result.errorcode, 0);
    auto f = (size_t(*)(const char*))result.function;
    EXPECT_EQ(f("Hello, World!"), strlen("Hello, World!"));
}

}
