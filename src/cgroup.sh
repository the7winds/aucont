#!/bin/bash

PID=$1
CG=cgrouph
PERIOD=1000000
CPU=`echo /sys/bus/cpu/devices/* | wc -w`
PC=$2
let "QUOTA = $PERIOD * $CPU * $PC / 100"

CGDIR='/sys/fs/cgroup/cpu/'$PID

mkdir $CGDIR &&
echo $PID > $CGDIR/tasks &&
echo $PERIOD > $CGDIR/cpu.cfs_period_us &&
echo $QUOTA > $CGDIR/cpu.cfs_quota_us
