#include "force_field.h" 

#include <iomanip>
#include <sstream>

#include "../utilities/constants.h"
#include "../utilities/misc.h"
#include "potential_ewald_coul.h"
#include "potential_hard_sphere.h"
#include "potential_hard_wall.h"
#include "potential_spring.h"
#include "potential_truncated_lj.h"
#include "potential_truncated_lj_wall.h"
#include "potential_well_wall.h"

using namespace std; 

ForceField::ForceField () {

}

ForceField::~ForceField () {
  delete [] cbmc_trial_weights;
  delete [] vp_hs_g;
  delete [] vp_hs_rr;
  delete [] vp_hs_g_lsx;
  delete [] vp_hs_g_lsy;
  delete [] vp_zwall_rr;
  delete [] vp_hs_rr_lsx;
  delete [] vp_hs_rr_lsx2;
  delete [] vp_hs_rr_lsy;
  delete [] vp_el_rrx;
  delete [] vp_el_rry;
  delete [] vp_el_rrz;

}

void ForceField::Initialize(double beta_in, int npbc_in, double box_l_in[3],
                            int n_mol, vector<Molecule>& mols) {
  // Initial setting up.
  beta = beta_in;
  npbc = npbc_in;
  rigid_bond = -1;
  for (int i = 0; i < 3; i++) {
    box_l[i] = box_l_in[i];
  }
  vol = box_l[0]*box_l[1]*box_l[2];

  // Read in.
  string flag;
  cin >> flag >> vp_el_res;
  cin >> flag >> vp_bead_size;
  cin >> flag >> use_pair_pot;
  cin >> flag >> use_ewald_pot;
  cin >> flag >> use_bond_pot;
  cin >> flag >> use_bond_rigid;
  cin >> flag >> use_angle_pot;
  cin >> flag >> use_dihed_pot;
  cin >> flag >> use_ext_pot;
  cout << "  Force field parameters." << endl;
  cout << setw(35) << "[P] g(r) bin resolution (ul): " << vp_el_res
       << endl;
  cout << setw(35) << "[P] Hard sphere size (ul)   : " << vp_bead_size
       << endl;
  cout << setw(35) << "Use pair potential?         : " << YesOrNo(use_pair_pot)
       << endl;
  cout << setw(35) << "Use Ewald sum potential?    : " << YesOrNo(use_ewald_pot)
       << endl;
  cout << setw(35) << "Use bond potential?         : " << YesOrNo(use_bond_pot)
       << endl;
  cout << setw(35) << "Use rigid bond?             : " << YesOrNo(use_bond_rigid)
       << endl;
  cout << setw(35) << "Use angle potential?        : " << YesOrNo(use_angle_pot)
       << endl;
  cout << setw(35) << "Use dihedral potential?     : " << YesOrNo(use_dihed_pot)
       << endl;
  cout << setw(35) << "Use external potential?     : " << YesOrNo(use_ext_pot)
       << endl;
  cin >> flag >> use_gc;
  cout << setw(35) << "Use grand canonical MC?     : " << YesOrNo(use_gc)
       << endl;
  if (use_gc) {
    cin >> flag >> chem_pot
        >> flag >> gc_deBroglie_prefactor
        >> flag >> gc_freq        
        >> flag >> gc_chain_len
        >> flag >> gc_bead_charge
        >> flag >> gc_bead_symbol 
        >> flag >> cbmc_no_of_trials;
    cout << setw(35) << "[GC] Chemical pot (kBT)     : " << chem_pot << endl; 
    cout << setw(35) << "[GC] de Broglie wavelength^3: "
         << gc_deBroglie_prefactor << endl;
    cout << setw(35) << "[GC] GCMC move frequency    : " << gc_freq << endl;
    cout << setw(35) << "[GC] CBMC chain length      : " << gc_chain_len
         << endl;
    cout << setw(35) << "[GC] CBMC bead charge (e)   : " << gc_bead_charge
         << endl;
    cout << setw(35) << "[GC] CBMC bead type         : " << gc_bead_symbol
         << endl;
    cout << setw(35) << "[GC] CBMC trials            : " << cbmc_no_of_trials
         << endl;
  }

  // Set potentials.
  string potential_name;
  int hard_pot = 0;
  int soft_pot = 0;
  // Pair potential.
  if (use_pair_pot) {
    cin >> flag >> potential_name;
    if (potential_name == "TruncatedLJ") {
      soft_pot++;
      pair_pot = new PotentialTruncatedLJ(potential_name);
    }
    else if (potential_name == "HardSphere") {
      hard_pot++;
      pair_pot = new PotentialHardSphere(potential_name);
      //CoordinateObeyRigidBond(mols);
    }
    else {
      cout << "ForceField::ReadParameters: " << endl;
      cout << "  Undefined pair potential! Exiting." << endl; 
      exit(1); 
    }
  }
  // Ewald potential.
  if (use_ewald_pot) {
    cin >> flag >> potential_name;
    if (potential_name == "Coul") {
      soft_pot++;
      ewald_pot = new PotentialEwaldCoul(potential_name, box_l); 
    }
    else {
      cout << "ForceField::ReadParameters: " << endl;
      cout << "  Undefined Ewald potential! Exiting." << endl; 
      exit(1); 
    }
  }
  // Bonded potential.
  if (use_bond_pot) {
    cin >> flag >> potential_name;
    if (potential_name == "Spring") {
      soft_pot++;
      bond_pot = new PotentialSpring(n_mol, potential_name);
    }
    else {
      cout << "ForceField::ReadParameters: " << endl;
      cout << "  Undefined bonded potential! Exiting." << endl; 
      exit(1); 
    }
  }
  // Rigid bond.
  if (use_bond_rigid) {
    hard_pot++;
    cin >> flag >> rigid_bond;
    cout << setw(35) << "[RB] Rigid bond length      : "
         << rigid_bond << "A" << endl;
  }
  // External potential.
  if (use_ext_pot) {
    cin >> flag >> potential_name;
    if (potential_name == "TruncatedLJWall") {
      soft_pot++;
      ext_pot = new PotentialTruncatedLJWall(potential_name); 
    }
    else if (potential_name == "HardWall") {
      hard_pot++;
      ext_pot = new PotentialHardWall(potential_name);
    }
    else if (potential_name == "WellWall") {
      hard_pot++;
      cout << "  Note: Currently, no correct pressure calculation" << endl;
      cout << "        routine exists for well wall potential." << endl;
      ext_pot = new PotentialWellWall(potential_name);
    }
    else {
      cout << "ForceField::ReadParameters: " << endl;
      cout << "  Undefined external potential! Exiting." << endl;
      exit(1);
    }
  }

  ////////////////////////
  // A few more checks. //
  ////////////////////////
  if (soft_pot * hard_pot > 0) {
    cout << "  Note: Currently, pressure cannot be calculated" << endl;
    cout << "        correctly for all combinations of \"hard\" and" << endl;
    cout << "        \"soft\" potentials." << endl;
  }
  if (vp_bead_size < 0) {
    cout << "  Virial bead size has to be bigger than zero!"
         << "Exiting! Program complete." << endl;
    exit(1);
  }
  else {
    cout << "  Note: For the hard sphere part of the virial pressure" << endl;
    cout << "        calculation to be correct, the virial sphere" << endl;
    cout << "        size should be consistent with the bead sizes" << endl;
    cout << "        given to the \"rigid bond\",  the \"hard sphere" << endl;
    cout << "        potential\" and the \"hard wall potential\"" << endl;
  }
  if (pair_pot->PotentialName() == "HardSphere") {
    cout << "  Note: If hard sphere potential is used, all inital pair" << endl;
    cout << "        energies of adjacent beads will be set to 0" << endl;
    cout << "        assuming the input coordinates have no overlap!!!" << endl;
    cout << "        This is to avoid precision-caused overlaps." << endl;
  }

  if (use_ext_pot) {
    cout << "  Note: [!!!] If a wall confining potential is used, the" << endl;
    cout << "        z-length of the simulation box needs to be 3 to" << endl;
    cout << "        5 times wider than the confinement for the" << endl;
    cout << "        electrostatics calculation to be correct!" << endl;
  }
  
  ///////////////////////////////
  // For pressure calculation. //
  ///////////////////////////////
  // Determining the length of the polymer chains. *Assuming that they all have
  // the same length.
  chain_len = -1;
  if (use_gc) { 
    chain_len = gc_chain_len;
  }
  else {
    for (int i = 0; i < (int)mols.size(); i++) {
      chain_len = mols[i].Size();
      if (chain_len > 1) {
        break;
      }
    }    
  }

  // Determine the number of chains, cations and anions for the starting
  // configuration for the pressure calculation.
  this->n_mol = (int)mols.size();
  n_chain = 0;
  n_cion = 0;
  n_aion = 0;
  for (int i = 0; i < this->n_mol; i++) {
    if (mols[i].Size() > 1) {
      n_chain++;
    }
    else if (mols[i].bds[0].Charge() >= 0) {
      n_cion++;
    }
    else if (mols[i].bds[0].Charge() < 0) {
      n_aion++;
    }
  }

  // Initializing pressure calculation related variables.
  p_tensor[0] = p_tensor_hs[0] = p_tensor_el[0] = p_tensor_el_tot[0] = 0;
  p_tensor[1] = p_tensor_hs[1] = p_tensor_el[1] = p_tensor_el_tot[1] = 0;
  p_tensor[2] = p_tensor_hs[2] = p_tensor_el[2] = p_tensor_el_tot[2] = 0;

  // chain_len number of sites plus 2 that stands for counterion and coion.
  vp_g_s = (chain_len+2)*(chain_len+2);
  vp_hs_res = vp_bead_size*0.01;  // See Chang and Sandler (1993).
  vp_g_res = vp_bead_size*0.01;
  vp_hs_bin = 4;                  // See Chang and Sandler (1993).
  vp_g_bin = (int)floor((0.5*box_l[0]-vp_bead_size)/vp_g_res);
  // Four dimensional array. D1 - bin, D2 - xyz, D3 - site1, D4 site2.
  vp_hs_g = new double[vp_g_bin*3*vp_g_s];
  vp_hs_rr = new double[vp_hs_bin*3*vp_g_s];
  vp_zwall_rr = new double[vp_hs_bin];
  vp_hs_g_lsx = new double[vp_hs_bin];
  vp_hs_g_lsy = new double[vp_hs_bin];
  vp_hs_rr_lsx = new double[vp_hs_bin];
  vp_hs_rr_lsx2 = new double[vp_hs_bin];
  vp_hs_rr_lsy = new double[vp_hs_bin];
  for (int i = 0; i < vp_g_bin*3*vp_g_s; i++) {
    vp_hs_g[i] = 0;
  }
  for (int i = 0; i < vp_hs_bin*3*vp_g_s; i++) {
    vp_hs_rr[i] = 0;
  }
  for (int i = 0; i < vp_hs_bin; i++) {
    vp_zwall_rr[i] = 0;
    vp_hs_g_lsx[i] = (i+0.5)*vp_g_res + vp_bead_size;
    vp_hs_rr_lsx[i] = (i+0.5)*vp_hs_res + vp_bead_size;
    vp_hs_rr_lsx2[i] = (i+0.5)*vp_hs_res + vp_bead_size/2.0;
  }
  vp_z = 0;

  if (use_ewald_pot) {
    vp_el_bin[0] = (int)floor(0.5*box_l[0]/vp_el_res);
    vp_el_bin[1] = (int)floor(0.5*box_l[1]/vp_el_res);
    vp_el_bin[2] = (int)floor(0.5*box_l[2]/vp_el_res);
    // Three dimensional array. D1 - bin, D2 - site1, D3 site2.
    vp_el_rrx = new double[vp_el_bin[0]*vp_g_s];
    vp_el_rry = new double[vp_el_bin[1]*vp_g_s];
    vp_el_rrz = new double[vp_el_bin[2]*vp_g_s];
    for (int i = 0; i < vp_g_s; i++) {
      for (int j = 0; j < vp_el_bin[0]; j++) {
        vp_el_rrx[i*vp_el_bin[0] + j] = 0;
      }
      for (int j = 0; j < vp_el_bin[1]; j++) {
        vp_el_rry[i*vp_el_bin[1] + j] = 0;
      }
      for (int j = 0; j < vp_el_bin[2]; j++) {
        vp_el_rrz[i*vp_el_bin[2] + j] = 0;
      }
    }
    lB = ewald_pot->GetlB();
  }
  else {
    vp_el_bin[0] = 0;
    vp_el_bin[1] = 0;
    vp_el_bin[2] = 0;
    vp_el_rrx = new double[1];
    vp_el_rry = new double[1];
    vp_el_rrz = new double[1];
    lB = 0;
  }

  /////////////////////////////////////////
  // For chemical potential calculation. //
  /////////////////////////////////////////
  mu_tot_ins = 100;
  // These parameter assignments should be read from input in the next
  // development of the code.
  // This is for neutral system only so far.
  if (!use_gc) {
    gc_deBroglie_prefactor = 1;
    gc_chain_len = chain_len;
    gc_bead_charge = 0;
    gc_bead_symbol = "P";
    cbmc_no_of_trials = 20;
  }

  InitializeEnergy(mols);

}

