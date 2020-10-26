/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.70
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef DCD_OUTPUT_H
#define DCD_OUTPUT_H

#include <vector> //for molecule string storage.
#include <string> //to store lines of finished data.
#include <iostream>
#include <cstring>

#include "BasicTypes.h" //For uint

#include "OutputAbstracts.h"
#include "Molecules.h"
#include "MoleculeKind.h"
#include "StaticVals.h"
#include "Coordinates.h"
#include "DCDlib.h"
#include "PDBSetup.h" //For atoms class

class System;
namespace config_setup
{
struct Output;
}
class MoveSettings;
class MoleculeLookup;

struct DCDOutput : OutputableBase {
public:
  DCDOutput(System & sys, StaticVals const& statV);

  ~DCDOutput()
  {
    if (x) {
      delete [] x; // y and z are continue of x
    }

    for (uint b = 0; b < BOX_TOTAL; ++b) {
      close_dcd_write(stateFileFileid[b]);
      if(restartCoor[b]) {
        delete [] restartCoor[b];
      }
      if(outDCDStateFile[b]) {
        delete [] outDCDStateFile[b];
      }
      if(outDCDRestartFile[b]) {
        delete [] outDCDRestartFile[b];
      }
    }
  }

  //PDB does not need to sample on every step, so does nothing.
  virtual void Sample(const ulong step) {}

  virtual void Init(pdb_setup::Atoms const& atoms,
                    config_setup::Output const& output);

  virtual void DoOutput(const ulong step);
private:

  void Copy_lattice_to_unitcell(double *unitcell, int box);
  void SetCoordinates(float *x, float *y, float *d,
      std::vector<int> &molInBox, const int box);
  // unwrap and same coordinates of molecule in box
  void SetMolInBox(const int box);
  //Return a vector that defines the box id for each molecule
  void SetMolBoxVec(std::vector<int> & mBox);
  // returns the total number of atoms in box
  int NumAtomInBox(const int box);
  // Write a binary restart file with coordinates
  void Write_binary_file(char *fname, int n, XYZ *vec); 

  MoveSettings & moveSetRef;
  MoleculeLookup & molLookupRef;
  BoxDimensions& boxDimRef;
  Molecules const& molRef;
  Coordinates & coordCurrRef;
  COM & comCurrRef;

  char *outDCDStateFile[BOX_TOTAL];
  char *outDCDRestartFile[BOX_TOTAL];
  int stateFileFileid[BOX_TOTAL];
  bool enableRestartOut, enableStateOut;
  ulong stepsRestartPerOut, stepsStatePerOut;

  float *x, *y, *z;
  XYZ *restartCoor[BOX_TOTAL];
};

#endif /*DCD_OUTPUT_H*/
