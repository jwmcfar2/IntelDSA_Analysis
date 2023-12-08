#!/bin/bash
count=$1
mode=$2
quietFlag=$3
fileTimeStamp=$(date +"%b%d_%H%M")

if [[ -z $count || -z $mode ]]; then
    echo -e "\nMissing/Invalid runCount or runMode - syntax: \"./initRun [runCount] [runMode])\""
    echo -e "DEBUG: $count $mode)"
    exit
fi

for ((i=0; i<=count; i++)); do
    myScripts/runScript.sh 1 $mode $fileTimeStamp $quietFlag
done

myScripts/getAvg.sh $count $mode $fileTimeStamp