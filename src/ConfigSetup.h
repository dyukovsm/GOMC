/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.70
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef CONFIGSETUP_H
#define CONFIGSETUP_H

#include <map> //for function handle storage.
#include <iostream> //for cerr, cout;
#include <string> //for var names, etc.

#include "InputAbstracts.h" //For ReadableBase parent class.
#include "InputFileReader.h" // Input reader
#include "BasicTypes.h" //for uint, ulong
#include "EnsemblePreprocessor.h" //For box total;
#include "Reader.h" //For config reader
#include "FFConst.h" //For # of param file kinds
#include "MersenneTwister.h" //For MTRand::uint32
#include "XYZArray.h" //For box dimensions.
#include "MoveConst.h"
#include "UnitConst.h" //For bar --> mol*K / A3 conversion
#include "EnergyTypes.h"

#if ENSEMBLE == GCMC
#include <sstream>  //for reading in variable # of chem. pot.
#endif

#include "GOMC_Config.h"    //For PT
#include "ParallelTemperingPreprocessor.h"
#include <sstream>  //for prefixing uniqueVal with the pathToReplicaOutputDirectory
#ifdef WIN32
#define OS_SEP '\\'
#else
#define OS_SEP '/'
#endif

namespace config_setup
{
/////////////////////////////////////////////////////////////////////////
// Reoccurring structures

//A filename
struct FileName {
  std::string name;
  bool defined;
  FileName(void) {
    defined = false;
    name = ""; 
  }
  FileName(std::string file, bool def) {
    defined = def;
    name = file; 
  }
};

//Multiple filenames
template <uint N>
struct FileNames {
  std::string name[N];
  bool defined[N];
  FileNames(void) {
    std::fill_n(defined, N, false);  
  }
};

/////////////////////////////////////////////////////////////////////////
// Input-specific structures

//Could use "EnableEvent", but restart's enable arguably needs its
//own struct as "frequency" is misleading name for step number
struct RestartSettings {
  bool enable;
  ulong step;
  bool recalcTrajectory;
  bool restartFromCheckpoint;
  bool restartFromBinaryFile;
  bool restartFromXSCFile;
  bool operator()(void)
  {
    return enable;
  }
};

//Kinds of Mersenne Twister initialization
struct PRNGKind {
  std::string kind;
  MTRand::uint32 seed;
  bool IsRand(void) const
  {
    return str::compare(KIND_RANDOM, kind);
  }
  bool IsSeed(void) const
  {
    return str::compare(KIND_SEED, kind);
  }
  bool IsRestart(void) const
  {
    return str::compare(KIND_RESTART, kind);
  }
  static const std::string KIND_RANDOM, KIND_SEED, KIND_RESTART;
};

struct FFKind {
  uint numOfKinds;
  bool isCHARMM, isMARTINI, isEXOTIC;
  static const std::string FF_CHARMM, FF_EXOTIC, FF_MARTINI;
};

//Files for input.
struct InFiles {
  std::vector<FileName> param;
  FileNames<BOX_TOTAL> pdb, psf, binaryInput, xscInput;
  FileName seed;
};

//Input section of config file data.
struct Input {
  RestartSettings restart;
  PRNGKind prng, prngParallelTempering;
  FFKind ffKind;
  InFiles files;
};



/////////////////////////////////////////////////////////////////////////
// System-specific structures

struct Temperature {
  double inKelvin;
};

struct Exclude {
  uint EXCLUDE_KIND;

  static const std::string EXC_ONETWO, EXC_ONETHREE, EXC_ONEFOUR;
  static const uint EXC_ONETWO_KIND, EXC_ONETHREE_KIND, EXC_ONEFOUR_KIND;
};

struct PotentialConfig {
  uint kind;
  double cutoff;
  uint VDW_KIND;
};
struct VDWPot : public PotentialConfig {
  bool doTailCorr;
};
typedef PotentialConfig VDWShift;
struct VDWSwitch : public PotentialConfig {
  double cuton;
};

//Items that effect the system interactions and/or identity, e.g. Temp.
struct FFValues {
  uint VDW_KIND;
  double cutoff, cutoffLow, rswitch;
  bool doTailCorr, vdwGeometricSigma;
  std::string kind;

