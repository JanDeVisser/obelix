#!/usr/bin/python

import sys
import os
import subprocess
import json

def check_stream(script, which, stream):
    ret = 0
    if which in script:
        written = [ line.strip() for line in stream ]
        written = "\n".join(written)
        expected = ("\n".join(script[which]) 
                    if isinstance(script[which], (list,tuple))
                    else script[which])
        if written != expected:
            print "%s: %s '%s' != '%s'" % (script["name"], which, written, expected)
            ret = 1
    return ret

def test_script(name):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stdout") and os.remove("stderr")
    
    f = name + ".obl"

    with open(name + ".json") as fd:
        script = json.load(fd)
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        ex = subprocess.call(["obelix", f], stdout = out, stderr = err)
        out.seek(0)
        err.seek(0)
        
        error = 0;
        if "exit" in script and ex != script["exit"]:
            print "%s: Exit code %s != %s" % (name, ex, script["exit"])
            error += 1
        error += check_stream(script, "stdout", out)
        error += check_stream(script, "stderr", err)
    os.remove("stdout")
    os.remove("stderr")

    if error > 0:
        print "%s: Failed" % name
    else:
        print name
        
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


def config_test(name):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    f = name + ".obl"

    print name
    script = { "name": name }
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        ex = subprocess.call(["obelix", f], stdout = out, stderr = err)
        out.seek(0)
        err.seek(0)
        script["exit"] = ex
        script["stdout"] = [ line.strip() for line in out ]
        script["stderr"] = [ line.strip() for line in err ]
    scripts = load_test_names()
    if name not in scripts:
        scripts.append(name)
        with open("tests.json", "w+") as fd:
            json.dump(scripts, fd)
    with open(name + ".json", "w+") as fd:
        json.dump(script, fd)
    json.dump(script, sys.stdout)
    print
    os.remove("stdout")
    os.remove("stderr")
    
def config_tests(tests):
    with open(tests) as fd:
        map(config_test, [name.strip() for name in fd if not name.startswith("#")])

if len(sys.argv) > 1:
    if sys.argv[1] == "-c":
        config_test(sys.argv[2])
    elif sys.argv[1] == "-f":
        config_tests(sys.argv[2])
    else:
        test_script(sys.argv[1])
else:
    run_all_tests()
