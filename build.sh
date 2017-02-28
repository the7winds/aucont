#!/bin/bash

gcc src/aucont_start.c -o bin/aucont_start
cp src/*.py bin/
cp src/*.sh bin/
chmod +x bin/*.py bin/*.sh