  static const std::string VDW, VDW_SHIFT, VDW_SWITCH, VDW_EXP6;
  static const uint VDW_STD_KIND, VDW_SHIFT_KIND, VDW_SWITCH_KIND, VDW_EXP6_KIND;
};

#if ENSEMBLE == GEMC || ENSEMBLE == NPT

//Items that effect the system interactions and/or identity, e.g. Temp.
struct GEMCKind {
  uint kind;
  double pressure;
};

#endif


struct Step {
  ulong total, equil, adjustment, pressureCalcFreq, parallelTempFreq, parallelTemperingAttemptsPerExchange;
  bool pressureCalc;
  bool parallelTemp;
};

//Holds the percentage of each kind of move for this ensemble.
struct MovePercents {
  double displace, rotate, intraSwap, intraMemc, regrowth, crankShaft,
         multiParticle;
  bool multiParticleEnabled;
#ifdef VARIABLE_VOLUME
  double volume;
#endif
#ifdef VARIABLE_PARTICLE_NUMBER
  double transfer, memc, cfcmc, targetedSwap;
#endif
};

struct ElectroStatic {
  bool readEwald;
  bool readElect;
  bool readCache;
  bool enable;
  bool ewald;
  bool cache;
  bool cutoffCoulombRead[BOX_TOTAL];
  double tolerance;
  double oneFourScale;
  double dielectric;
  double cutoffCoulomb[BOX_TOTAL];
  ElectroStatic(void)
  {
    std::fill_n(cutoffCoulombRead, BOX_TOTAL, false);
    std::fill_n(cutoffCoulomb, BOX_TOTAL, 0.0);
  }
};

struct Volume {
  bool hasVolume, cstArea, cstVolBox0;
  bool readCellBasis[BOX_TOTAL][3];
  XYZArray axis[BOX_TOTAL];
  Volume(void) : hasVolume(false), cstArea(false), cstVolBox0(false)
  {
    for(uint b = 0; b < BOX_TOTAL; ++b) {
      axis[b] = XYZArray(3);
      std::fill_n(readCellBasis[b], 3, false);
    }
  }

  bool ReadCellBasis() const
  {
    for(uint b = 0; b < BOX_TOTAL; ++b) {
      for(uint i = 0; i < 3; i++) {
        if(!readCellBasis[b][i])
          return false;
      }
    }
    return true;
  }
};

//If particle number varies (e.g. GCMC, GEMC) load in parameters for
//configurational bias
struct GrowNonbond {
  uint first, nth;
};

//If particle number varies (e.g. GCMC, GEMC) load in parameters for
//configurational bias
struct GrowBond {
  uint ang, dih;
};

struct CBMC {
  GrowNonbond nonbonded;
  GrowBond bonded;
};

// Holds information that is required to define the cavity
// for targeted swap
struct TargetSwapParam {
  // defines the center of subVolume
  XYZ subVolumeCenter; 
  // defines the dimension of subVolume
  XYZ subVolumeDim; 
  // defines the targeted molecule kind
  std::vector<std::string> selectedResKind; 
  // defines box index for subVolume
  uint selectedBox;
  // defines the subVolume index for error checking
  int subVolumeIdx; 
  // defines if we do rigid body insertion/deletion or not
  // defaut value is true
  bool rigid_swap;

  bool center_defined, dim_defined, reskind_defined;
  bool box_defined, rigidSwap_defined;;
  TargetSwapParam(void) {
    center_defined = dim_defined = false;
    reskind_defined = box_defined = false;
    rigidSwap_defined = false;
    rigid_swap = false;
    subVolumeIdx = 0;
  }

  TargetSwapParam& operator=(TargetSwapParam const& rhs)
  {
    subVolumeCenter = rhs.subVolumeCenter;
    subVolumeDim = rhs.subVolumeDim;
    selectedResKind = rhs.selectedResKind;
    selectedBox = rhs.selectedBox;
    subVolumeIdx = rhs.subVolumeIdx;
    rigid_swap = rhs.rigid_swap;
    //copy boolean parameters
    center_defined = rhs.center_defined;
    dim_defined = rhs.dim_defined;
    reskind_defined = rhs.reskind_defined;
    box_defined = rhs.box_defined;
    rigidSwap_defined = rhs.rigidSwap_defined;
    return *this;
  }

