/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include <core/ScopeGuard.h>
#include <core/Process.h>

namespace Obelix {

ErrorOr<int> execute(std::string const& cmd, std::vector<std::string> const& args)
{
    auto sz = args.size();
    char** argv = new char*[sz + 2];
    argv[0] = new char[cmd.length() + 1];
    strcpy(argv[0], cmd.c_str());
    for (int ix = 0; ix < sz; ++ix) {
        argv[ix + 1] = new char[args[ix].length() + 1];
        strcpy(argv[ix + 1], args[ix].c_str());
    }
    argv[sz + 1] = nullptr;
    std::cout << format("[CMD] {} {}", cmd, join(args, ' ')) << '\n';

    ScopeGuard sg([&argv, &sz]() {
        for (auto ix = 0; ix < sz; ++ix)
            delete[] argv[ix];
        delete[] argv;
    });

    int pid = fork();
    if (pid == 0) {
        execvp(cmd.c_str(), argv);
        return Error { ErrorCode::IOError, format("execvp() failed: {}", strerror(errno)) };
    } else if (pid == -1) {
        return Error { ErrorCode::IOError, format("fork() failed: {}", strerror(errno)) };
    }
    int exit_code;
    if (waitpid(pid, &exit_code, 0) == -1)
        return Error { ErrorCode::IOError, format("waitpid() failed: {}", strerror(errno)) };
    if (!WIFEXITED(exit_code))
        return Error { ErrorCode::IOError, format("Child program {} crashed due to signal {}", cmd.c_str(), WTERMSIG(exit_code)) };
    return WEXITSTATUS(exit_code);
}

}
