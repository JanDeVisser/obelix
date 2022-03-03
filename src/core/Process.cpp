/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/wait.h>
#include <unistd.h>

#include <core/Process.h>

namespace Obelix {

ErrorOr<int> execute(std::string const& cmd, std::vector<std::string> const& args)
{
    char** argv = new char*[args.size()];
    for (int ix = 0; ix < args.size(); ix++) {
        argv[ix] = const_cast<char*>(args[ix].c_str());
    }
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