  // Make sure all parameters have been set
  void VerifyParm() {
    bool allSet = true;
    if(!box_defined) {
      printf("Error: Box has not been defined for subVolume index %d!\n",
            subVolumeIdx);
      allSet = false;
    }
    if(!center_defined) {
      printf("Error: Center has not been defined for subVolume index %d!\n",
            subVolumeIdx);
      allSet = false;
    }
    if(!dim_defined) {
      printf("Error: dimension has not been defined for subVolume index %d!\n",
            subVolumeIdx);
      allSet = false;
    }
    if(!reskind_defined) {
      printf("Error: residue kind has not been defined for subVolume index %d!\n",
            subVolumeIdx);
      allSet = false;
    }
    if(!rigidSwap_defined) {
      printf("Default: Rigid body swap type has been defined for subVolume index %d!\n",
            subVolumeIdx);
      rigid_swap = true;
    }

    if(!allSet)
      exit(EXIT_FAILURE);
  }
};

struct TargetSwapCollection {
  TargetSwapCollection(void) {
    enable = false;
  }
  // Search for cavIdx in the targetSwap. If it exists,
  // returns true + the index to targetSwap vector
  bool SearchExisting(const int &subVIdx, int &idx) {
    for (int i = 0; i < targetedSwap.size(); i++) {
      if (targetedSwap[i].subVolumeIdx == subVIdx) {
        idx = i;
        return true;
      }
    }
    return false;
  }

  // add a new subVolume
  void AddsubVolumeBox(const int &subVIdx, uint &box) {
    int idx = 0;
    if (box < BOXES_WITH_U_NB) {
      if (!SearchExisting(subVIdx, idx)) {
        // If the subVolume index did not exist, add one
        TargetSwapParam tempPar;
        tempPar.subVolumeIdx = subVIdx;
        tempPar.selectedBox = box;
        tempPar.box_defined = true;
        targetedSwap.push_back(tempPar);
      } else {
        // If subVolume index exist and subvolume box is defined
        if(targetedSwap[idx].box_defined) {
          printf("Error: The subVolume index %d has already been defined for Box %d!\n",
                subVIdx, targetedSwap[idx].selectedBox);
          printf("       Please use different subVolume index.\n");
          exit(EXIT_FAILURE);
        } else {
          targetedSwap[idx].selectedBox = box;
          targetedSwap[idx].box_defined = true;
        }
      }
    } else {
      printf("Error: Subvolume index %d cannot be set for box %d!\n", subVIdx, box);
      #if ENSEMBLE == GCMC
        printf("       Maximum box index for this simulation is %d!\n", BOXES_WITH_U_NB - 1);
      #else
        printf("       Maximum box index for this simulation is %d!\n", BOXES_WITH_U_NB - 1);
      #endif
      exit(EXIT_FAILURE);
    }
  }

  // add dimension of subVolume to subVIdx
  void AddsubVolumeDimension(const int &subVIdx, XYZ &dimension) {
    int idx = 0;
    if (!SearchExisting(subVIdx, idx)) {
      // If the subVolume index did not exist, add one
      TargetSwapParam tempPar;
      tempPar.subVolumeIdx = subVIdx;
      tempPar.subVolumeDim = dimension;
      tempPar.dim_defined = true;
      targetedSwap.push_back(tempPar);
    } else {
      // If subVolume index exist and subvolume dimension is defined
      if(targetedSwap[idx].dim_defined) {
        printf("Error: The subVolume dimension (%g, %g, %g) has already been defined for subVolume index %d!\n",
               targetedSwap[idx].subVolumeDim.x, targetedSwap[idx].subVolumeDim.y,
               targetedSwap[idx].subVolumeDim.z, subVIdx);
        printf("       Please use different subVolume index.\n");
        exit(EXIT_FAILURE);
      } else {
        targetedSwap[idx].subVolumeDim = dimension;
        targetedSwap[idx].dim_defined = true;
      }
    }
  }

  // add dimension of subVolume to subVIdx
  void AddsubVolumeCenter(const int &subVIdx, XYZ &center) {
    int idx = 0;
    if (!SearchExisting(subVIdx, idx)) {
      // If the subVolume index did not exist, add one
      TargetSwapParam tempPar;
      tempPar.subVolumeIdx = subVIdx;
      tempPar.subVolumeCenter = center;
      tempPar.center_defined = true;
      targetedSwap.push_back(tempPar);
    } else {
      // If subVolume index exist and and subvolume center is defined
      if(targetedSwap[idx].center_defined) {
        printf("Error: The subVolume center (%g, %g, %g) has already been defined for subVolume index %d!\n",
               targetedSwap[idx].subVolumeCenter.x, targetedSwap[idx].subVolumeCenter.y,
               targetedSwap[idx].subVolumeCenter.z, subVIdx);
        printf("       Please use different subVolume index.\n");
        exit(EXIT_FAILURE);
      } else {
        targetedSwap[idx].subVolumeCenter = center;
        targetedSwap[idx].center_defined = true;
      }
    }
  }

