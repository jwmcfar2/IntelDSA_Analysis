#!/bin/bash
# Syntax ./getAvg.sh [runCount] [mode] [timestamp] {'-q' = quiet prints}
runCount=$1
mode=$2
timeStamp=$3

quietFlag=0;
if [[ "$4" == "-q" ]]; then
    quietFlag=1
fi

testTypes=("memmv" "flush" "cmp")
bufferStrs=("64B" "128B" "256B" "512B" "1KB" "2KB" "4KB")
bufferType=("single" "single" "bulk" "bulk" "bulk" "bulk")
modeType=("Cold" "Cold" "Bulk" "Bulk" "Contention" "Contention")
outFileArr=("" "" "" "" "" "" "")

for testTypeIndx in "${!testTypes[@]}"; do
    for ((i=0; i<${#bufferStrs[@]}; i++)); do
        outFileArr[i]="results/${testTypes[testTypeIndx]}/${bufferType[mode]}/${bufferStrs[i]}_${modeType[mode]}_${timeStamp}.out"
    done

    if [[ $quietFlag -eq 0 ]]; then
        echo ""
    fi
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
        if [[ $quietFlag -eq 0 ]]; then
            echo -e "Saved Averages to: \"${filename%.out}_avg.out\""
        fi
    done
done

if [[ $quietFlag -eq 0 ]]; then
    echo -e "\n\t## Reminder: You can pass '-q' to end of script to quiet most script print-statements ##\n"
else
    echo -e "\tFinished Saving Averages {(FilePath)_avg.out}\n"
fi