void ForceField::InitializeEnergy(vector<Molecule>& mols) {
  if (use_pair_pot) {
    pair_pot->EnergyInitialization(mols, box_l, this->npbc); 
    cout << "  Initialized pair potential." << endl;
  }
  if (use_ewald_pot) {
    ewald_pot->EnergyInitialization(mols, this->npbc); 
    cout << "  Initialized Ewald potential." << endl;
  }
  if (use_bond_pot) {
    bond_pot->EnergyInitialization(mols, box_l, this->npbc);
    cout << "  Initialized bond potential." << endl;
  }
  if (use_ext_pot) {
    ext_pot->EnergyInitialization(mols, box_l, this->npbc);
    cout << "  Initialized external potential." << endl; 
  }

  // Add beads to CG CBMC data structures. Proper IDs are not assigned at this
  // stage since the beads are not yet in the simulation.
  for (int i = 0; i < gc_chain_len; i++) {
    Bead bead(gc_bead_symbol, -1, -1, gc_bead_charge, 0, 0, 0);
    cbmc_chain.push_back(bead);
  }
  for (int i = 0; i < cbmc_no_of_trials; i++) {
    Bead bead(gc_bead_symbol, -1, -1, gc_bead_charge, 0, 0, 0);
    cbmc_trial_beads.push_back(bead);
  }
  // Add counterions into array if the chain is charged.
  if (gc_bead_charge != 0) {
    for (int i = 0; i < gc_chain_len; i++) {
      Bead bead(gc_bead_symbol, -1, -1, -gc_bead_charge, 0, 0, 0);
      cbmc_chain.push_back(bead);
    }
    for (int i = 0; i < cbmc_no_of_trials; i++) {
      Bead bead(gc_bead_symbol, -1, -1, -gc_bead_charge, 0, 0, 0);
      cbmc_trial_beads.push_back(bead);
    }
  }
  // Will be initialized in other functions before use.
  cbmc_trial_weights = new double[cbmc_no_of_trials];

}

