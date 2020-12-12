#!/bin/bash

exceptList="sshd|bash|ps|grep|wc|/bin/sh|/bin/bash"
userName="netpa11"

procNo=$(ps aux | grep $userName | grep -Ev "$exceptList" | wc -l)
procLimit=20

echo "==============================================="
date
echo "Running process number is ${procNo}"
echo "(except : $exceptList)"
echo "==============================================="
ps aux | grep $userName | grep -Ev "$exceptList"

if [ $procNo -gt $procLimit ]; then
    kill $(ps aux | grep $userName | grep -Ev "$exceptList" | awk '{print $2}')
    echo ">> Process number is over $procLimit. Kill All Procs."
else
    echo ">> Process number is below $procLimit."
fi

echo ""
