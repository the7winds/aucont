#!/bin/bash

# arguments:
# 1st - container's PID
# 2nd - host's ip
# 3rd - container's ip

pid=$1
host=auveth"$pid"_1
cont=container
ip_host=$2
ip_cont=$3
netns=/proc/"$pid"/ns/net

ip link add $host type veth peer name $cont &&

# configures container's veth
ip link set $cont netns $pid &&
nsenter -t $pid -n ip link set $cont up &&
nsenter -t $pid -n ip address add $ip_cont dev $cont &&
nsenter -t $pid -n ip link set lo up &&

# configures host's veth
ip link set $host up &&
ip address add $ip_host dev $host