double ForceField::EnergyDifference(vector<Molecule>& mols, int moved_mol) {
  double dE = 0;
  // In case we use hard potentials for pair_pot and ext_pot, we can return
  // energy early if there are collisions.
  if (use_pair_pot) {
    dE += pair_pot->EnergyDifference(mols, moved_mol, box_l, npbc);
    if (dE >= kVeryLargeEnergy) {
      return dE;
    }
  }
  if (use_ext_pot) {
    dE += ext_pot->EnergyDifference(mols, moved_mol, box_l, npbc);
    if (dE >= kVeryLargeEnergy) {
      return dE;
    }
  }

  // We do not use hard potentials for these.
  if (use_ewald_pot) {
    dE += ewald_pot->EnergyDifference(mols, moved_mol, npbc);
  }
  if (use_bond_pot) {
    dE += bond_pot->EnergyDifference(mols, box_l, npbc, moved_mol);
  }

  return dE;

}

void ForceField::FinalizeEnergies(vector<Molecule>& mols, bool accept,
                                  int moved_mol) {
  if (use_pair_pot) {
    pair_pot->FinalizeEnergyBothMaps(mols, moved_mol, accept);
  }
  if (use_ewald_pot) {
    ewald_pot->FinalizeEnergyBothMaps(mols, moved_mol, accept);
  }
  if (use_bond_pot) {
    bond_pot->UpdateEnergy(moved_mol, accept);
  }
  if (use_ext_pot) {
    ext_pot->FinalizeEnergyBothMaps(mols, moved_mol, accept);
  }

}

