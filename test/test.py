#!/usr/bin/env python3
import os, subprocess, sys, json, argparse, shutil
from pathlib import Path

def runNoRedTest(test_path, test_argv, log_name):
	global vg_iota
	pipe_out, pipe_in = os.pipe()
	if args.valgrind:
		popen_args = ["valgrind", "--leak-check=full", "--track-origins=yes", f"--log-file={valgrind_log_name}", test_path] + test_argv
	else:
		popen_args = [test_path] + test_argv
	child = subprocess.Popen(popen_args, stdout=pipe_in, stderr=pipe_in)
	tee = subprocess.Popen(["tee", log_name], stdin=pipe_out)
	child.wait()
	os.close(pipe_in)
	tee.wait()
	os.close(pipe_out)
	status = True
	if child.returncode != 0:
		print("\033[1;31mtest.py: nonzero exit code from test\033[0m", file=sys.stderr)
		status = False
	if subprocess.call(["grep", "-qF", "\033[1;31m", log_name]) == 0:
		print("\033[1;31mtest.py: error reported in test\033[0m", file=sys.stderr)
		status = False
	if args.valgrind:
		if subprocess.call(["grep", "-qF",  "== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: ", valgrind_log_name]) != 0:
			print("\033[1;31mtest.py: valgrind reported error in test\033[0m", file=sys.stderr)
			status = False
			shutil.copy(valgrind_log_name, args.cfg_dir / f"{vg_iota}.valgrind.log")
			vg_iota += 1
		os.remove(valgrind_log_name)
	os.remove(log_name)
	if status:
		print("\033[1;32mtest.py: test passed\033[0m", file=sys.stderr)
	return status

def runLogDiffTests(test_path, i, test_argv, log_name):
	global vg_iota
	pipe_out, pipe_in = os.pipe()
	if args.valgrind:
		popen_args = ["valgrind", "--leak-check=full", "--track-origins=yes", f"--log-file={valgrind_log_name}", test_path] + test_argv
	else:
		popen_args = [test_path] + test_argv
	child = subprocess.Popen(popen_args, stdout=pipe_in, stderr=pipe_in)
	tee = subprocess.Popen(["tee", log_name], stdin=pipe_out)
	child.wait()
	os.close(pipe_in)
	tee.wait()
	os.close(pipe_out)
	expected_log_name = args.cfg_dir / f"{test_name}.{i}.log"
	status = True
	if child.returncode != 0:
		print("\033[1;33mtest.py: nonzero exit code from test\033[0m", file=sys.stderr)
	if subprocess.call(["diff", "-q", expected_log_name, log_name]) != 0:
		print("\033[1;31mtest.py: log file does not match\033[0m", file=sys.stderr)
		status = False
	if args.valgrind:
		if subprocess.call(["grep", "-qF",  "== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: ", valgrind_log_name]) != 0:
			print("\033[1;31mtest.py: valgrind reported error in test\033[0m", file=sys.stderr)
			status = False
		os.remove(valgrind_log_name)
	os.remove(log_name)
	if status:
		print("\033[1;32mtest.py: test passed\033[0m", file=sys.stderr)
	return status

if __name__ == "__main__":
	arg_parser = argparse.ArgumentParser(description="Automatically run tests stored in a json file")
	#arg_parser.add_argument("cfg_dir", help="directory containing tests.json and expected output logs if used")
	#arg_parser.add_argument("bin_dir", help="directory containing test binaries")
	arg_parser.add_argument("-g", "--valgrind", action="store_true", help="run tests under valgrind")
	arg_parser.add_argument("-v", "--variant", help="what variant to test (debug, release, coverage, valgrind, etc)", required=True)
	args = arg_parser.parse_args()
	args.cfg_dir = Path(__file__).resolve().parent
	args.bin_dir = Path("build") / args.variant
	log_name = args.cfg_dir / "tmp.log"
	valgrind_log_name = args.cfg_dir / "tmp.valgrind.log"
	vg_iota = 0

	with open(args.cfg_dir / "tests.json", "r") as test_json:
		test_cfg = json.load(test_json)

	passed = 0
	tested = 0
	for test_name, test_spec in test_cfg.items():
		run_on_builds = test_spec.get("run_on_builds", None)
		if run_on_builds is not None and args.variant not in run_on_builds:
			continue
		no_red_tests = test_spec.get("no_red_tests", ())
		test_path = args.bin_dir / test_name
		for i, test_argv in enumerate(no_red_tests):
			print(f"\033[1;34mtest.py: running \"{test_name}\" no_red_tests.{i}\033[0m", file=sys.stderr)
			if runNoRedTest(test_path, test_argv, log_name):
				passed += 1
			tested += 1
		log_diff_tests = test_spec.get("log_diff_tests", ())
		for i, test_argv in enumerate(log_diff_tests):
			print(f"\033[1;34mtest.py: running \"{test_name}\" log_diff_tests.{i}\033[0m", file=sys.stderr)
			if runLogDiffTests(test_path, i, test_argv, log_name):
				passed += 1
			tested += 1

	print(f"\033[1;33mtest.py: {passed}/{tested} tests passed.\033[0m")
	if passed == tested:
		print("\033[1;32mtest.py suite passed all tests!\033[0m")
	sys.exit(passed != tested)

