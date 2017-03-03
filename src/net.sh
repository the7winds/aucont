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

sudo ip link add $HOST type veth peer name $CONT && 

# configures container's veth
sudo ip link set $CONT netns $PID &&
sudo nsenter -t $PID -n ip link set $CONT up &&
sudo nsenter -t $PID -n ip address add $IP_CONT dev $CONT &&
sudo nsenter -t $PID -n ip link set lo up &&

# configures host's veth
sudo ip link set $HOST up &&
sudo ip address add $IP_HOST dev $HOST

