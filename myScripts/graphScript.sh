#!/bin/bash
# ./graphScript.sh [runMode]
#############################
# I'm sorry in advanced to anyone unfortunate enough to see how clunky this is... gnuplot broke me.
# Fair warning, y ranges need to be tweaked manually for every graph.
###################################################################################################

testTypes=("memmv" "flush" "cmp")
bufferSizes=("64B" "128B" "256B" "512B" "1KB" "2KB" "4KB")
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "bulk" "bulk" "bulk" "bulk")
modeType=("Cold" "Cold" "Bulk" "Bulk" "Contention" "Contention")
axisNamesMemMv=("DSA EnQ" "DSA memmov" "C memcpy" "SSE1 movaps" "SSE2 mov" "SSE4 mov" "AVX 256" "AVX 512-32" "AVX 512-64" "AMX LdSt")
axisNamesFlush=("DSA EnQ" "DSA Flush" "clflushopt")
axisNamesCmp=("DSA EnQ" "DSA Compare" "SSE2" "SSE4" "AVX-256" "AVX-512")
normalVal=0
largestAMX=0

#################################
######## Script Setup ###########

runMode=$1
# Choose run mode
if [[ -z $runMode || $runMode -lt 0 || $runMode -gt 5 ]]; then
    echo -e "\nMissing/Invalid runMode - (Quicker syntax: \"./graphScript [runMode]\")"
    read -p "Enter run mode number (0-5, defined in DSATest.h): " runMode
fi

# Make temp files to parse info into:
dataFileMemMv="results/.temp"
graphFileMemMv="results/.temp2"
normalizedMemMv="results/.temp3"
dataFileFlush="results/.temp4"
graphFileFlush="results/.temp5"
normalizedFlush="results/.temp6"
dataFileCmp="results/.temp7"
graphFileCmp="results/.temp8"
normalizedCmp="results/.temp9"
> $dataFileMemMv
> $graphFileMemMv
> $normalizedMemMv
> $dataFileFlush
> $graphFileFlush
> $normalizedFlush
> $dataFileCmp
> $graphFileCmp
> $normalizedCmp

#################################
########### Parse ###############

# Parse mode results into temp files
for bufferIndex in ${!bufferSizes[@]}; do
    mostRecentFile=$(find results/memmv/${bufferType[runMode]} -type f -name "${bufferSizes[bufferIndex]}_${modeType[runMode]}_*_avg.out"\
        -printf '%T@ %p\n' | sort -n | tail -n1 | cut -d' ' -f2-)
    awk 'NR == 2' "$mostRecentFile" >> "$dataFileMemMv"
done
for bufferIndex in ${!bufferSizes[@]}; do
    mostRecentFile=$(find results/flush/${bufferType[runMode]} -type f -name "${bufferSizes[bufferIndex]}_${modeType[runMode]}_*_avg.out"\
        -printf '%T@ %p\n' | sort -n | tail -n1 | cut -d' ' -f2-)
    awk 'NR == 2' "$mostRecentFile" >> "$dataFileFlush"
done
for bufferIndex in ${!bufferSizes[@]}; do
    mostRecentFile=$(find results/cmp/${bufferType[runMode]} -type f -name "${bufferSizes[bufferIndex]}_${modeType[runMode]}_*_avg.out"\
        -printf '%T@ %p\n' | sort -n | tail -n1 | cut -d' ' -f2-)
    awk 'NR == 2' "$mostRecentFile" >> "$dataFileCmp"
done