  // add targeted residue kind of subVolume to subVIdx
  void AddsubVolumeResKind(const int &subVIdx, std::vector<std::string> &kind) {
    CheckSelectedKind(subVIdx, kind);
    int idx = 0;
    if (!SearchExisting(subVIdx, idx)) {
      // If the subVolume index did not exist, add one
      TargetSwapParam tempPar;
      tempPar.subVolumeIdx = subVIdx;
      // if there is any kind defined!
      if (kind.size()) {
        tempPar.selectedResKind = kind;
        tempPar.reskind_defined = true;
        targetedSwap.push_back(tempPar);
      }
    } else {
      // If subVolume index exist and and reskind is defined
      if(targetedSwap[idx].reskind_defined) {
        printf("Error: The targeted residue kind has already been defined for subVolume index %d!\n",
                subVIdx);
        printf("       Please use different subVolume index.\n");
        exit(EXIT_FAILURE);
      } else {
        // if there is any kind defined!
        if (kind.size()) {
          targetedSwap[idx].selectedResKind = kind;
          targetedSwap[idx].reskind_defined = true;
        }
      }
    }
  }
  //Check if user defined multiple reskind
  void CheckSelectedKind(const int &subVIdx, std::vector<std::string> &kind) {
    std::vector<std::string> newKind = kind;
    std::vector<std::string>::iterator ip;
    std::sort(newKind.begin(), newKind.end());
    ip = std::unique(newKind.begin(), newKind.end());
    //Find Uniquie reskind
    newKind.resize(std::distance(newKind.begin(), ip));
    if(newKind.size() != kind.size()) {
      printf("Warning: Duplicated residue kind was defined for subVolume index %d!\n",
                subVIdx);
      printf("Warning: Proceed with unique residue kind for subVolume index %d!\n",
                subVIdx);
    }

    bool selectedAll = (std::find(newKind.begin(), newKind.end(), "ALL") != newKind.end());
    selectedAll |= (std::find(newKind.begin(), newKind.end(), "all") != newKind.end());
    selectedAll |= (std::find(newKind.begin(), newKind.end(), "All") != newKind.end());
    if(selectedAll) {
      if(newKind.size() > 1) {
        printf("Warning: %d additional residue kind was defined for subVolume index %d, while using all residues!\n",
                  newKind.size() - 1, subVIdx);
        printf("Warning: Proceed with all residue kind for subVolume index %d!\n",
                  subVIdx);
      }
      newKind.clear();
      newKind.push_back("ALL");
    }

    kind = newKind;
  }

  // add a swapType for subvolume
  void AddsubVolumeSwapType(const int &subVIdx, bool &isRigid) {
    int idx = 0;
      if (!SearchExisting(subVIdx, idx)) {
        // If the subVolume index did not exist, add one
        TargetSwapParam tempPar;
        tempPar.subVolumeIdx = subVIdx;
        tempPar.rigid_swap = isRigid;
        tempPar.rigidSwap_defined = true;
        targetedSwap.push_back(tempPar);
      } else {
        // If subVolume index exist and subvolume box is defined
        if(targetedSwap[idx].rigidSwap_defined) {
          printf("Error: The swap type has already been defined for subVolume index %d!\n",
                subVIdx);
          printf("       Please use different subVolume index.\n");
          exit(EXIT_FAILURE);
        } else {
          targetedSwap[idx].rigid_swap = isRigid;
          targetedSwap[idx].rigidSwap_defined = true;
        }
      }
  }

