# IntelDSA Analysis

Repo Exploring Intel Data Streaming Accelerator (DSA) Architecture and Analysis

# Scripts (IntelDSA_Analysis/myScripts/)

Scripts were written with the intention of running them FROM the root folder (IntelDSA_Analysis/), using them from another folder may cause issues. 

I tried to make them as general as possible to maximize different use-cases for different users/purposes.

# System Environment

- Red Hat Enterprise Linux release 9.3 (Plow)
- GCC v11.4.1
- Any/All Libraries Needed for AVX, AMX, SSE<=4, DSA
- Root permission needed to initially config DSA (program can be ran via user-space)

# Use DSATest

1) You must configure at least 1 Group to use, which maps DSA-WQs -> DSA-Engines. 
	Run myScripts/configDSA.sh and follow the instructions (run as root/sudo)...

2) For simplicity of my program, I statically defined the WQ I chose in 'utils.h'. Modify 
	utils.h (or automate this) and change wqPath string to match your WQ (set to '/dev/dsa/wq2.0' by default)

3) Build code via script: myScripts/buildScript.sh

3) Run DSATests:
	- All Tests: ./initRun [numTests] [mode] {-q == quiet script prints | Optional}
	- One Test: bin/DSA... [params]

5) Example Uses:
    - Run 'Serialized Cold Miss' DSA Analysis/Comparison Tests (Slow! '100 tests' ~=10m): 
		- **myScripts/initRun 100 0**

	- Run 'Serialized Max Performance' DSA Analysis/Comparison Tests (numTests hard coded by 'BULK_TEST_COUNT' in utils.h, so only give it '1'): 
		- **myScripts/initRun 1 2** 

	- Run 'Batch Spectre' (NOTE: Not in 'initRun.sh' script! Syntax = bin/DSASpectre [bufferSize ( <= 4096 && divisible by 64)] [outputFile]):
		- **bin/DSASpectre 1024 results/batchSpectreFails.out > results/batchSpectrePrints.out**

	- Graph Cold Misses / Max Performance results (NOTE: Graph scripts may need a little manual tweaking of axis values for different runs to show data more clearly):
		- **myScripts/graphScript.sh 0**
		- **myScripts/graphScript.sh 2**

-------------------------------------------------------------------------------------------------------------------------

NOTE: I tried to avoid server-specific/user-specific code when possible, but cannot guarantee it will run as-is on all
systems that have DSA - some adjustments may be needed (posisbly other source code variables/configs I'm forgetting)
