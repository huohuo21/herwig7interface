# herwig7interface
Repo for upcoming Herwig7 interface in CMSW

## Setup of Herwig7 interface (Status of 15.11.2016)
* For the setup and test of the Herwig7 interface the following steps are necessary.
* The `cmsenv` and the `source herwig7-env.sh` has to be done always, meaning, each time you open a new terminal. All the other steps have to be done only once.

1. Installation of CMSSW 7_1_25_patch1 
  * For the moment I provide a local Herwig7 installation since the global Herwig7 executable is not working due to some wrongly set paths
  ```
scram p CMSSW_7_1_25_patch1
  ```
2. Set CMSSW environment 
  * Has to be done in each new terminal
  ```
cd CMSSW_7_1_25_patch1/src
cmsenv
  ```
3. Clone newest version of Herwig7 interface
  * If you would like to contribute it is maybe better to fork my repository and to clone your own version since you can then push stuff to your repository and make use of pull requests.
  ```
  git clone git@github.com:mharrend/herwig7interface.git
  ```
4. Sourcing of local Herwig7 installation 
  * Has to be done in each new terminal
  * In this way, the path to my local Herwig7 installation is set. Otherwise, CMSSW would provide only  Herwig++ 2.7.1
  ```
source herwig7-env.sh
  ```
5. Reset scram build and rebuild of Herwig7 interface
  * During the second step you can ignore the following two warnings:
    * ****WARNING: Invalid tool madgraph5amcatnlo. Please fix herwigpp file.
    * fatal: ambiguous argument 'CMSSW_7_1_25_patch1': unknown revision or path not in the working tree.
  ```
scram b clean
scram b -j 10
  ```
6. Test of the Herwig7 interface
  * For testing the recently build interface you can run one of the following cmsRun files via ```cmsRun [Filename]```
    * Herwigpp_Herwig7interfaceTestProcess_cff_py_GEN_SIM.py
    * Herwigpp_MatchboxHerwig7interface_cff_py_GEN_SIM.py
    * Herwigpp_ConfigFile_cff_py_GEN_SIM.py: Note this cmsRun config works only if you provide a Herwig inputfile called TestConfig.in
  * Note the cmsRun config file Herwigpp_TestProcess_cff_py_GEN_SIM.py needs some update and does not work.
  
## Changing code of Herwig7 interface
* You can change the code of the Herwig7 interface and rebuild the interface.
* For this, in a new terminal you have always to setup the CMSSW environment and source the local Herwig7 installation via
  ```
  cd CMSSW_7_1_25_patch1/src
  
  cmsenv
  source herwig7-env.sh
  ```
* After a code change you can rebuild the interface via
  ```
  scram b -j 10
  ```
* If strange errors occur, try a ``` scram b clean``` before rebuild the interface.
  
## Code structure
* Main folder
 * Herwigpp_ConfigFile_cff_py_GEN_SIM.py: Uses Herwig7 interface to read in an external Herwig7 input file called TestConfig.in. Difference to other cmsRun files is that the Herwig7 config is not provided in the python code but as an external file.
 * Herwigpp_Herwig7interfaceTestProcess_cff_py_GEN_SIM.py: Similar to Herwigpp_TestProcess_cff_py_GEN_SIM.py. Use the old Herwig++ parametrization style. In more detail, an internal matrix element for ttH is used.
 * Herwigpp_MatchboxHerwig7interface_cff_py_GEN_SIM.py: Uses the new Herwig7 Matchbox interface to use MadGraph5aMC@NLO as a matrix element provider.
 * Herwigpp_TestProcess_cff_py_GEN_SIM.py: Needs some update. Uses Herwig's internal matrixelement as a basic example. 
 * TestConfig.in: Herwig7 inputfile used as a test config together with the Herwigpp_ConfigFile_cff_py_GEN_SIM.py cmsRun file
	* herwig7-env.sh: Bash script to set paths for local Herwig7 installation
	* herwigpp.xml: Needed to overwrite scram config, so that Herwig7 is available instead of CMSSW's Herwig++
	* madgraph5amcatnlo.xml: Needed to provide MadGraph5aMC@NLO as external matrix element provider
	* thepeg.xml: Needed to overwrite scram config, so that ThePEG 2.0.X is available instead of CMSSW's 1.9.2

## Matchbox interface: Available external matrix element providers
* MadGraph5aMC@NLO
* GoSam
* OpenLoops

## Current workflow
* If you use a cmsRun config file together with cmsRun the following happens:
1. The configuration in the "Herwig7GeneratorFilter" part and the external Herwig7 inputfile which is set via the optional "configFiles" option is converted by the Herwig7 interface into a Herwig7 input file. The default file name is HerwigConfig.in. Furthermore, it defines a HerwigUI object with some basic settings.
2. The Herwig7 interface will call then the read mode of Herwig7 via the Herwig7 API handing over the HerwigUI object which was constructed before. Herwig7 will produce then a run file with the default name InterfaceTest.run.
3. After the successful read step, the Herwig7 interface will invoke the Herwig7 API again. This time handing over a slightly changed HerwigUI object which requests the Herwig7 run mode and points to the freshly created Herwig7 run file. Herwig7 will then be in the run mode producing events.

