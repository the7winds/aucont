#!/bin/bash

pid=$1
perc=$2
cores=`nproc`
period=1000000
let 'quota=period*cores*perc/100'

cgdir=/sys/fs/cgroup/cpu/$pid

mkdir $cgdir
echo $pid > $cgdir/tasks
echo $period > $cgdir/cpu.cfs_period_us
echo $quota > $cgdir/cpu.cfs_quota_us