# Normalize the data (NORMALIZED TO ASM_movq) and output to a new file
{
  # Read the data file line by line
  while IFS= read -r line; do
    # Read fields into an array
    read -ra cols <<< "$line"
    # Use 'bc' to calculate normalized values with two decimal places
    for i in {0..10}; do
      if [[ $i -eq 2 ]]; then
        continue #printf "1.00\t" # Baseline column has a normalized value of 1
      else
        normalVal=$(bc -l <<< "${cols[$i]}/${cols[2]}")
        if [[ $i -eq 10 && $(bc -l <<< "$normalVal > $largestAMX") -eq 1 ]]; then
          largestAMX=$normalVal
        fi 
        printf "%.2f\t" "$normalVal"
      fi
    done
    echo # Newline after each row of data
  done < "$dataFileMemMv" # Skip the first line if it's a header
} > "$normalizedMemMv"
# Normalize the data (NORMALIZED TO clflush) and output to a new file
{
  # Read the data file line by line
  while IFS= read -r line; do
    # Read fields into an array
    read -ra cols <<< "$line"
    # Use 'bc' to calculate normalized values with two decimal places
    for i in {0..3}; do
      if [[ $i -eq 2 ]]; then
        continue #printf "1.00\t" # Baseline column has a normalized value of 1
      else
        normalVal=$(bc -l <<< "${cols[$i]}/${cols[2]}")
        printf "%.2f\t" "$normalVal"
      fi
    done
    echo # Newline after each row of data
  done < "$dataFileFlush" # Skip the first line if it's a header
} > "$normalizedFlush"
# Normalize the data (NORMALIZED TO c_memcmp) and output to a new file
{
  # Read the data file line by line
  while IFS= read -r line; do
    # Read fields into an array
    read -ra cols <<< "$line"
    # Use 'bc' to calculate normalized values with two decimal places
    for i in {0..6}; do
      if [[ $i -eq 2 ]]; then
        continue #printf "1.00\t" # Baseline column has a normalized value of 1
      else
        normalVal=$(bc -l <<< "${cols[$i]}/${cols[2]}")
        printf "%.2f\t" "$normalVal"
      fi
    done
    echo # Newline after each row of data
  done < "$dataFileCmp" # Skip the first line if it's a header
} > "$normalizedCmp"

#################################
# Round AMX Max to the nearest 1 decimal place
largestAMX_rounded=$(printf "%.1f" "$largestAMX")
#################################

# Reshape for gnuplot and add X-axis values:
xval=0
xinc=0.5
xgap=0.5
num_columns=$(head -1 "$normalizedMemMv" | wc -w)
for ((col=1; col<=num_columns; col++)); do
    while read -a line; do
      echo "$xval ${line[$((col-1))]}" >> "$graphFileMemMv"
      xval=$(echo "$xval + $xinc" | bc)
    done < <(cut -d ' ' -f $col "$normalizedMemMv")
    xval=$(echo "$xval + $xgap" | bc)
    echo "" >> "$graphFileMemMv"
done
# Reshape for gnuplot and add X-axis values:
xval=0
xinc=0.5
xgap=0.5
num_columns=$(head -1 "$normalizedFlush" | wc -w)
for ((col=1; col<=num_columns; col++)); do
    while read -a line; do
      echo "$xval ${line[$((col-1))]}" >> "$graphFileFlush"
      xval=$(echo "$xval + $xinc" | bc)
    done < <(cut -d ' ' -f $col "$normalizedFlush")
    xval=$(echo "$xval + $xgap" | bc)
    echo "" >> "$graphFileFlush"
done
# Reshape for gnuplot and add X-axis values:
xval=0
xinc=0.5
xgap=0.5
num_columns=$(head -1 "$normalizedCmp" | wc -w)
for ((col=1; col<=num_columns; col++)); do
    while read -a line; do
      echo "$xval ${line[$((col-1))]}" >> "$graphFileCmp"
      xval=$(echo "$xval + $xinc" | bc)
    done < <(cut -d ' ' -f $col "$normalizedCmp")
    xval=$(echo "$xval + $xgap" | bc)
    echo "" >> "$graphFileCmp"
done
#################################
# Set to blue colors if 'Cold' Test, orange if 'Bulk' tests
if [[ $runMode -eq 0 ]]; then
  colorStr="set style line 1 lc rgb '#E0F7FA' # color for \"64B\"
