#!/bin/bash

pid=$1
perc=$2
cores=`nproc`
period=1000000
let 'quota=period*cores*perc/100'

cgdir=/sys/fs/cgroup/cpu/$pid

sudo mkdir $cgdir > /dev/null
echo $pid | sudo tee $cgdir/tasks > /dev/null
echo $period | sudo tee $cgdir/cpu.cfs_period_us > /dev/null
echo $quota | sudo tee $cgdir/cpu.cfs_quota_us > /dev/null
