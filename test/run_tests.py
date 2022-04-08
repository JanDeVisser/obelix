#!/opt/homebrew/bin/python3
#  Copyright (c) 2021-2022, Jan de Visser <jan@finiandarcy.com>
#
#  SPDX-License-Identifier: GPL-3.0-or-later

import json
import os
import subprocess
import sys


def check_stream(script, which, stream):
    ret = 0
    if which in script:
        written = [line.strip() for line in stream]
        written = "\n".join(written)
        expected = ("\n".join(script[which])
                    if isinstance(script[which], (list, tuple))
                    else script[which])
        if written != expected:
            print("%s: %s '%s' != '%s'" % (script["name"], which, written, expected))
            ret = 1
    return ret


def test_script(name):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stdout") and os.remove("stderr")

    f = name + ".obl"

    with open(name + ".json") as fd:
        script = json.load(fd)
    with open("/dev/null", "w+") as out, open("/dev/null", "w+") as err:
        ex = subprocess.call(["../build/bin/obelix", f], stdout=out, stderr=err)
        if ex != 0:
            print(f"{name}: Compilation Failed")
            return False
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        cmdline = [f"./{name}"]
        cmdline.extend(script["args"])
        ex = subprocess.call(cmdline, stdout=out, stderr=err)
        out.seek(0)
        err.seek(0)

        error = 0
        if "exit" in script:
            expected = script["exit"]
            if ex != expected and ex != expected + 256 and ex != expected - 256:
                print("%s: Exit code %s != %s" % (name, ex, script["exit"]))
                error += 1
        error += check_stream(script, "stdout", out)
        error += check_stream(script, "stderr", err)
    os.remove("stdout")
    os.remove("stderr")
    print("%s: %s" % (name, "OK" if error == 0 else "Failed"))

    return error == 0


def load_test_names():
    if os.path.exists("tests.json"):
        with open("tests.json") as fd:
            scripts = json.load(fd)
    else:
        scripts = []
    return scripts


def run_all_tests():
    scripts = load_test_names()
    for script in scripts:
        if not test_script(script):
            break


def config_test(name, args):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    f = name + ".obl"

    print(name)
    script = {"name": name}
    with open("/dev/null", "w+") as out, open("/dev/null", "w+") as err:
        ex = subprocess.call(["../build/bin/obelix", f], stdout=out, stderr=err)
        if ex != 0:
            print(f"Compilation of '{f}' failed")
            sys.exit(1)
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        cmdline = [f"./{name}"]
        cmdline.extend(args)
        ex = subprocess.call(cmdline, stdout=out, stderr=err)
        out.seek(0)
        err.seek(0)
        script["exit"] = ex
        script["stdout"] = [line.strip() for line in out]
        script["stderr"] = [line.strip() for line in err]
        script["args"] = args
    scripts = load_test_names()
    if name not in scripts:
        scripts.append(name)
        with open("tests.json", "w+") as fd:
            json.dump(scripts, fd)
    with open(name + ".json", "w+") as fd:
        json.dump(script, fd)
    json.dump(script, sys.stdout)
    print()
    os.remove("stdout")
    os.remove("stderr")


def remove_test(name):
    fname = name + ".json"
    os.path.exists(fname) and os.remove(fname)
    scripts = load_test_names()
    if name in scripts:
        scripts.remove(name)
        with open("tests.json", "w+") as fd:
            json.dump(scripts, fd)


def remove_all_tests():
    scripts = load_test_names()
    for script in scripts:
        fname = script + ".json"
        os.path.exists(fname) and os.remove(fname)
    os.remove("tests.json")


def print_index():
    scripts = load_test_names()
    for script in scripts:
        print(script)


def config_tests(tests):
    with open(tests) as fd:
        map(config_test, [name.strip() for name in fd if not name.startswith("#")])


def usage():
    print("run_tests - Execute and manage obelix tests.")
    print("")
    print("Usage: python run_tests.py [option]")
    print("Options:")
    print(" -c <test>:   Add test script <test>.obl to test suite.")
    print("              Execute script, capture and register exit code and stdout/stderr.")
    print(" -f <file>:   Add all tests in <file>. <file> contains test script names,")
    print("              one per line, without .obl extension.")
    print(" -x <test>:   Run test <test>.")
    print("              Compare exit code and stderr/out with registered values.")
    print(" -a, --all:   Run all registered tests.")
    print(" -d <test>:   Remove test <test> from registry.")
    print(" --clear:     Clear test registry.")
    print(" -i, --index: Print an index of all registered tests. Output can be used by -f.")
    print(" -h, --help:  Display this message.")
    print("")


if len(sys.argv) > 1:
    if sys.argv[1] == "-c":
        config_test(sys.argv[2], sys.argv[3:])
    elif sys.argv[1] == "-d":
        remove_test(sys.argv[2])
    elif sys.argv[1] == "--clear":
        remove_all_tests()
    elif sys.argv[1] == "-f":
        config_tests(sys.argv[2])
    elif sys.argv[1] in ["-a", "--all"]:
        run_all_tests()
    elif sys.argv[1] == "-x":
        test_script(sys.argv[2])
    elif sys.argv[1] in ["-h", "--help"]:
        usage()
    elif sys.argv[1] in ["-i", "--index"]:
        print_index()
    else:
        print("Unrecognized option " + sys.argv[1])
        print("")
        usage()
else:
    usage()
