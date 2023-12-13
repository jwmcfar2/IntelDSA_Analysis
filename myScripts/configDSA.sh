#!/bin/bash

## Set to fn so it can be called from multiple diff places ###
### Optional: Set WQ DevFile Permissions ####
permissionSet(){
    read -p $'\n- Would you like to set this WQ available to all users (user-space programs)? (y/n): ' userInput

    if [[ $userInput == *[yY]* && $userInput != *[nN]* ]]; then
        for wq in ${wqIDs[@]}; do
            sudo chmod 666 /dev/dsa/wq$dsaID.$wq
            echo -e "\nSet permissions for: /dev/dsa/wq$dsaID.$wq"
        done
            echo -e "\nFinished setting all permissions for all WQs!\n"
    else
        exit
    fi
}
###############################################################

groupID=0
wqSettings="--wq-size=32 --priority=1 --block-on-fault=0 --threshold=4 --type=user --name=swq --mode=shared --max-batch-size=32 --max-transfer-size=2097152"
confFolder="$(pwd)/configs/"

if [[ $EUID -ne 0 ]]; then
  echo -e "This script must be run as super-user/root (run 'sudo $(basename "$0")')"
  exit 1
fi

# Make a string which lists all DSA devices, user chooses which to config (NOTE: AZAM LIKELY USING DSA0!)
firstStr="- Choose which DSA Device to configure "
BuildStr="    (Available: dsa"
while IFS= read -r number; do
    # Append each number to the BuildStr string
    BuildStr+="$number, dsa"
done < <(ls -df /sys/bus/dsa/devices/dsa* | grep -v 'wq' | awk '{print substr($0, length($0))}')
BuildStr=${BuildStr:: -5}
BuildStr+="): "

# Prompt user
echo -e "\n$firstStr"
read -p "$BuildStr" userInput
dsaID+=$(echo "$userInput" | grep -o '[0-9]\+')
echo -e "\nDevice Chosen: \"dsa$dsaID\""

# Put device into 'config' mode
accel-config config-device dsa$dsaID
echo -e "Set dsa$dsaID to allow for new config..."

# Choose between 'Load Existing' or 'Make New Config'
read -p $'\n- Make Selection\n\t[1] -- Load Existing Config from \"configs/\"\n\t[2] -- Create and save new config\n\t\tSelection (1/2): ' userInput

# Load Config
if [[ userInput -eq 1 ]]; then
    echo -e "\t### AUTHOR NOTE: For some reason, I can't get this to load an old config\n\tafter a server restart - either debug this script or just make a new config altogether if a restart occurs... _JMac\n"
    echo -e "\nDiscovered .conf files:"
    ls $confFolder
    mostRecentConf=$(ls -t configs/ | head -n1)
    read -e -p $'\n- Make Selection\n\t[1] -- Load Most Recent Config: "'"${mostRecentConf}"$'"\n\t[2] -- Manually Enter Different File Name\n\t\tSelection (1/2): ' configInput
    
    if [[ $configInput != 1 ]]; then
        read -p $'\n\tChoose name of file (\"file.conf\"): ' fileName
    else
        fileName=$mostRecentConf
    fi
    
    echo -e "\n'Disabling' dsa$dsaID to allow for new config..."
    accel-config config-device dsa$dsaID
    accel-config load-config -c $confFolder$fileName -e
    echo -e "\nFinished loading config $confFolder$fileName"
    permissionSet
    exit
fi

# Get Group-ID #
while true; do
    read -p $'\n- Choose Group-ID# [0-3] (Default:0): ' groupID
    if [[ $groupID == "0" || $groupID == "1" || $groupID == "2" || $groupID == "4" ]]; then
        break
    fi
    echo -e "Invalid Choice. Select an ID between [0 - 3]...\n"
done
echo ""

##### Config Engines/Group #####
# Get User's choice of WQ Mode
while true; do
    read -p $'- Choose DSA Engine Configuration Mode\n\t[1] -- *ONE* Engine Mapped to WQ(s)\n\t[2] -- ALL Engines Mapped to WQ(s)\n\t\tSelection (1/2): ' userInput

    if [[ $userInput == "1" || $userInput == "2" ]]; then
        wqMode=$userInput
        break
    fi
    echo -e "Invalid Choice. Select options '1' or '2'...\n"
