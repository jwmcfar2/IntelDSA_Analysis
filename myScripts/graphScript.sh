#!/bin/bash
# ./graphScript.sh [runMode]

bufferSizes=("64B" "128B" "256B" "512B" "1KB" "2KB" "4KB") #8192 16384 32768 65536 131072) # Issue with Page Size Faults for single enQ DSA
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "single" "bulk" "bulk" "bulk")
modeType=("Cold" "Hits" "Contention" "Cold" "Hits" "Contention")
graphAxisNames=("DSA EnQ" "DSA memmov" "C memcpy" "SSE1 movaps" "SSE2 mov" "SSE4 mov" "AVX 256" "AVX 512-32" "AVX 512-64" "AMX LdSt")
normalVal=0
largestAMX=0

runMode=$1
# Choose run mode
if [[ -z $runMode || $runMode -lt 0 || $runMode -gt 5 ]]; then
    echo -e "\nMissing/Invalid runMode - (Quicker syntax: \"./graphScript [runMode]\")"
    read -p "Enter run mode number (0-5, defined in DSATest.h): " runMode
fi

# Make temp files to parse info into:
dataFile="results/.temp"
normalizedFile="results/.temp3"
graphFile="results/.temp4"
> $dataFile
> $graphFile
> $normalizedFile

#################################
########### Parse ###############

# Parse mode results into temp files
for bufferIndex in ${!bufferSizes[@]}; do
    mostRecentFile=$(find results/${bufferType[runMode]}_mov -type f -name "${bufferSizes[bufferIndex]}_${modeType[runMode]}_*_avg.out"\
        -printf '%T@ %p\n' | sort -n | tail -n1 | cut -d' ' -f2-)
    awk 'NR == 2' "$mostRecentFile" >> "$dataFile"
done

# Normalize the data (NORMALIZED TO ASM_movq) and output to a new file
{
  # Read the data file line by line
  while IFS= read -r line; do
    # Read fields into an array
    read -ra cols <<< "$line"
    # Use 'bc' to calculate normalized values with two decimal places
    for i in {0..10}; do
      if [[ $i -eq 3 ]]; then
        continue #printf "1.00\t" # Baseline column has a normalized value of 1
      else
        normalVal=$(bc -l <<< "${cols[$i]}/${cols[3]}")
        if [[ $i -eq 10 && $(bc -l <<< "$normalVal > $largestAMX") -eq 1 ]]; then
          largestAMX=$normalVal
        fi 
        printf "%.2f\t" "$normalVal"
      fi
    done
    echo # Newline after each row of data
  done < "$dataFile" # Skip the first line if it's a header
} > "$normalizedFile"
# Round AMX Max to the nearest 1 decimal place
echo "$largestAMX"
largestAMX_rounded=$(printf "%.1f" "$largestAMX")
echo "$largestAMX_rounded"

# Reshape for gnuplot and add X-axis values:
xval=0
xinc=0.5
xgap=0.5
num_columns=$(head -1 "$normalizedFile" | wc -w)
for ((col=1; col<=num_columns; col++)); do
    while read -a line; do
      echo "$xval ${line[$((col-1))]}" >> "$graphFile"
      xval=$(echo "$xval + $xinc" | bc)
    done < <(cut -d ' ' -f $col "$normalizedFile")
    xval=$(echo "$xval + $xgap" | bc)
    echo "" >> "$graphFile"
done

#################################
########### Graph ###############
# Initialize the xtics string
xLoc=1.2
xtic_string="set xtics ("
for i in {0..9}; do
  xtic_string+="\"{/=10.5:Bold ${graphAxisNames[i]}}\" $xLoc"
  xLoc=$(echo "$xLoc + 4" | bc)
  [[ $i -lt 9 ]] && xtic_string+=", " # Add comma if it's not the last benchmark
done
xtic_string+=")"

# Test String
#echo -e "String = \"$xtic_string\"\n"
#exit

# Define the output PNG file
outputFile="results/Normalized_Execution_Time.png"

gnuplot <<- EOF
set terminal png
set output "$outputFile"

set style data histogram
set style histogram cluster gap 3
set style fill solid border -1
set boxwidth 0.495

# Key configuration
set style line 1 lt 1 lc rgb "black" lw 1
set key width 1
set key height 0.4
set key font ",11"
set key at screen 0.325,0.85
set key spacing 1.1
set key samplen 1
set key box linestyle 1
set label "{/=8:Bold Buffer Sizes}" at screen 0.21, 0.58

# Margins configuration
set lmargin at screen 0.1255
set rmargin at screen 0.935
#set tmargin at screen 0.975
#set bmargin at screen 0.2

# Axes, labels, and grid configuration
set title "{/=15:Bold Avg Latency for MemMov Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}"
set termoption enhanced
set ylabel "{/:Bold Normalized Latency}\n{/=10(Baseline=x86'movq')}" offset 1,0
set xlabel "{/=14:Bold Instruction Type}\n{/=10(N=1,000)}" offset 0,-0.35
set ytics nomirror

# Custom xtics
set xtics nomirror rotate by -45 font ", 9"
set xtics scale 0
#set ytics scale 0
$xtic_string

# Setting up the y-range and x-range
set yrange [0:7]
set xrange [-0.9:39.85]

set style line 1 lc rgb '#E0F7FA' # color for "64B"
set style line 2 lc rgb '#B3E5FC' # color for "128B"
set style line 3 lc rgb '#4FC3F7' # color for "512B"
set style line 4 lc rgb '#039BE5' # color for "256B"
set style line 5 lc rgb '#0277BD' # color for "1KB"
set style line 6 lc rgb '#01579B' # color for "2KB"
set style line 7 lc rgb '#01329B' # color for "4KB"

# Normalized bounds with a red line at y=1
set arrow from graph 0, first 1 to graph 1, first 1 nohead lc rgb "red" lw 2
set arrow from screen 0.83, screen 0.75 to screen 0.84, screen 0.825 lc rgb "red" lw 2
set label "{/=8:Bold Up to ${largestAMX_rounded}x Baseline}" textcolor rgb "red" at screen 0.65, 0.725

plot \
  '$graphFile' using 1:2 every ::0::0 with boxes ls 1 title "{/:Bold 64B}",\
  '$graphFile' using 1:2 every ::1::1 with boxes ls 2 title "{/:Bold 128B}",\
  '$graphFile' using 1:2 every ::2::2 with boxes ls 3 title "{/:Bold 256B}",\
  '$graphFile' using 1:2 every ::3::3 with boxes ls 4 title "{/:Bold 512B}",\
  '$graphFile' using 1:2 every ::4::4 with boxes ls 5 title "{/:Bold 1KB}",\
  '$graphFile' using 1:2 every ::5::5 with boxes ls 6 title "{/:Bold 2KB}",\
  '$graphFile' using 1:2 every ::6::6 with boxes ls 7 title "{/:Bold 4KB}"

EOF

echo -e "\nDone! Graph saved to: $outputFile\n"

# Comment out to allow for deletion of temp files
#exit
rm results/.temp*