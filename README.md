# IntelDSA Analysis

Private Repo Exploring Intel DSA Architecture and Analysis

# Scripts (IntelDSA_Analysis/myScripts/)

Scripts were written with the intention of running them FROM the root folder (IntelDSA_Analysis/), using them from another folder may cause issues. 

I tried to make them as general as possible to maximize different usecases for different users/purposes.

# Use DSATest

1) You must configure at least 1 Group to use, which maps DSA-WQs -> DSA-Engines. 
	Run myScripts/configDSA.sh and follow the instructions (run as root/sudo)...

2) For simplicity of my program, I statically defined the WQ I chose in DSATest.h.
	Modify DSATest.h and change wqPath string to match your WQ

3) Run DSATest (bin/DSATest)