set style line 2 lc rgb '#B3E5FC' # color for \"128B\"
set style line 3 lc rgb '#4FC3F7' # color for \"512B\"
set style line 4 lc rgb '#039BE5' # color for \"256B\"
set style line 5 lc rgb '#0277BD' # color for \"1KB\"
set style line 6 lc rgb '#01579B' # color for \"2KB\"
set style line 7 lc rgb '#01329B' # color for \"4KB\""
else
  colorStr="set style line 1 lc rgb '#FFF9C4' # color for \"64B\"
set style line 2 lc rgb '#FFECB3' # color for \"128B\"
set style line 3 lc rgb '#FFE082' # color for \"512B\"
set style line 4 lc rgb '#FFD54F' # color for \"256B\"
set style line 5 lc rgb '#FFC107' # color for \"1KB\"
set style line 6 lc rgb '#FFA000' # color for \"2KB\"
set style line 7 lc rgb '#FF6F00' # color for \"4KB\""
fi
#################################

if [[ $runMode -eq 0 ]]; then
  outputFile="results/Normalized_MemMv_Time_ColdMiss.png"
  titleStr="set title \"{/=15:Bold Avg Latency for MemMov Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=1,000)}\" offset 0,-0.35"
else
  outputFile="results/Normalized_MemMv_Time_Bulk.png"
  titleStr="set title \"{/=15:Bold Avg Latency for MemMov Instructions}\n{/=12:Bold Bulk Run Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=100,000)}\" offset 0,-0.35"
fi

#################################
########## Graph 1 ##############
# Initialize the xtics string
xLoc=1.2
xtic_string="set xtics ("
for i in {0..9}; do
  xtic_string+="\"{/=10.5:Bold ${axisNamesMemMv[i]}}\" $xLoc"
  xLoc=$(echo "$xLoc + 4" | bc)
  [[ $i -lt 9 ]] && xtic_string+=", " # Add comma if it's not the last benchmark
done
xtic_string+=")"

# Test String
#echo -e "String = \"$xtic_string\"\n"
#exit

# Define the output PNG file
#outputFile="results/Normalized_MemMv_Time_ColdMiss.png"

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
set key at screen .985,0.85
set key spacing 1.1
set key samplen 1
set key box linestyle 1
set label "{/=8:Bold Buffer Sizes}" at screen 0.8625, 0.58

# Margins configuration
set lmargin at screen 0.1255
set rmargin at screen 0.835
#set tmargin at screen 0.975
#set bmargin at screen 0.2

# Axes, labels, and grid configuration
#set title "{/=15:Bold Avg Latency for MemMov Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}"
$titleStr
set termoption enhanced
set ylabel "{/:Bold Normalized Latency}\n{/=10(Baseline=x86 'movq')}" offset 1,0
#set xlabel "{/=14:Bold Instruction Type}\n{/=10(N=1,000)}" offset 0,-0.35
$xlabelStr
set ytics nomirror

# Custom xtics
set xtics nomirror rotate by -45 font ", 9"
set xtics scale 0
#set ytics scale 0
$xtic_string

# Setting up the y-range and x-range
set yrange [0:3]
set xrange [-0.9:39.85]

$colorStr

# Normalized bounds with a red line at y=1
set arrow from graph 0, first 1 to graph 1, first 1 nohead lc rgb "red" lw 2
#set arrow from screen 0.73, screen 0.75 to screen 0.74, screen 0.825 lc rgb "red" lw 2
#set label "{/=8:Bold Up to ${largestAMX_rounded}x Baseline}" textcolor rgb "red" at screen 0.55, 0.725