done
echo ""

# Find all available engines
maxEngineID=$(ls -df /sys/bus/dsa/devices/dsa2/engine2* | tail -n1 | awk '{print substr($0, length($0))}')

# Set the actual Engine Config Mode for an entire group
case $wqMode in
    1) # Single-Engine Mapping
        accel-config config-engine dsa${dsaID}/engine${dsaID}.0 --group-id=$groupID
        echo -e "\tMapped a single engine (dsa${dsaID}/engine${dsaID}.0) to Group-$groupID"
        ;;
    2) # Full-Engine Mapping
        for engine in $(seq 0 $maxEngineID); do
            accel-config config-engine dsa${dsaID}/engine${dsaID}.${engine} --group-id=$groupID
            echo -e "\tMapped an engine (dsa${dsaID}/engine${dsaID}.${engine}) to Group-$groupID"
        done
        ;;
    *)
        echo "ERROR: Corrupted wqMode - Exiting..."
        exit
        ;;
esac

# Add the WQ(s)
# Make a string which lists all groups for device, user chooses which one(s) to config
firstStr="- Choose which WQ(s) to configure, separated by a 'space'"
BuildStr="    (Available WQs: "
while IFS= read -r number; do
    # Append each number to the BuildStr string
    BuildStr+="$number, "
done < <(ls -df /sys/bus/dsa/devices/dsa2/wq2* | awk '{print substr($0, length($0))}')
BuildStr=${BuildStr:: -2}
BuildStr+="): "

# Prompt user -- WQs now stored in 'wqIDs' Array
echo -e "\n$firstStr"
read -p "$BuildStr" userInput
userMod=$(echo "$userInput" | grep -o '[0-9]\+'| tr '\n' ' ')
userMod="${userMod%}"
IFS=' ' read -r -a wqIDs <<< "$userMod"
wqArrLen=${#wqIDs[@]}
wqStr="WQs Chosen:"
for wq in ${wqIDs[@]}; do
    wqStr+=" WQ-$wq"
done
wqStr+="\n"
echo -e "\n$wqStr"

# Config each specified WQ
for wq in ${wqIDs[@]}; do
    accel-config config-wq dsa${dsaID}/wq${dsaID}.${wq} --group-id=$groupID $wqSettings
    echo -e "Mapped WQ-${wq} to Group-$groupID\n"
done

echo -e "\t### AUTHOR NOTE: If you see errors like 'enabled 0 out of 1',\n\tthat just means everything is already enabled - safe to ignore! _JMac\n"
################################

##### Enable the Device #####
echo -e "Enabling Device and WQs..."
accel-config enable-device dsa$dsaID
for wq in ${wqIDs[@]}; do
    accel-config enable-wq dsa$dsaID/wq$dsaID.$wq
done
################################

##### Save Config to File #####
read -p $'\n- Choose name for file: ' fileName
if [[ $fileName != *".conf" ]]; then
    fileName+=".conf"
fi
if [[ $fileName != configs/* ]]; then
    fileName="configs/$fileName"
fi
accel-config save-config -s $fileName
chmod 666 $(pwd)/$fileName
echo -e "Saved this config to: \"$fileName\""
################################

### Optional: Read File Now ####
echo -e "\n\t### AUTHOR NOTE: When you first make this config - it \n\ttakes a snapshot of all DSA configs..."
echo -e "\tThis means if you don't want to 'touch' other DSA\n\taccelerators, don't load this .conf immediately..."
echo -e "\tInstead, open the file directly, delete the necessary\n\t sections so only your DSA section is left..."
echo -e "\tThen just rerun this script, and choose the 'load config' option! _JMac"
read -p $'\n- Load config now? (y/n): ' userInput

if [[ $userInput == *[yY]* && $userInput != *[nN]* ]]; then
    accel-config load-config -c $(pwd)/$fileName -e
    echo -e "\nFinished loading config $(pwd)/$fileName"
    permissionSet
else
    exit
fi
################################