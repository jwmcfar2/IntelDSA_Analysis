#!/bin/bash
# Quick Syntax = ./runScript [runCount] [runMode]

runFile="bin/DSATest"

bufferSizes=(64 128 256 512 1024 2048 4096) #8192 16384 32768 65536 131072)
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "single" "bulk" "bulk" "bulk")
modeType=("Cold" "Hits" "Contention" "Cold" "Hits" "Contention")
fileTimeStamp=$(date +"%b%d_%H%M")

# Check if 'bin/DSATest' exists and is a file
if [[ ! -f "$runFile" ]]; then
    echo -e "\nFATAL: '$runFile' Missing - Compile source code using \"myScripts/buildScript.sh\". Exiting...\n"
    exit
fi

# Set number of runs
runCount=$1
runMode=$2
if [[ -z $runCount || $runCount -le 0 ]]; then
    echo -e "\nMissing/Invalid runCount - (Quicker syntax: \"./runScript [runCount] [runMode])\""
    read -p "Enter number of runs per test: " runCount
fi

# Choose run mode
if [[ -z $runMode || $runMode -lt 0 || $runMode -gt 5 ]]; then
    echo -e "\nMissing/Invalid runMode - (Quicker syntax: \"./runScript [runCount] [runMode]\")"
    read -p "Enter run mode number (0-5, defined in DSATest.h): " runMode
fi

for bufferSize in ${bufferSizes[@]}; do
    sizeSubstr=$bufferSize
    suffix="B" # Default suffix for bytes

    if [[ $bufferSize -ge 1024 ]]; then
        # If bufferSize is 1024 or more, convert to kilobytes and change suffix
        sizeSubstr=$(($bufferSize / 1024))
        suffix="KB"
    fi

    sizeSubstr+="$suffix"
    outputResFile="results/${bufferType[runMode]}_mov/${sizeSubstr}_${modeType[runMode]}_${fileTimeStamp}.out"
    #result="${bufferSize}${suffix}"
    #echo -e "TEST: $outputResFile"

    echo -e "\nRunning $runCount ${modeType[runMode]}/${bufferType[runMode]}-Mode Tests for ${sizeSubstr}-sized Buffer..."

    # Run actual tests $runCount times - in a SERIAL implementation
    for ((i=0; i<runCount; i++)); do
        taskset -c 7 $runFile $bufferSize $outputResFile $runMode
    done

    echo -e "\t...Done! Saved/Appended to: \"$outputResFile\""
done
