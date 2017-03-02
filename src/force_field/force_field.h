/** The ForceField is in charge of holding potential objects and mediates
    everything that requires force field parameters.
  */

#ifndef SRC_FORCE_FIELD_FORCE_FIELD_H_
#define SRC_FORCE_FIELD_FORCE_FIELD_H_

#include <string>
#include <vector>
#include <fstream>
#include <random> 

#include "../molecules/bead.h"
#include "../molecules/molecule.h"
#include "potential_bond.h"
#include "potential_ewald.h"
#include "potential_external.h"
#include "potential_pair.h"

using namespace std;

class ForceField {
 private:
  // Basic simulation parameters.
  /** 1/kBT, should be set to 1. The energy unit of the simulation. */
  double beta;
  /** The number of npbcs to apply PBC. */
  int npbc;
  /** The x,y,z lengths of the simulation box, in unit length. */
  double box_l[3];
  /** The number of molecules. *Currently used by vp calc only. */
  int n_mol;
  /** Chain length of the polymer, assuming they all have the same length.
      *Currently used by vp calc only. */
  int chain_len;
  /** Number of chains. *Currently used by vp calc only. */
  int n_chain;
  /** Number of cations. *Currently used by vp calc only. */
  int n_cion;
  /** Number of anions. *Currently used by vp calc only. */
  int n_aion;
  /** Record whether to use grand canonical ensemble. */
  bool use_gc;
  /** Record whether to use pair potential. */
  bool use_pair_pot;
  /** Record whether to use Ewald sum potential. */
  bool use_ewald_pot;
  /** Record whether to use bond potential. */
  bool use_bond_pot;
  /** Use rigid bonds. */
  bool use_bond_rigid;
  /** Record whether to use angle potential. */
  bool use_angle_pot;
  /** Record whether to use dihedral potential. */
  bool use_dihed_pot;
  /** Record whether to use external potential. */
  bool use_ext_pot;

  // GC molecular information. Right now all GC beads have the same type and
  // charge, this could be changed in the future.
  /** A short code that serves as the type of the GC chain. */
  string gc_bead_symbol;
  /** Chain length for the GC added chains. Can only add chain of this length 
      but can delete chains of various length. */
  int gc_chain_len;
  /** The charge on a GC bead. */
  int gc_bead_charge;

  // Grand canonical ensemble parameters.
  /** The average frequency to perform a GC MC move. */
  int gc_freq;
  /** Chemical potential. */
  double chem_pot;
  /** See Frenkel Eq. (5.6.8) and (5.6.9). This variable is not exact the
      thermal de Broglie wavelengh lambda, but vol/lambda^3. Lambda depends on
      molecular mass, this universal de Broglie wavelength may need to be
      extended in the future. Also, currently this value is approximated by 1.
    */
  double gc_deBroglie_prefactor;
  /** The number of trial beads to generate for each CBMC growth. */
  int cbmc_no_of_trials;
  /** The array to store the weight for each trial, size = cbmc_no_of_trials. */
  double* cbmc_trial_weights;
  /** Store the generated trial beads for each CBMC growth. */
  vector<Bead> cbmc_trial_beads;
  /** Store the generated CBMC chain. */
  vector<Bead> cbmc_chain;
  /** Number of insertion to try for each configuration. */
  int mu_tot_ins;

  // Potential objects.
  /** Pair potential, could include dispersive and/or elec. */
  PotentialPair* pair_pot;
  /** Ewald sum potential, could include dispersive and/or elec. */
  PotentialEwald* ewald_pot;
  /** Bond potential. */
  PotentialBond* bond_pot;
  /** The length of the rigid bond if the flag is on. */
  double rigid_bond;
  /** External potential. */
  PotentialExternal* ext_pot; 

