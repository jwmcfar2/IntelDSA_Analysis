#!/bin/bash

bufferSizes=("64B" "128B" "256B" "512B" "1KB" "2KB" "4KB") #8192 16384 32768 65536 131072) # Issue with Page Size Faults for single enQ DSA
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "single" "bulk" "bulk" "bulk")
modeType=("Cold" "Hits" "Contention" "Cold" "Hits" "Contention")
graphAxisNames=("DSAEnQ" "DSAMemMov" "Cmemcpy" "SSE1movaps" "SSE2mov" "SSE4mov" "AVX256" "AVX512-32" "AVX512-64")

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

# Normalize the data (NORMALIZED TO DSA_memmov) and output to a new file
{
  # Read the data file line by line
  while IFS= read -r line; do
    # Read fields into an array
    read -ra cols <<< "$line"
    # Use 'bc' to calculate normalized values with two decimal places
    for i in {0..9}; do
      if [[ $i -eq 3 ]]; then
        continue #printf "1.00\t" # Baseline column has a normalized value of 1
      else
        printf "%.2f\t" "$(bc -l <<< "${cols[$i]}/${cols[3]}")"
        #echo -e "DEBUG: i=$i, cols[i]=${cols[i]}, cols[3]=${cols[3]}, bc=$(bc -l <<< "${cols[$i]}/${cols[3]}")"
      fi
    done
    echo # Newline after each row of data
  done < "$dataFile" # Skip the first line if it's a header
} > "$normalizedFile"

# Reshape for gnuplot and add X-axis values:
xval=0
xinc=0.5
xgap=1
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
for i in {0..8}; do
  xtic_string+="\"${graphAxisNames[i]}\" $xLoc"
  xLoc=$(echo "$xLoc + 4.5" | bc)
  [[ $i -lt 8 ]] && xtic_string+=", " # Add comma if it's not the last benchmark
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

# Define buffer size labels as a gnuplot array-like string (used below in the plot command)
#bufferSizes=("64B" "128B" "256B" "512B" "1KB" "2KB" "4KB")
bufferSizesString = "64B 128B 256B 512B 1KB 2KB 4KB"
BufferSizeLabel(n) = word(bufferSizesString, n)

instString="DSAEnQ DSAMemMov Cmemcpy SSE1movaps SSE2mov SSE4mov AVX256 AVX512-32 AVX512-64"
instLabel(n) = word(instString, n)

# Define the space between each bar and the cluster itself

set style data histogram
set style histogram cluster gap 3
set style fill solid border -1
set boxwidth 0.495

# Key configuration
set style line 1 lt 1 lc rgb "black" lw 1
set key width 1
set key height 0.4
set key font ",11"
set key spacing 1.1
set key samplen 1
set key box linestyle 1

# Margins configuration
#set lmargin at screen 0.125
set rmargin at screen 0.95
#set tmargin at screen 0.975
#set bmargin at screen 0.2

# Axes, labels, and grid configuration
set ylabel "{/:Bold Normalized Latency}" offset 1,0
set xlabel "{/:Bold Instruction Type}" offset 0,0.5
set grid ytics

# Custom xtics
set xtics nomirror rotate by -45 font ", 9"
set xtics scale 0
$xtic_string

# Setting up the y-range and x-range
#set yrange [0.975:1.035]
set xrange [-0.9:39.9]

set style line 1 lc rgb '#E0F7FA' # color for "64B"
set style line 2 lc rgb '#B3E5FC' # color for "128B"
set style line 3 lc rgb '#4FC3F7' # color for "512B"
set style line 4 lc rgb '#039BE5' # color for "256B"
set style line 5 lc rgb '#0277BD' # color for "1KB"
set style line 6 lc rgb '#01579B' # color for "2KB"
set style line 7 lc rgb '#01329B' # color for "4KB"

# Normalized bounds with a red line at y=1
set arrow from graph 0, first 1 to graph 1, first 1 nohead lc rgb "red" lw 2

plot \
  '$graphFile' using 1:2 every ::0::0 with boxes ls 1 title "{/:Bold 64B}",\
  '$graphFile' using 1:2 every ::1::1 with boxes ls 2 title "{/:Bold 128B}",\
  '$graphFile' using 1:2 every ::2::2 with boxes ls 3 title "{/:Bold 256B}",\
  '$graphFile' using 1:2 every ::3::3 with boxes ls 4 title "{/:Bold 512B}",\
  '$graphFile' using 1:2 every ::4::4 with boxes ls 5 title "{/:Bold 1KB}",\
  '$graphFile' using 1:2 every ::5::5 with boxes ls 6 title "{/:Bold 2KB}",\
  '$graphFile' using 1:2 every ::6::6 with boxes ls 7 title "{/:Bold 4KB}"

EOF

# Comment out to allow for deletion of temp files
exit

rm results/.temp*

#rm $dataFile
#rm $normalizedFile
#rm $graphFile