// !!! When using mols.size(), need to substract the phantoms out!
void ForceField::CalcPressureVirialHSEL(vector<Molecule>& mols, double rho) {
  // Use g(r) to calculate the electrostatic pressure component. If false, use
  // the direct derivative of the Ewald energy.
  bool use_g_el = false;
  // Use the analytical form of <dU/dV> to calculate pressure.
  bool use_d_el = true;
  if (!use_ewald_pot) {
    use_d_el = false;
    use_g_el = false;
  }

  //////////////////
  // Preparation. //
  //////////////////
  // If using GCMC, update the number of chains and ions as they vary.
  if (use_gc) {
    n_mol = (int)mols.size();
    n_chain = 0;
    n_cion = 0;
    n_aion = 0;
    for (int i = 0; i < n_mol; i++) {
      if (mols[i].Size() > 1) {
        n_chain++;
      }
      else if (mols[i].bds[0].Charge() >= 0) {
        n_cion++;
      }
      else if (mols[i].bds[0].Charge() < 0) {
        n_aion++;
      }
    }
  }

  ///////////////
  // Counting. //
  ///////////////
  // Partition function++.
  vp_z++;
  // For every bead in every chain, compare it with all other beads that are not
  // in the same chain.
  for (int k = 0; k < n_mol; k++) {
    int k_len = mols[k].Size();
    for (int i = 0; i < k_len; i++) {
      for (int l = 0; l < n_mol; l++) {
      int l_len = mols[l].Size();
        for (int j = 0; j < l_len; j++) {
          // If not the same moleucle.
          if (l != k) {
            int site1, site2;
            double z1 = mols[k].bds[i].Charge();
            double z2 = mols[l].bds[j].Charge();
            double C = 1.0/(double)(n_mol*n_mol);
            if (k_len > 1)     site1 = i;
            else if (z1 >= 0)  site1 = chain_len;
            else               site1 = chain_len + 1;
            if (l_len > 1)     site2 = j;
            else if (z2 >= 0)  site2 = chain_len;
            else               site2 = chain_len + 1;
            double vx  = mols[l].bds[j].BBDistVec(mols[k].bds[i], box_l,npbc,0);
            double vy  = mols[l].bds[j].BBDistVec(mols[k].bds[i], box_l,npbc,1);
            double vz  = mols[l].bds[j].BBDistVec(mols[k].bds[i], box_l,npbc,2);
            double vcx = mols[l].bds[0].BBDistVecWithRef(mols[k].bds[0],
                                mols[l].bds[j], mols[k].bds[i], box_l, npbc, 0);
            double vcy = mols[l].bds[0].BBDistVecWithRef(mols[k].bds[0],
                                mols[l].bds[j], mols[k].bds[i], box_l, npbc, 1);
            double vcz = mols[l].bds[0].BBDistVecWithRef(mols[k].bds[0],
                                mols[l].bds[j], mols[k].bds[i], box_l, npbc, 2);
            double vlen = sqrt(vx*vx + vy*vy + vz*vz);
            double dot_xx = vcx * vx / vlen;
            double dot_yy = vcy * vy / vlen;
            double dot_zz = vcz * vz / vlen;
            // Loop for g.
            /*
            if (vlen < vp_bead_size + vp_g_res*vp_g_bin) {
              int bin_id = (int)floor((vlen - vp_bead_size) / vp_g_res);
              if (bin_id < 0)  bin_id = 0;
              int ix = bin_id*3*vp_g_s + 0*vp_g_s + site1*(chain_len+2) + site2;
              int iy = bin_id*3*vp_g_s + 1*vp_g_s + site1*(chain_len+2) + site2;
              int iz = bin_id*3*vp_g_s + 2*vp_g_s + site1*(chain_len+2) + site2;
              vp_hs_g[ix] += 1.0/n_mol;
              vp_hs_g[iy] += 1.0/n_mol;
              vp_hs_g[iz] += 1.0/n_mol;
            }
            */
            // Loop for hard sphere interactions.
            if (vlen <= vp_bead_size + vp_hs_res*vp_hs_bin) {
              int bin_id = (int)floor((vlen - vp_bead_size) / vp_hs_res);
              if (bin_id < 0)  bin_id = 0;
              int ix = bin_id*3*vp_g_s + 0*vp_g_s + site1*(chain_len+2) + site2;
              int iy = bin_id*3*vp_g_s + 1*vp_g_s + site1*(chain_len+2) + site2;
              int iz = bin_id*3*vp_g_s + 2*vp_g_s + site1*(chain_len+2) + site2;
              vp_hs_rr[ix] += C * dot_xx;
              vp_hs_rr[iy] += C * dot_yy;
              vp_hs_rr[iz] += C * dot_zz;
            }
            // Loop for electrostatic interactions.
            if (use_g_el) {
              int bin_id = (int)floor(vlen / vp_el_res);
              int iall = bin_id*vp_g_s + site1*(chain_len+2) + site2;
              if (bin_id < vp_el_bin[0])  vp_el_rrx[iall] += C*z1*z2*dot_xx;
              if (bin_id < vp_el_bin[1])  vp_el_rry[iall] += C*z1*z2*dot_yy;
              if (bin_id < vp_el_bin[2])  vp_el_rrz[iall] += C*z1*z2*dot_zz;
            }
          }
        }
      }
    }
  }
  // For hard wall confining potential.
  if (use_ext_pot && ext_pot->PotentialName() == "HardWall") {
    for (int i = 0; i < n_mol; i++) {
      for (int j = 0; j < mols[i].Size(); j++) {
        istringstream iss(mols[i].bds[j].DistToWall(box_l));
        double dist_to_wall[2];
        iss >> dist_to_wall[0];  // Left.
        iss >> dist_to_wall[1];  // Right.
        // Warning! How many sites should be assigned to a hard wall???
        // Should we increase n_mol accordingly as well?
        // Currently, we have not changed n_mol.
        double C = 1.0/(double)(n_mol);
        // 1/r is already applied here.
        for (int k = 0; k < 2; k++) {
          if (dist_to_wall[k] <= vp_bead_size/2.0 + vp_hs_res*vp_hs_bin) {
            int bin_id = (int)floor((dist_to_wall[k] - vp_bead_size/2.0) /
                         vp_hs_res);
            vp_zwall_rr[bin_id] += C;
          }
        }
      }
    }
  }

  ////////////////////////
  // Adding up results. //
  ////////////////////////
  // g. ////////////////////////////
  /*
  double * tot_g = new double[vp_g_bin];      // !!! Put into class???
  for (int b = 0; b < vp_g_bin; b++) {        // Bin.
    tot_g[b] = 0;
    double C = 1.0/(3.0*chain_len*chain_len);
    for (int i = 0; i < chain_len+2; i++) {    // Site 1.
      for (int j = 0; j < chain_len+2; j++) {  // Site 2.
        tot_g[b] += C * vp_hs_g[b*3*vp_g_s + 0*vp_g_s + i*(chain_len+2) + j];
        tot_g[b] += C * vp_hs_g[b*3*vp_g_s + 1*vp_g_s + i*(chain_len+2) + j];
        tot_g[b] += C * vp_hs_g[b*3*vp_g_s + 2*vp_g_s + i*(chain_len+2) + j];
      }
    }
    double dis1 = (b+0.0)*vp_g_res + vp_bead_size;
    double dis2 = (b+1.0)*vp_g_res + vp_bead_size;
    double eps_vol = 4.0/3.0*kPi * (pow(dis2, 3) - pow(dis1, 3));
    tot_g[b] *= (1.0/(vp_z * eps_vol))/(n_mol/vol);
  }
  for (int j = 0; j < vp_hs_bin; j++) {
    vp_hs_g_lsy[j] = tot_g[j];
  }
  double g_sig = Interpolate(vp_hs_g_lsx, vp_hs_g_lsy, vp_hs_bin, vp_bead_size);
  cout << g_sig << endl;
  for (int b = 0; b < vp_g_bin; b++) {
    if ((b+1.0)*vp_g_res + vp_bead_size < 3)
      cout << tot_g[b] << endl;
  }
  cout << endl;
  delete [] tot_g;
  */
  // Hard sphere potential. ////////
  double * tot = new double[vp_hs_bin*3];      // !!! Put into class???
  for (int b = 0; b < vp_hs_bin; b++) {        // Bin.
    tot[b*3 + 0] = 0;
    tot[b*3 + 1] = 0;
    tot[b*3 + 2] = 0;
    double C = 1.0/((b+0.5)*vp_hs_res + vp_bead_size);
    for (int i = 0; i < chain_len+2; i++) {    // Site 1.
      for (int j = 0; j < chain_len+2; j++) {  // Site 2.
        // This is slightly different from Yethiraj's formula. As I am dividing
        // each term by an over-counting factor. That results in the canceling.
        tot[b*3+0] += C * vp_hs_rr[b*3*vp_g_s + 0*vp_g_s + i*(chain_len+2) + j];
        tot[b*3+1] += C * vp_hs_rr[b*3*vp_g_s + 1*vp_g_s + i*(chain_len+2) + j];
        tot[b*3+2] += C * vp_hs_rr[b*3*vp_g_s + 2*vp_g_s + i*(chain_len+2) + j];
      }
    }
    double dis1 = (b+0.0)*vp_hs_res + vp_bead_size;
    double dis2 = (b+1.0)*vp_hs_res + vp_bead_size;
    double eps_vol = 4.0/3.0*kPi * (pow(dis2, 3) - pow(dis1, 3));
    tot[b*3+0] /= (vp_z * eps_vol/vol);
    tot[b*3+1] /= (vp_z * eps_vol/vol);
    tot[b*3+2] /= (vp_z * eps_vol/vol);
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < vp_hs_bin; j++) {
      vp_hs_rr_lsy[j] = tot[j*3+i];
    }
    double f_sig = Interpolate(vp_hs_rr_lsx, vp_hs_rr_lsy, vp_hs_bin, vp_bead_size);
    p_tensor_hs[i] = 2*kPi/1.0 * rho * pow(vp_bead_size,3) * f_sig;
  }
  delete [] tot;
  // Hard wall. ////////////////////
  if (use_ext_pot && ext_pot->PotentialName() == "HardWall") {
    for (int b = 0; b < vp_hs_bin; b++)
      vp_hs_rr_lsy[b] = vp_zwall_rr[b] / (vp_z * vp_hs_res/box_l[2]);
    double f_sig = Interpolate(vp_hs_rr_lsx2, vp_hs_rr_lsy, vp_hs_bin, vp_bead_size/2.0);
    p_tensor_hs[2] += 2*kPi/1.0 * rho * pow(vp_bead_size,3) * f_sig;
  }
  // Electrostatics. ///////////////
  if (use_g_el) {
    double vp_el_rrx_tot = 0;
    double vp_el_rry_tot = 0;
    double vp_el_rrz_tot = 0;
    for (int i = 0; i < chain_len+2; i++) {       // Site 1.
      for (int j = 0; j < chain_len+2; j++) {     // Site 2.
        double C = 1.0;
        int c = i*(chain_len+2) + j;
        for (int b = 0; b < vp_el_bin[0]; b++) {
          double vol_eps = 4.0/3.0*kPi*(pow((b+1)*vp_el_res,3)-pow(b*vp_el_res,3));
          vp_el_rrx_tot += C*vp_el_res*vp_el_rrx[b*vp_g_s+c]/(vp_z * vol_eps/vol);
        }
        for (int b = 0; b < vp_el_bin[1]; b++) {
          double vol_eps = 4.0/3.0*kPi*(pow((b+1)*vp_el_res,3)-pow(b*vp_el_res,3));
          vp_el_rry_tot += C*vp_el_res * vp_el_rry[b*vp_g_s+c]/(vp_z * vol_eps/vol);
        }
        for (int b = 0; b < vp_el_bin[2]; b++) {
          double vol_eps = 4.0/3.0*kPi*(pow((b+1)*vp_el_res,3)-pow(b*vp_el_res,3));
          vp_el_rrz_tot += C*vp_el_res * vp_el_rrz[b*vp_g_s+c]/(vp_z * vol_eps/vol);
        }
      }
    }
    p_tensor_el[0] = 2*kPi/1.0 * lB * rho * vp_el_rrx_tot;
    p_tensor_el[1] = 2*kPi/1.0 * lB * rho * vp_el_rry_tot;
    p_tensor_el[2] = 2*kPi/1.0 * lB * rho * vp_el_rrz_tot;
  }
  else if (use_d_el) {
    double p_el = ewald_pot->PUPV(mols, vol, npbc);
    p_tensor_el_tot[0] += -p_el*beta/((double)n_mol/vol);
    p_tensor_el_tot[1] += -p_el*beta/((double)n_mol/vol);
    p_tensor_el_tot[2] += -p_el*beta/((double)n_mol/vol);
    p_tensor_el[0] = p_tensor_el_tot[0]/vp_z;
    p_tensor_el[1] = p_tensor_el_tot[1]/vp_z;
    p_tensor_el[2] = p_tensor_el_tot[2]/vp_z;
  }
  else {
    p_tensor_el[0] = 0;
    p_tensor_el[1] = 0;
    p_tensor_el[2] = 0;
  }
  // Total. ////////
  for (int i = 0; i < 3; i++) {
    p_tensor[i] = 1.0 + p_tensor_hs[i] + p_tensor_el[i];
  }

}