  public:
    std::vector<TargetSwapParam> targetedSwap;
    bool enable;
};

struct MEMCVal {
  bool enable, readVol, readRatio, readSmallBB, readLargeBB;
  bool readSK, readLK;
  bool MEMC1, MEMC2, MEMC3;
  XYZ subVol;
  std::vector<std::string> smallKind, largeKind;
  std::vector<uint> exchangeRatio;
  std::vector<std::string> smallBBAtom1, smallBBAtom2;
  std::vector<std::string> largeBBAtom1, largeBBAtom2;
  MEMCVal(void)
  {
    MEMC1 = MEMC2 = MEMC3 = false;
    readVol = readRatio = readSmallBB = false;
    readLargeBB = readSK = readLK = false;
  }
};

struct CFCMCVal {
  bool enable, readLambdaCoulomb, readLambdaVDW, readRelaxSteps;
  bool readHistFlatness, MPEnable, readMPEnable;
  uint relaxSteps;
  double histFlatness;
  //scaling parameter
  uint scalePower;
  double scaleAlpha, scaleSigma;
  bool scaleCoulomb;
  bool scalePowerRead, scaleAlphaRead, scaleSigmaRead, scaleCoulombRead;
  std::vector<double> lambdaCoulomb, lambdaVDW;
  CFCMCVal(void)
  {
    readLambdaCoulomb = readRelaxSteps = readHistFlatness = false;
    readMPEnable = MPEnable = readLambdaVDW = enable = false;
    scalePowerRead = scaleAlphaRead = scaleSigmaRead = scaleCoulombRead = false;
  }
};

struct FreeEnergy {
  bool enable, readLambdaCoulomb, readLambdaVDW, freqRead;
  bool molTypeRead, molIndexRead, iStateRead;
  uint frequency, molIndex, iState;
  //scaling parameter
  uint scalePower;
  double scaleAlpha, scaleSigma;
  bool scaleCoulomb;
  bool scalePowerRead, scaleAlphaRead, scaleSigmaRead, scaleCoulombRead;
  std::string molType;
  std::vector<double> lambdaCoulomb, lambdaVDW;
  FreeEnergy(void)
  {
    readLambdaCoulomb = readLambdaVDW = enable = freqRead = false;
    molTypeRead = molIndexRead = iStateRead = false;
    scalePowerRead = scaleAlphaRead = scaleSigmaRead = scaleCoulombRead = false;
  }
};


#if ENSEMBLE == GCMC
struct ChemicalPotential {
  bool isFugacity;
  std::map<std::string, double> cp;
};
#endif
struct SystemVals {
  ElectroStatic elect;
  Temperature T;
  FFValues ff;
  Exclude exclude;
  Step step;
  MovePercents moves;
  Volume volume; //May go unused
  CBMC cbmcTrials;
  MEMCVal memcVal, intraMemcVal;
  CFCMCVal cfcmcVal;
  FreeEnergy freeEn;
  TargetSwapCollection targetedSwapCollection;
#if ENSEMBLE == GCMC
  ChemicalPotential chemPot;
#elif ENSEMBLE == GEMC || ENSEMBLE == NPT
  GEMCKind gemc;
#endif
};

/////////////////////////////////////////////////////////////////////////
// Output-specific structures

struct EventSettings { /* : ReadableStepDependentBase*/
  bool enable;
  ulong frequency;
  bool operator()(void)
  {
    return enable;
  }
};

struct UniqueStr { /* : ReadableBase*/
  std::string val;
};

struct HistFiles { /* : ReadableBase*/
  std::string histName, number, letter, sampleName;
  uint stepsPerHistSample;
};

//Files for output.
struct OutFiles {
  /* For split pdb, psf, and dcd files , BOX 0 and BOX 1 */
  FileNames<BOX_TOTAL> pdb, splitPSF, dcd;

  /* For merged PSF */
  FileName psf, seed;
  HistFiles hist;
};
struct Settings {
  EventSettings block, hist;
  UniqueStr uniqueStr;
};

//Enables for each variable that can be tracked
struct OutputEnables {
  bool block, fluct;
};

struct TrackedVars {
  OutputEnables energy, pressure;
#ifdef VARIABLE_VOLUME
  OutputEnables volume;
#endif
#ifdef VARIABLE_PARTICLE_NUMBER
  OutputEnables molNum;
#endif
  OutputEnables density;
  OutputEnables surfaceTension;
};

struct SysState {
  EventSettings settings;
  OutFiles files;
};
struct Statistics {
  Settings settings;
  TrackedVars vars;
};
struct Output {
  SysState state, restart;
  SysState state_dcd, restart_dcd;
  Statistics statistics;
  EventSettings console, checkpoint;
};

}

class ConfigSetup
{
public:
  config_setup::Input in;
  config_setup::Output out;
  config_setup::SystemVals sys;
  ConfigSetup(void);
  void Init(const char *fileName, MultiSim const*const& multisim);
private:
  void fillDefaults(void);
  bool checkBool(std::string str);
  bool CheckString(std::string str1, std::string str2);
  void verifyInputs(void);
  InputFileReader reader;

  //Names of config file.
  static const char defaultConfigFileName[]; // "in.dat"
  static const char configFileAlias[];       // "GO-MC Configuration File"
};

#endif
