#!/usr/bin/env python3
import os, subprocess, sys, json

if len(sys.argv) != 3:
	print("Use like\n{} <path to directory containing tests.json> <path to directory containing test binaries>".format(sys.argv[0]))
	sys.exit(1)

with open(os.path.join(sys.argv[1], "tests.json"), "r") as test_json:
	test_cfg = json.load(test_json)

passed = 0
tested = 0
for test_name, test_spec in test_cfg.items():
	no_red_tests = test_spec.get("no_red_tests", ())
	test_path = os.path.join(sys.argv[2], test_name)
	log_name = os.path.join(sys.argv[1], "tmp.log")
	for test_argv in no_red_tests:
		pipe_out, pipe_in = os.pipe()
		child = subprocess.Popen([test_path] + test_argv, stdout=pipe_in, stderr=pipe_in)
		tee = subprocess.Popen(["tee", log_name], stdin=pipe_out)
		child.wait()
		os.close(pipe_in)
		tee.wait()
		os.close(pipe_out)
		status = subprocess.call(["grep", "-qF", "\033[1;31m", log_name])
		if status == 1:
			passed += 1
		os.remove(log_name)
		tested += 1
	for i, test_argv in enumerate(test_spec.get("log_diff_tests", ())):
		pipe_out, pipe_in = os.pipe()
		child = subprocess.Popen([test_path] + test_argv, stdout=pipe_in, stderr=pipe_in)
		tee = subprocess.Popen(["tee", log_name], stdin=pipe_out)
		child.wait()
		os.close(pipe_in)
		tee.wait()
		os.close(pipe_out)
		expected_log_name = os.path.join(sys.argv[1], "{}.{}.{}".format(test_name, str(i), "log"))
		status = subprocess.call(["diff", "-q", expected_log_name, log_name])
		if status == 0:
			passed += 1
		os.remove(log_name)
		tested += 1

print(str(passed) + "/" + str(tested) + " tests passed.")
if passed == tested:
	print("\033[1;32mtest.py suite passed all tests!\033[0m")
sys.exit(passed != tested)