string ForceField::GetPressure() {
  std::ostringstream foo;
  foo << p_tensor[0]    << " " << p_tensor[1]    << " " << p_tensor[2]    << " "
      << p_tensor_hs[0] << " " << p_tensor_hs[1] << " " << p_tensor_hs[2] << " "
      << p_tensor_el[0] << " " << p_tensor_el[1] << " " << p_tensor_el[2];
  return foo.str();

}

double ForceField::BeadEnergy(Bead& bead, vector<Molecule>& mols,
                              int current_len, int delete_id, int type) {
  double energy = 0;
  int counterion = 0;
  if (gc_bead_charge != 0)  counterion = gc_chain_len;

  // Pairwise interactions for the bead with other molecules.
  for (int i = 0; i < (int)mols.size(); i++) {
    if (delete_id == -1 || (i < delete_id || i > delete_id + counterion)) {
      for (int j = 0; j < mols[i].Size(); j++) {
        if (use_pair_pot) {
          energy += pair_pot->PairEnergy(bead, mols[i].bds[j], box_l, npbc);
        }
      }
    }
  }
  // Loop over the beads in the current trial chain.
  for (int i = 0; i < current_len; i++) {
    if (i < current_len-1 || type == 1) {
      if (use_pair_pot) {
        energy += pair_pot->PairEnergy(bead, cbmc_chain[i], box_l, npbc);
      }
    }
  }
  // Last, interaction with the external potential.
  if (use_ext_pot) {
    energy += ext_pot->BeadEnergy(bead, box_l); 
  }

  return energy;
 
}

