#!/bin/sh
cd $1
for a in $(ls bin/test); do
	bin/test/$a
done;

