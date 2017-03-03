#!/bin/bash

# arguments:
# 1st - container's PID
# 2nd - host's ip
# 3rd - container's ip

PID=$1
HOST=auveth"$PID"_1
CONT=container
IP_HOST=$2
IP_CONT=$3
NETNS=/proc/"$PID"/ns/net

ip link add $HOST type veth peer name $CONT &&

# configures container's veth
ip link set $CONT netns $PID &&
nsenter -t $PID -n ip link set $CONT up &&
nsenter -t $PID -n ip address add $IP_CONT dev $CONT &&
nsenter -t $PID -n ip link set lo up &&

# configures host's veth
ip link set $HOST up &&
ip address add $IP_HOST dev $HOST

