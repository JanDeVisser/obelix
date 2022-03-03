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
    }
    int exit_code;
    waitpid(pid, &exit_code, 0);
    return exit_code;
}

}
