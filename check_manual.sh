#!/bin/bash

exceptList="sshd|bash|ps|grep|wc|/bin/sh|/bin/bash"
userName="netpa11"

procNo=$(ps aux | grep $userName | grep -Ev "$exceptList" | wc -l)

if [ $# -eq 1 ]; then
    procLimit=$1
else
    procLimit=10
fi

echo "==============================================="
date
echo "except Name : $exceptList"
echo "==============================================="

while [ 1 ]
do
    procNo=$(ps aux | grep $userName | grep -Ev "$exceptList" | wc -l)
    if [ $procNo -gt $procLimit ]; then
        kill $(ps aux | grep $userName | grep -Ev "$exceptList" | awk '{print $2}')
        echo ">> Process number is over $procLimit. Kill All Procs."
    else
        echo ">> Process number is $procNo below $procLimit."
    fi
    sleep 1
done
echo ""
