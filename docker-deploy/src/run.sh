#!/bin/bash
make clean
make
echo 'start running server...'
taskset -c 0,1,2,3 ./daemon &
while true ; do continue ; done