plot \
  '$graphFileMemMv' using 1:2 every ::0::0 with boxes ls 1 title "{/:Bold 64B}",\
  '$graphFileMemMv' using 1:2 every ::1::1 with boxes ls 2 title "{/:Bold 128B}",\
  '$graphFileMemMv' using 1:2 every ::2::2 with boxes ls 3 title "{/:Bold 256B}",\
  '$graphFileMemMv' using 1:2 every ::3::3 with boxes ls 4 title "{/:Bold 512B}",\
  '$graphFileMemMv' using 1:2 every ::4::4 with boxes ls 5 title "{/:Bold 1KB}",\
  '$graphFileMemMv' using 1:2 every ::5::5 with boxes ls 6 title "{/:Bold 2KB}",\
  '$graphFileMemMv' using 1:2 every ::6::6 with boxes ls 7 title "{/:Bold 4KB}"

EOF

echo -e "\nDone! Graph saved to: $outputFile\n"
#################################
#################################

if [[ $runMode -eq 0 ]]; then
  outputFile="results/Normalized_Flush_Time_ColdMiss.png"
  titleStr="set title \"{/=15:Bold Avg Latency for Flush Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=1,000)}\" offset 0,-0.35"
else
  outputFile="results/Normalized_Flush_Time_Bulk.png"
  titleStr="set title \"{/=15:Bold Avg Latency for Flush Instructions}\n{/=12:Bold Bulk Run Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=100,000)}\" offset 0,-0.35"
fi

#################################
########## Graph 2 ##############
# Initialize the xtics string
xLoc=1.2
xtic_string="set xtics ("
for i in {0..2}; do
  xtic_string+="\"{/=10.5:Bold ${axisNamesFlush[i]}}\" $xLoc"
  xLoc=$(echo "$xLoc + 4" | bc)
  [[ $i -lt 2 ]] && xtic_string+=", " # Add comma if it's not the last benchmark
done
xtic_string+=")"

# Test String
#echo -e "String = \"$xtic_string\"\n"
#exit

# Define the output PNG file
#outputFile="results/Normalized_Flush_Time_ColdMiss.png"

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
set key at screen .985,0.85
set key spacing 1.1
set key samplen 1
set key box linestyle 1
set label "{/=8:Bold Buffer Sizes}" at screen 0.8625, 0.58

# Margins configuration
set lmargin at screen 0.1255
set rmargin at screen 0.835
#set tmargin at screen 0.975
#set bmargin at screen 0.2

# Axes, labels, and grid configuration
#set title "{/=15:Bold Avg Latency for Flush Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}"
$titleStr
set termoption enhanced
set ylabel "{/:Bold Normalized Latency}\n{/=10(Baseline=x86 'clflush')}" offset 1,0
#set xlabel "{/=14:Bold Instruction Type}\n{/=10(N=1,000)}" offset 0,0
$xlabelStr
set ytics nomirror

# Custom xtics
set xtics nomirror rotate by -45 font ", 9"
set xtics scale 0
#set ytics scale 0
$xtic_string

# Setting up the y-range and x-range
set yrange [0:2]
set xrange [-0.9:12]

$colorStr

# Normalized bounds with a red line at y=1
set arrow from graph 0, first 1 to graph 1, first 1 nohead lc rgb "red" lw 2
#set arrow from screen 0.73, screen 0.75 to screen 0.74, screen 0.825 lc rgb "red" lw 2
#set label "{/=8:Bold Up to ${largestAMX_rounded}x Baseline}" textcolor rgb "red" at screen 0.55, 0.725

plot \
  '$graphFileFlush' using 1:2 every ::0::0 with boxes ls 1 title "{/:Bold 64B}",\
  '$graphFileFlush' using 1:2 every ::1::1 with boxes ls 2 title "{/:Bold 128B}",\
  '$graphFileFlush' using 1:2 every ::2::2 with boxes ls 3 title "{/:Bold 256B}",\
  '$graphFileFlush' using 1:2 every ::3::3 with boxes ls 4 title "{/:Bold 512B}",\
  '$graphFileFlush' using 1:2 every ::4::4 with boxes ls 5 title "{/:Bold 1KB}",\
  '$graphFileFlush' using 1:2 every ::5::5 with boxes ls 6 title "{/:Bold 2KB}",\
  '$graphFileFlush' using 1:2 every ::6::6 with boxes ls 7 title "{/:Bold 4KB}"

