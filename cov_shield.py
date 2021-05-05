#!/usr/bin/env python3
import sys
from math import floor, ceil

if len(sys.argv) == 1:
	print("https://img.shields.io/badge/coverage-n%2Fa-inactive")
else:
	perc_cov = float(sys.argv[1])
	print("https://img.shields.io/badge/coverage-{}%25-{:x}{:x}00".format(perc_cov, floor((1 - perc_cov/100)*255), ceil(perc_cov/100*255)))
	