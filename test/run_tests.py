#!/opt/homebrew/bin/python3
#  Copyright (c) 2021-2022, Jan de Visser <jan@finiandarcy.com>
#
#  SPDX-License-Identifier: GPL-3.0-or-later

import argparse
import json
import os
import shutil
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


def compile_script(name):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    if name.endswith(".obl"):
        f = name
        name = name[:-4]
    else:
        f = name + ".obl"

    print(name)
    script = {"name": name}
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        if os.path.exists(name):
            os.remove(name)
        if os.path.exists(os.path.join(".compiled", name)):
            os.remove(os.path.join(".compiled", name))
        ex = subprocess.call(["../build/bin/obelix", "--keep-assembly", f], stdout=out, stderr=err)
        if ex != 0:
            print(f"Compilation of '{f}' failed: {ex}")
            subprocess.call(["cat", "stdout"])
            subprocess.call(["cat", "stderr"])
            return None
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    os.rename(name, os.path.join(".compiled", name))
    return name


def test_script(name):
    if name.endswith(".obl"):
        name = name[:-4]

    with open(name + ".json") as fd:
        script = json.load(fd)
    name = compile_script(name)
    if name is None:
        print(f"{name}: Compilation Failed")
        return False

    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        cmdline = [os.path.join(".compiled", name)]
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


def config_test(name, *args):
    name = compile_script(name)
    if name is None:
        sys.exit(1)
    script = {"name": name}

    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        cmdline = [os.path.join(".compiled", name)]
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
            json.dump(scripts, fd, indent=2)
            print(file=fd);
    with open(name + ".json", "w+") as fd:
        json.dump(script, fd, indent=2)
        print(file=fd)
    os.remove("stdout")
    os.remove("stderr")


def remove_test(name, destroy=False):
    if destroy:
        fname = name + ".json"
        os.path.exists(fname) and os.remove(fname)
    scripts = load_test_names()
    if name in scripts:
        scripts.remove(name)
        with open("tests.json", "w+") as fd:
            json.dump(scripts, fd, indent=2)


def remove_all_tests(nuke=False):
    scripts = load_test_names()
    if nuke:
        for script in scripts:
            fname = script + ".json"
            os.path.exists(fname) and os.remove(fname)
    if os.path.exists("tests.json"):
        os.remove("tests.json")


def print_index():
    scripts = load_test_names()
    for script in scripts:
        s = [script]
        with open(script + ".json") as fd:
            script_data = json.load(fd)
            if "args" in script_data:
                s.extend(script_data["args"])
        print(" ".join(s))


def config_tests(tests):
    with open(tests) as fd:
        # FIXME Why does this map() not work?
        # map(config_test, [stripped for stripped in [name.strip() for name in fd] if not stripped.startswith("#")])
        for tests in [stripped for stripped in [test.strip() for test in fd] if not stripped.startswith("#")]:
            name_args = filter(None, tests.split(" "))
            config_test(*name_args)


def initialize():
    shutil.rmtree(".compiled", True)
    os.mkdir(".compiled")


initialize()
arg_parser = argparse.ArgumentParser()
group = arg_parser.add_mutually_exclusive_group(required=True)
group.add_argument(
    "-i", "--index", action='store_true',
    help="Displays an index of all registered tests. Output can be used by -f/--add-all")
group.add_argument("-a", "--execute-all", action='store_true', help="Execute all tests in the registry")
group.add_argument("-x", "--execute", nargs="+", metavar="Test", help="Execute the specified tests")
group.add_argument(
    "-c", "--create", nargs="+", metavar=("Test", "Argument"),
    help="Add the specified test script with optional arguments to the test registry, and executes them")
group.add_argument(
    "-f", "--add-all", metavar="File",
    help="Add all tests in <File>. <File> should contain test script names, one per line")
group.add_argument(
    "-d", "--delete", nargs="+", metavar="Test",
    help="Remove the specified tests from the registry. The expected outcome .json files will be retained")
group.add_argument(
    "--destroy", nargs="+", metavar="Test",
    help="Remove the specified tests from the registry. The expected outcome .json files will be deleted as well")
group.add_argument(
    "--clear", action='store_true',
    help="Clear the test registry. The expected outcome .json files will be retained")
group.add_argument(
    "--nuke", action='store_true',
    help="Clear the test registry. The expected outcome .json files will be deleted as well")
args = arg_parser.parse_args()

if args.execute_all:
    run_all_tests()
if args.execute:
    for name in args.execute:
        test_script(name)
if args.create:
    config_test(args.create[0], *args.create[1:])
if args.add_all:
    config_tests(args.add_all)
if args.delete:
    for name in args.delete:
        remove_test(name)
if args.destroy:
    for name in args.destroy:
        remove_test(name, True)
if args.clear or args.nuke:
    remove_all_tests(args.nuke)
if args.index:
    print_index()