/*
 * Input
 * end_bead   : the previously chosen bead in the trial chain.
 * mols       : molecule array of the existing beads.
 * current_len: current length of the trial chain.
 * rand_gen   : random number generator.
 * delete_id  : used for chain deletion, the ID of the chain to be deleted.
 * type    : the identity of the bead to be added, 0 for chain, 1 for ion.
 */
double ForceField::CBMCGenTrialBeads(Bead& end_bead, vector<Molecule>& mols,
                                     int current_len, mt19937& rand_gen,
                                     int delete_id, int type) {
  double Wi = 0;

  for (int i = 0; i < cbmc_no_of_trials; i++) {
    int index = i;
    if (type == 1)  index += cbmc_no_of_trials;

    /////////////////////////
    // Decide bond length. //
    /////////////////////////
    double bond_len = 0;
    if (use_bond_pot) {
      bond_len = bond_pot->RandomBondLen(beta, rand_gen); 
    }
    else if (use_bond_rigid && type == 0) {
      bond_len = rigid_bond;
    }

    //////////////////////////////
    // Decide bead coordinates. //
    //////////////////////////////
    double bead_coord[3];
    // If chain, grow along chain.
    if (type == 0) {
      randSphere(bead_coord, rand_gen);
      for (int j = 0; j < 3; j++) {
        bead_coord[j] *= bond_len;
        bead_coord[j] += end_bead.GetCrd(0, j);
      }
    }
    // If counterion, grow randomly in box.
    else {
      bead_coord[0] = (double)rand_gen()/rand_gen.max() * box_l[0];
      bead_coord[1] = (double)rand_gen()/rand_gen.max() * box_l[1];
      bead_coord[2] = (double)rand_gen()/rand_gen.max() * box_l[2];
    }
    cbmc_trial_beads[index].SetAllCrd(bead_coord);

    //////////////////////////////
    // Calculating bead energy. //
    //////////////////////////////
    double bead_energy = BeadEnergy(cbmc_trial_beads[index], mols, current_len,
                                    delete_id, type);
    cbmc_trial_weights[i] = exp(-beta * bead_energy);    
    Wi += cbmc_trial_weights[i];
  }

  return Wi;

}

