#!/bin/bash
# syntax: ./initRun [numTests] [mode] {-q == quiet script prints}

count=$1
mode=$2
quietFlag=$3
fileTimeStamp=$(date +"%b%d_%H%M")

if [[ -z $count || -z $mode ]]; then
    echo -e "\nMissing/Invalid runCount or runMode - syntax: \"./initRun [runCount] [runMode])\""
    echo -e "DEBUG: $count $mode)"
    exit
fi

for ((i=0; i<count; i++)); do
    myScripts/runScript.sh 1 $mode $fileTimeStamp $quietFlag &
    wait $!
    sleep 0.0000000001 # Script seems to lock up without this for larger runCounts -- dunno why
done

myScripts/getAvg.sh $count $mode $fileTimeStamp $quietFlag