#!/bin/bash
# Quick Syntax = ./runScript [runCount] [runMode]

runFile="bin/DSATest"
cpuAffin="7"

bufferSizes=(64 128 256 512 1024 2048 4096) #8192 16384 32768 65536 131072) # Issue with Page Size Faults for single enQ DSA
#bufferSizes=(4096 2048 1024 512 256 128 64) #128 256 512 1024 2048 4096)
#bufferSizes=(2048 1024 128 4096 512 256 64) # Shuffle around
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "single" "bulk" "bulk" "bulk")
modeType=("Cold" "Hits" "Contention" "Cold" "Hits" "Contention")
fileTimeStamp=$(date +"%b%d_%H%M")
outFileArr=("" "" "" "" "" "" "")

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

index=0
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
    outFileArr[index]=$outputResFile

    echo -e "\nRunning $runCount ${modeType[runMode]}/${bufferType[runMode]}-Mode Tests for ${sizeSubstr}-sized Buffer..."

    # Run actual tests $runCount times - in a SERIAL implementation, split off from this script
    #lowerCPU=0
    #upperCPU=5
    #gap=40
    #mod1=70
    #mod2=110
    #./debug.sh $runCount $runFile $bufferSize $outputResFile $runMode
    for ((i=0; i<runCount; i++)); do
        #taskset -c $cpuAffin $runFile $bufferSize $outputResFile $runMode &
        #wait $!
        $runFile $bufferSize $outputResFile $runMode &
        #sleep 0.02 # sleep for 20ms -- Otherwise mutex fails ~ 1/250 runs
        wait $!
        #sleep 0.1
        #lowerCPU=$((lowerCPU + gap))
        #((lowerCPU %= mod1))
        #upperCPU=$((lowerCPU + gap))
        #((upperCPU %= mod2))
        #bin/exampleAMX & # Aggitation Script May fix this???
        #wait $!
        #sleep 0.1
        #taskset -c 6,7 $runFile $bufferSize $outputResFile $runMode  
    done

    mv $outputResFile ".tempFile.out"
    cat ".tempFile.out" | column -t > "$outputResFile"
    rm ".tempFile.out"
    echo -e "\t...Done! Saved/Appended to: \"$outputResFile\""
    (( index++ ))
done

############# Now We Get The Avg Of all these tests ####################
echo ""
for filename in ${outFileArr[@]}; do
    # Read the header and store it in an array
    read -r -a headers < <(head -n 1 "$filename")

    # Prepare an array to hold the sum of each column
    sums=()
    for ((i = 0; i < ${#headers[@]}; ++i)); do
        sums[i]=0
    done

    # Process the file and sum up each column
    while read -r -a line; do
        for ((i = 0; i < ${#line[@]}; ++i)); do
            sums[i]=$(echo "${sums[i]} + ${line[i]}" | bc)
        done
    done < <(tail -n +2 "$filename")  # Skip the header line

    # Calculate the averages for each column
    averages=()
    for i in "${!sums[@]}"; do
    if [ "$runCount" -ne 0 ]; then
        averages[i]=$(echo "scale=2; ${sums[i]}/${runCount}" | bc)
    else
        averages[i]=0
    fi
    done

    # Save the averages to a temp file
    {
        printf "%s " "${headers[@]}"
        echo
        printf "%.2f " "${averages[@]}"
        echo
    } > "${filename%.out}_unaligned_avg.out"
    cat "${filename%.out}_unaligned_avg.out" | column -t > "${filename%.out}_avg.out"
    rm ${filename%.out}_unaligned_avg.out
    echo -e "Saved Averages to: \"${filename%.out}_avg.out\""
done