bool ForceField::CBMCChainInsertion(vector<Molecule>& mols, mt19937& rand_gen) {
  bool accept = false;
  double weight = 1.0;

  ///////////////////
  // Grow the 1st. //
  ///////////////////
  double xyz[3];
  for (int i = 0; i < 3; i++)
    xyz[i] = (double)rand_gen() / rand_gen.max() * box_l[i];
  cbmc_chain[0].SetAllCrd(xyz);
  weight *= exp(-beta * BeadEnergy(cbmc_chain[0], mols, 0, -1, 0));

  ////////////////////
  // Grow the rest. //
  ////////////////////
  int toadd = gc_chain_len;
  // If chain isn't neutral, also need to insert counterions.
  if (gc_bead_charge != 0)  toadd *= 2;
  int type = 0;  // 0 means chain, 1 means counterion; starts with chain.
  for (int i = 1; i < toadd && weight > 0; i++) {
    if (i >= gc_chain_len)  type = 1;
    double Wi = CBMCGenTrialBeads(cbmc_chain[i-1], mols, i, rand_gen, -1, type);
    weight *= Wi/cbmc_no_of_trials;
    double rand_num = (double)rand_gen() / rand_gen.max() * Wi;
    int current_bead = 0;
    double cumulate_weight = cbmc_trial_weights[0];
    while (cumulate_weight < rand_num) {
      current_bead++;
      cumulate_weight += cbmc_trial_weights[current_bead];
    }
    if (type == 1)  current_bead += cbmc_no_of_trials; 
    xyz[0] = cbmc_trial_beads[current_bead].GetCrd(0, 0);
    xyz[1] = cbmc_trial_beads[current_bead].GetCrd(0, 1);
    xyz[2] = cbmc_trial_beads[current_bead].GetCrd(0, 2);
    cbmc_chain[i].SetAllCrd(xyz);
  }

  ////////////////////////////////
  // Electrostatic energy diff. //
  ////////////////////////////////
  double dE = 0;
  if (use_ewald_pot)
    dE = ewald_pot->TrialChainEnergy(mols, cbmc_chain, toadd, -1, npbc);

  //////////////////////////////////
  // Decide acceptance/rejection. //
  //////////////////////////////////
  double rand_num = (double)rand_gen() / rand_gen.max(); 
  if (rand_num < (vol * exp(beta * chem_pot - beta*dE) * weight)
                 /(gc_deBroglie_prefactor * ((int)mols.size() + 1))) {
    accept = true; 
    mols.push_back(Molecule());
    for (int i = 0; i < gc_chain_len; i++) {
      mols[(int)mols.size() - 1].AddBead(cbmc_chain[i]); 
    }
    // Add bonds to the new chain ASSUMING LINEAR MOLECULE!
    for (int i = 0; i < mols[(int)mols.size() - 1].Size()-1; i++) {
      mols[(int)mols.size() - 1].AddBond(i, i+1); 
    }
    // Add counterions.
    for (int i = gc_chain_len; i < toadd; i++) {
      mols.push_back(Molecule());
      mols[(int)mols.size() - 1].AddBead(cbmc_chain[i]);
    }
  }

  // At this point, the new beads are added with the correct coordinates, but
  // they still need the proper IDs and energy initialization, also Simulation
  // nParticle fields need to be updated.
  return accept; 

}

int ForceField::CBMCChainDeletion(vector<Molecule>& mols, mt19937& rand_gen) {
  int delete_id = (double)rand_gen() / rand_gen.max() * (int)mols.size();
  if (delete_id == (int)mols.size())  delete_id--;
  while (mols[delete_id].Size() != gc_chain_len ||
         mols[delete_id].bds[0].Charge() != gc_bead_charge) {
    delete_id = (double)rand_gen() / rand_gen.max() * (int)mols.size();
    if (delete_id == (int)mols.size())  delete_id--;
  }

  ////////////////////////
  // "Grow" first bead. //
  ////////////////////////
  double weight = 1.0;
  // Weight for the first bead in chain.
  weight *= exp(-beta*BeadEnergy(mols[delete_id].bds[0], mols, 0, delete_id,0));
  // Replace the xyz bead by the existing bead on the chain to be deleted.
  double xyz[3] = {mols[delete_id].bds[0].GetCrd(0, 0),
                   mols[delete_id].bds[0].GetCrd(0, 1),
                   mols[delete_id].bds[0].GetCrd(0, 2)};
  cbmc_chain[0].SetAllCrd(xyz);

  ////////////////////
  // Grow the rest. //
  ////////////////////
  int toadd = gc_chain_len;
  if (gc_bead_charge != 0)  toadd *= 2;
  int type = 0;
  int shift_id = delete_id;
  for (int i = 1; i < toadd; i++) {
    if (i < gc_chain_len) {
      xyz[0] = mols[shift_id].bds[i].GetCrd(0, 0);
      xyz[1] = mols[shift_id].bds[i].GetCrd(0, 1);
      xyz[2] = mols[shift_id].bds[i].GetCrd(0, 2);
    }
    else {
      type = 1;
      shift_id = delete_id + i - gc_chain_len + 1; 
      xyz[0] = mols[shift_id].bds[0].GetCrd(0, 0);
      xyz[1] = mols[shift_id].bds[0].GetCrd(0, 1);
      xyz[2] = mols[shift_id].bds[0].GetCrd(0, 2);
    }
    cbmc_chain[i].SetAllCrd(xyz);

    CBMCGenTrialBeads(cbmc_chain[i-1], mols, i, rand_gen, delete_id, type);
    cbmc_trial_weights[0] = exp(-beta * BeadEnergy(cbmc_chain[i], mols, i,
                            delete_id, type));
    double Wi = 0;
    for(int j = 0; j < cbmc_no_of_trials; j++) {
      Wi += cbmc_trial_weights[j];
    }
    weight *= Wi/cbmc_no_of_trials;
  }

  ////////////////////////////////
  // Electrostatic energy diff. //
  ////////////////////////////////
  double dE = 0;
  if (use_ewald_pot)
    dE = ewald_pot->TrialChainEnergy(mols, cbmc_chain, toadd, delete_id, npbc);

  //////////////////////////////////
  // Decide acceptance/rejection. //
  //////////////////////////////////
  double rand_num = (double)rand_gen() / rand_gen.max();
  if (rand_num < ((int)mols.size() * gc_deBroglie_prefactor)
                 / (vol * exp(beta*chem_pot - beta*dE) * weight)) {
    if (use_pair_pot) {
      pair_pot->AdjustEnergyUponMolDeletion(mols, delete_id); 
    }
    if (use_ewald_pot) {
      ewald_pot->AdjustEnergyUponMolDeletion(mols, delete_id);
    }
    if (use_bond_pot) {
      bond_pot->AdjustEnergyUponMolDeletion(delete_id); 
    }
    if (use_ext_pot) {
      ext_pot->AdjustEnergyUponMolDeletion(mols, delete_id); 
    }
    return delete_id; 
  }

  // Signal unsuccessful deletion.
  return -1;

}

