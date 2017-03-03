#!/bin/bash

gcc -Isrc src/aucont_util.c src/aucont_start.c -o bin/aucont_start
gcc -Isrc src/aucont_util.c src/aucont_stop.c -o bin/aucont_stop
gcc -Isrc src/aucont_util.c src/aucont_exec.c -o bin/aucont_exec

cp src/*.py bin/
cp src/*.sh bin/
chmod +x bin/*.py bin/*.sh

ln -f -s aucont_list.py bin/aucont_list

exe="./bin/aucont_start ./bin/aucont_exec ./bin/aucont_stop"
sudo chown 0:0 $exe
sudo chmod u+s $exe