EOF

echo -e "Done! Graph saved to: $outputFile\n"
#################################
#################################

if [[ $runMode -eq 0 ]]; then
  outputFile="results/Normalized_Compare_Time_ColdMiss.png"
  titleStr="set title \"{/=15:Bold Avg Latency for Compare Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=1,000)}\" offset 0,-0.35"
else
  outputFile="results/Normalized_Compare_Time_Bulk.png"
  titleStr="set title \"{/=15:Bold Avg Latency for Compare Instructions}\n{/=12:Bold Bulk Run Instructions}\""
  xlabelStr="set xlabel \"{/=14:Bold Instruction Type}\n{/=10(N=100,000)}\" offset 0,-0.35"
fi

#################################
########## Graph 3 ##############
# Initialize the xtics string
xLoc=1.2
xtic_string="set xtics ("
for i in {0..5}; do
  xtic_string+="\"{/=10.5:Bold ${axisNamesCmp[i]}}\" $xLoc"
  xLoc=$(echo "$xLoc + 4" | bc)
  [[ $i -lt 5 ]] && xtic_string+=", " # Add comma if it's not the last benchmark
done
xtic_string+=")"

# Test String
#echo -e "String = \"$xtic_string\"\n"
#exit

# Define the output PNG file
#outputFile="results/Normalized_Compare_Time_ColdMiss.png"

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
set key at screen .985,0.85
set key spacing 1.1
set key samplen 1
set key box linestyle 1
set label "{/=8:Bold Buffer Sizes}" at screen 0.8625, 0.58

# Margins configuration
set lmargin at screen 0.1255
set rmargin at screen 0.835
#set tmargin at screen 0.975
#set bmargin at screen 0.2

# Axes, labels, and grid configuration
#set title "{/=15:Bold Avg Latency for Compare Instructions}\n{/=12:Bold Serialized 'Cold Miss' Instructions}"
$titleStr
set termoption enhanced
set ylabel "{/:Bold Normalized Latency}\n{/=10(Baseline=x86 'C memcmp')}" offset 1,0
#set xlabel "{/=14:Bold Instruction Type}\n{/=10(N=1,000)}" offset 0,0
$xlabelStr
set ytics nomirror

# Custom xtics
set xtics nomirror rotate by -45 font ", 9"
set xtics scale 0
#set ytics scale 0
$xtic_string

# Setting up the y-range and x-range
set yrange [0:4]
set xrange [-0.9:23.8]

$colorStr

# Normalized bounds with a red line at y=1
set arrow from graph 0, first 1 to graph 1, first 1 nohead lc rgb "red" lw 2
#set arrow from screen 0.73, screen 0.75 to screen 0.74, screen 0.825 lc rgb "red" lw 2
#set label "{/=8:Bold Up to ${largestAMX_rounded}x Baseline}" textcolor rgb "red" at screen 0.55, 0.725

plot \
  '$graphFileCmp' using 1:2 every ::0::0 with boxes ls 1 title "{/:Bold 64B}",\
  '$graphFileCmp' using 1:2 every ::1::1 with boxes ls 2 title "{/:Bold 128B}",\
  '$graphFileCmp' using 1:2 every ::2::2 with boxes ls 3 title "{/:Bold 256B}",\
  '$graphFileCmp' using 1:2 every ::3::3 with boxes ls 4 title "{/:Bold 512B}",\
  '$graphFileCmp' using 1:2 every ::4::4 with boxes ls 5 title "{/:Bold 1KB}",\
  '$graphFileCmp' using 1:2 every ::5::5 with boxes ls 6 title "{/:Bold 2KB}",\
  '$graphFileCmp' using 1:2 every ::6::6 with boxes ls 7 title "{/:Bold 4KB}"

EOF

echo -e "Done! Graph saved to: $outputFile\n"




# Comment out to allow for deletion of temp files
#exit
rm results/.temp*