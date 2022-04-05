/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <core/ScopeGuard.h>
#include <core/Process.h>

namespace Obelix {

ErrorOr<int> execute(std::string const& cmd, std::vector<std::string> const& args)
{
    Process p(cmd, args);
    return p.execute();
}

Process::Process(std::string command, std::vector<std::string> arguments)
    : m_command(move(command))
    , m_arguments(move(arguments))
{
}

ErrorOr<int> Process::execute()
{
    auto sz = m_arguments.size();
    char** argv = new char*[sz + 2];
    argv[0] = new char[m_command.length() + 1];
    strcpy(argv[0], m_command.c_str());
    for (int ix = 0; ix < sz; ++ix) {
        argv[ix + 1] = new char[m_arguments[ix].length() + 1];
        strcpy(argv[ix + 1], m_arguments[ix].c_str());
    }
    argv[sz + 1] = nullptr;
    std::cout << format("[CMD] {} {}", m_command, join(m_arguments, ' ')) << '\n';

    ScopeGuard sg([&argv, &sz]() {
        for (auto ix = 0; ix < sz; ++ix)
            delete[] argv[ix];
        delete[] argv;
    });

    int filedes[2];
    if (pipe(filedes) == -1) {
        return Error<int> { ErrorCode::IOError, errno, format("pipe() failed: {}", strerror(errno)) };
    }

    pid_t pid = fork();
    if (pid == -1)
        return Error<int> { ErrorCode::IOError, errno, format("fork() failed: {}", strerror(errno)) };
    if (pid == 0) {
        while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) { }
        close(filedes[0]);
        close(filedes[1]);
        execvp(m_command.c_str(), argv);
        return Error<int> { ErrorCode::IOError, errno, format("execvp() failed: {}", strerror(errno)) };
    }
    close(filedes[1]);

    char buffer[4096];
    while (true) {
        auto count = read(filedes[0], buffer, sizeof(buffer) - 1);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                return Error<int> { ErrorCode::IOError, errno, format("Error reading child process output: {}", strerror(errno)) };
            }
        }
        if (count == 0)
            break;
        buffer[count] = '\0';
        m_stdout += buffer;
    }
    close(filedes[0]);

    int exit_code;
    if (waitpid(pid, &exit_code, 0) == -1)
        return Error<int> { ErrorCode::IOError, errno, format("waitpid() failed: {}", strerror(errno)) };
    if (!WIFEXITED(exit_code))
        return Error<int> { ErrorCode::IOError, errno, format("Child program {} crashed due to signal {}", m_command.c_str(), WTERMSIG(exit_code)) };
    return WEXITSTATUS(exit_code);
}

}
