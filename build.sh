#!/bin/bash

gcc src/aucont_start.c -o bin/aucont_start
gcc src/aucont_stop.c -o bin/aucont_stop
cp src/*.py bin/
cp src/*.sh bin/
chmod +x bin/*.py bin/*.sh

ln -f -s aucont_list.py bin/aucont_list