  ////////////////////////////////////////////////////
  // Pressure calculation variables and parameters. //
  ////////////////////////////////////////////////////
  /** The volume of the simulation box. */
  double vol;
  /*** Bjerrum length for pressure calculation. */
  double lB;
  /** The three diagonal elements for the pressure tensor. */
  double p_tensor[3];
  double p_tensor_hs[3];
  double p_tensor_el[3];
  double p_tensor_el_tot[3];
  // Virial method to calculate pressure for general systems "vp" for "virial
  // pressure".
  /** Hard sphere size. */
  double vp_bead_size;
  /** Bin resolution for g(r) itself. */
  double vp_g_res;
  /** Bin resolution for hard potential, see Chang and Sandler 1993. */
  double vp_hs_res;
  /** Determines the number of bins for bining g(r) for electrostatics. */ 
  double vp_el_res;
  /** Number of bins for g(r). */
  int vp_g_bin;
  /** Number of bins for hard potential. */
  int vp_hs_bin;
  /** No. of bins used along x, y, z, calculated using vp_res, for
      electrostatics. */
  int vp_el_bin[3];
  /** 1/3 of the size a bin in site array - one of x, y, z dimensions. */
  int vp_g_s;
  /** Pair correlation function between the molecules. */
  double * vp_hs_g;
  /** Average rr between the molecules. */
  double * vp_hs_rr;
  /** For the pressure contribution from electrostatics. */
  double * vp_el_rrx;
  double * vp_el_rry;
  double * vp_el_rrz;
  /** Pair correlation function between wall and the molecules. */
  double * vp_zwall_rr;
  /** The "partition function". */
  double vp_z;
  /** Temporary arrays for least square fitting. */
  double * vp_hs_g_lsx;
  double * vp_hs_g_lsy;
  /** Temporary arrays for least square fitting. */
  double * vp_hs_rr_lsx;
  double * vp_hs_rr_lsx2;
  double * vp_hs_rr_lsy;

 public:
  // Initialization functions.
  /** Constructor. */
  ForceField();
  /** Destructor. */
  ~ForceField();
  /** The "true constructor" of the force field. */
  void Initialize(double, int, double[3], int, vector<Molecule>&);
  /** Initialize all energy maps / vectors in potentials, set up gc if
      necessary. */
  void InitializeEnergy(vector<Molecule>&);

  // Energy functions.
  /** Only used in the translational MC moves. */ 
  double EnergyDifference(vector<Molecule>&, int);
  /** Read in GC parameters, if used, potentials and add potentials. */
  void FinalizeEnergies(vector<Molecule>&, bool, int); 
  /** Calculate the total energy between a bead and the rest of the system
      (other beads and external potential). */
  double BeadEnergy(Bead&, vector<Molecule>&, int, int, int);

  /** For a system of netrual, non-interactive hard chains with no other
      molecular species. The current version cannot calculate the pressure
      components. */
  void CalcPressureVirialHSEL(vector<Molecule>&, double);
  /** Get pressure components from their storage array. */
  string GetPressure();

  // GC functions.
  /** Return the frequency to use GC. */
  int GCFrequency();
  /** Configurational-bias Monte Carlo (CBMC) move to generate a set of trial
      beads to choose from. */
  double CBMCGenTrialBeads(Bead&, vector<Molecule>&, int, mt19937&, int, int);
  /** CBMC move for chain insertion. */
  bool CBMCChainInsertion(vector<Molecule>&, mt19937&); 
  /** Chain deletion. */
  int CBMCChainDeletion(vector<Molecule>&, mt19937&);
  double CalcChemicalPotential(vector<Molecule>&, mt19937&);
  /** Does all energy initializing for a new molecule. It requires that all IDs
      are properly assigned before hand. */
  void EnergyInitForAddedMolecule(vector<Molecule>&); 

  // Utilities.
  /** Return whether GC is used. */
  bool UseGC();
  bool UsePairPot();
  bool UseEwaldPot();
  bool UseBondPot();
  bool UseBondRigid();
  bool UseAnglePot();
  bool UseDihedPot();
  bool UseExtPot();
  double TotPairEnergy();
  double TotEwaldEnergy();
  double TotBondEnergy();
  double TotExtEnergy();
  /** Return the force due to the external potential. */
  double CalculateExternalForce(vector<Molecule>&);
  /** Return the length of the rigid bond. */
  double RigidBondLen();
  void CoordinateObeyRigidBond(vector<Molecule>&);
  void SetBoxLen(double []);

  /** For debug purposes only. */
  void GetEwaldEnergyComponents(vector<Molecule>&, double *);

}; 

#endif

