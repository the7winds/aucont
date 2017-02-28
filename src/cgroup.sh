#!/bin/bash

PID=$1
CG=cgrouph
PERIOD=1000000
CPU=`echo /sys/bus/cpu/devices/* | wc -w`
PC=$2
let "QUOTA = $PERIOD * $CPU * $PC / 100"

mount -t cgroup -o cpu,cpuacct $PID $PID/$CG &&
mkdir $PID/$CG/$PID &&
echo $PID > $PID/$CG/$PID/tasks &&
echo $PERIOD > $PID/$CG/$PID/cpu.cfs_period_us &&
echo $QUOTA > $PID/$CG/$PID/cpu.cfs_quota_us