double ForceField::CalcChemicalPotential(vector<Molecule>& mols,
                                         mt19937& rand_gen) {
  double total = 0;
  for (int c = 0; c < mu_tot_ins; c++) {
    double weight = 1.0;
    double chosen[3];
    for (int i = 0; i < 3; i++) {
      chosen[i] = (double)rand_gen() / rand_gen.max() * box_l[i];
    }
    cbmc_chain[0].SetAllCrd(chosen);
    weight *= exp(-beta * BeadEnergy(cbmc_chain[0], mols, 0, -1, 0));

    for (int i = 1; i < gc_chain_len; i++) {
      double Wi = CBMCGenTrialBeads(cbmc_chain[i-1], mols, i, rand_gen, -1, 0);
      weight *= Wi/cbmc_no_of_trials;
      double rand_num = (double)rand_gen() / rand_gen.max() * Wi;
      int current_bead = 0;
      double cumulate_weight = cbmc_trial_weights[0];
      while (cumulate_weight < rand_num) {
        current_bead++; 
        cumulate_weight += cbmc_trial_weights[current_bead];
      }
      chosen[0] = cbmc_trial_beads[current_bead].GetCrd(0, 0);
      chosen[1] = cbmc_trial_beads[current_bead].GetCrd(0, 1);
      chosen[2] = cbmc_trial_beads[current_bead].GetCrd(0, 2);
      cbmc_chain[i].SetAllCrd(chosen);
    }

    total += (vol * weight) /
             (gc_deBroglie_prefactor * ((int)mols.size() + 1));
  }
  
  return total/(double)mu_tot_ins;

}

void ForceField::EnergyInitForAddedMolecule(vector<Molecule>& mols) {
  if (use_pair_pot) {
    pair_pot->EnergyInitForLastMol(mols, box_l, npbc);
  }
  if (use_ewald_pot) {
    ewald_pot->EnergyInitForLastMol(mols, gc_chain_len, gc_bead_charge,
                                    npbc);
  }
  if (use_bond_pot) {
    bond_pot->EnergyInitForLastMol(mols, box_l, npbc);
  }
  if (use_ext_pot) {
    ext_pot->EnergyInitForLastMol(mols, box_l, npbc);
  }

}

void ForceField::CoordinateObeyRigidBond(vector<Molecule>& mols) {
  // Maintain rigid bond length.
  for (int i = 0; i < (int)mols.size(); i++) {
    for (int j = 0; j< mols[i].Size()-1; j++) {
      double x1 = mols[i].bds[j].GetCrd(0, 0);
      double y1 = mols[i].bds[j].GetCrd(0, 1);
      double z1 = mols[i].bds[j].GetCrd(0, 2);
      double incr = 0;
      while (pair_pot->PairEnergy(mols[i].bds[j], mols[i].bds[j+1],
             box_l, npbc) >= kVeryLargeEnergy) {
        double x2 = mols[i].bds[j+1].GetCrd(0, 0);
        double y2 = mols[i].bds[j+1].GetCrd(0, 1);
        double z2 = mols[i].bds[j+1].GetCrd(0, 2);
        double dx = x2-x1;
        double dy = y2-y1;
        double dz = z2-z1;
        double dist = sqrt(dx*dx + dy*dy + dz*dz);
        x2 = x1 + (rigid_bond+incr) * dx / dist;
        y2 = y1 + (rigid_bond+incr) * dy / dist;
        z2 = z1 + (rigid_bond+incr) * dz / dist;
        mols[i].bds[j+1].SetCrd(0, 0, x2);
        mols[i].bds[j+1].SetCrd(1, 0, x2);
        mols[i].bds[j+1].SetCrd(0, 1, y2);
        mols[i].bds[j+1].SetCrd(1, 1, y2);
        mols[i].bds[j+1].SetCrd(0, 2, z2);
        mols[i].bds[j+1].SetCrd(1, 2, z2);
        incr += kMedSmallNumber;
      }
    }
  }
}

int ForceField::GCFrequency() {
  return gc_freq; 

}

bool ForceField::UseGC() {
  return use_gc;

}

bool ForceField::UsePairPot() {
  return use_pair_pot;

}

bool ForceField::UseEwaldPot() {
  return use_ewald_pot;
}

bool ForceField::UseBondPot() {
  return use_bond_pot;

}

bool ForceField::UseBondRigid() {
  return use_bond_rigid;

}

bool ForceField::UseAnglePot() {
  return use_angle_pot;

}

bool ForceField::UseDihedPot() {
  return use_dihed_pot;

}

bool ForceField::UseExtPot() {
  return use_ext_pot;

}

double ForceField::TotPairEnergy() {
  return pair_pot->GetTotalEnergy();

}

double ForceField::TotEwaldEnergy() {
  return ewald_pot->GetTotalEnergy();

}

double ForceField::TotBondEnergy() {
  return bond_pot->GetTotalEnergy();

}

double ForceField::TotExtEnergy() {
  return ext_pot->GetTotalEnergy();

}

double ForceField::CalculateExternalForce(vector<Molecule>& mols) {
  return ext_pot->CalculateForce(mols, box_l);

}

double ForceField::RigidBondLen() {
  return rigid_bond;

}

void ForceField::GetEwaldEnergyComponents(vector<Molecule>& mols, double *out) {
  ewald_pot->UpdateEnergyComponents(mols, npbc);
  out[0] = ewald_pot->GetRealEnergy();
  out[1] = ewald_pot->GetReplEnergy();
  out[2] = ewald_pot->GetSelfEnergy();

}

void ForceField::SetBoxLen(double box_l_in[3]) {
  box_l[0] = box_l_in[0];
  box_l[1] = box_l_in[1];
  box_l[2] = box_l_in[2];

}

