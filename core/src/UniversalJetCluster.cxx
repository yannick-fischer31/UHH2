#include "UHH2/core/include/UniversalJetCluster.h"

using namespace std;
using namespace uhh2;
using namespace fastjet;
using namespace contrib;


UniversalJetCluster::UniversalJetCluster(vector<PFParticle> *pfparticles)
{
  for(unsigned int i = 0; i < pfparticles->size(); ++i) 
    {
      _psj.push_back(ConvertPFToPsj(&(pfparticles->at(i))));
    }
  ClusterHOTVR();
  ClusterXCone33();
}

// ---------------------------------------------------------------
// TopJets

// HOTVR 

// NOTE: The error seems to be, that HOTVR unfortunately cannot handle
// ClusterSequenceArea. With only ClusterSequence erverything seems to
// be just fine.
void UniversalJetCluster::ClusterHOTVR()
{
  double mu(30.),                 // massjump threshold
    theta(0.7),                   // massjump parameter
    max_r(1.5),                   // maximum allowed distance R
    min_r(0.1),                   // minimum allowed distance R
    rho(600.),                    // cone shrinking parameter
    hotvr_pt_min(30.);            // minimum pT of subjet

  // jet definition with HOTVR plugin
  HOTVR hotvr_plugin(mu, theta, min_r, max_r, rho, hotvr_pt_min, HOTVR::CALIKE); 
  JetDefinition jet_def(&hotvr_plugin);

  // area definition
  // double ghost_maxrap = 5.0;      // maxiumum y of ghost particles
  // AreaDefinition area_def(active_area, GhostedAreaSpec(ghost_maxrap));
 
  // setup cluster sequence with area
  // ClusterSequenceArea cs(_psj, jet_def, area_def);

  // NOTE: There is a problem when getting Nsubjettiness directly
  // from HOTVR jets, because the link to the cluster sequence got lost
  // somehow. Thus the approach here will be to identify the Jets from
  // the cluster sequence with the hotvr_jets by comparing their
  // four-vectors.

  // vector<PseudoJet> cs_jets = cs.inclusive_jets(hotvr_pt_min);
  // vector<PseudoJet> hotvr_cs_jets;
  // for (unsigned int i = 0; i < hotvr_jets.size(); ++i)
  //   {
  //     double hotvreta = hotvr_jets[i].eta();
  //     double hotvrphi = hotvr_jets[i].phi();
  //     double hotvrpt = hotvr_jets[i].pt();
  //     double hotvre = hotvr_jets[i].E();
  //     for (unsigned int j = 0; j < cs_jets.size(); ++j)
  // 	{
  // 	  double cseta = cs_jets[j].eta();
  // 	  double csphi = cs_jets[j].phi();
  // 	  double cspt = cs_jets[j].pt();
  // 	  double cse = cs_jets[j].E();
  // 	  if ((hotvreta == cseta) && (hotvrphi == csphi) && (hotvrpt == cspt) && (hotvre == cse))
  // 	    {
  // 	      hotvr_cs_jets.push_back(cs_jets[j]);
  // 	      break;
  // 	    }
  // 	}
  //   }
  // cout << "#HOTVR Jets == # new Jets? " << (hotvr_jets.size() == hotvr_cs_jets.size()) << endl;
  // cout << "new Jets have valid cs? " << hotvr_cs_jets[0].has_valid_cluster_sequence() << endl;
  // cout << "new Jets has pieces? " << hotvr_cs_jets[0].has_pieces() << endl;

  // Setup cluster sequence
  ClusterSequence cs(_psj, jet_def);
  vector<PseudoJet> hotvr_jets = hotvr_plugin.get_jets();	 
 
  for (unsigned int i = 0; i < hotvr_jets.size(); ++i)
    {
      double beta = 1.0;
      HOTVRinfo hi = hotvr_jets[i].user_info<HOTVRinfo>();
      vector<PseudoJet> subjets = hi.subjets();
      Nsubjettiness nSub1(1,   OnePass_WTA_KT_Axes(), NormalizedMeasure(beta, hi.max_distance()));
      Nsubjettiness nSub2(2,   OnePass_WTA_KT_Axes(), NormalizedMeasure(beta, hi.max_distance()));
      Nsubjettiness nSub3(3,   OnePass_WTA_KT_Axes(), NormalizedMeasure(beta, hi.max_distance()));
      double tau1 =  nSub1(hotvr_jets[i]);
      double tau2 =  nSub2(hotvr_jets[i]);
      double tau3 =  nSub3(hotvr_jets[i]);

      // gettin jet and subjet area
      // double jet_area = hotvr_cs_jets[i].area();
      // vector<double> subjet_area;
      // for (unsigned int j = 0; j < subjets2.size(); ++j) subjet_area.push_back(subjets2[j].area());
      _hotvrTopJets.push_back(ConvertPsjToTopJet(hotvr_jets[i], subjets, tau1, tau2, tau3));
    }
}
vector<TopJet> UniversalJetCluster::GetHOTVRTopJets()
{
  return _hotvrTopJets;
}

// XCone
void UniversalJetCluster::ClusterXCone33()
{
  // Run first clustering step (N=2, R=1.2) 
  vector<PseudoJet> fatjets;
  XConePlugin plugin_xcone(2, 1.2, 2.0);
  double ghost_maxrap = 5.0;      // maxiumum y of ghost particles
  AreaDefinition area_def(active_area, GhostedAreaSpec(ghost_maxrap));
  JetDefinition jet_def_xcone(&plugin_xcone);
  ClusterSequenceArea clust_seq_xcone(_psj, jet_def_xcone, area_def);
  fatjets = sorted_by_pt(clust_seq_xcone.inclusive_jets(0));
  ////

  // get SoftDrop Mass
  SoftDrop sd(0.0, 0.1, 1.2);
  PseudoJet sdjet1 = sd(fatjets[0]);
  PseudoJet sdjet2 = sd(fatjets[1]);
  float sd_mass1 = sdjet1.m();
  float sd_mass2 = sdjet2.m();
  ////

  // get and wirte list: if particle i ist clustered in jet j, the i-th entry of the list == j
  vector<int> list_fat;
  list_fat.clear();
  list_fat = clust_seq_xcone.particle_jet_indices(fatjets);
  vector<PseudoJet> particle_in_fat1, particle_in_fat2;

  // get one set of particles for each jet
  for (unsigned int ipart=0; ipart < _psj.size(); ++ipart){
    if (list_fat[ipart]==0){
      particle_in_fat1.push_back(_psj.at(ipart)); // get vector as PseudoJet for fatjet 1
    }
    if (list_fat[ipart]==1){
      particle_in_fat2.push_back(_psj.at(ipart)); // get vector as PseudoJet for fatjet 2
   }
  }
  ////

  // Run second clustering step (N=3, R=0.4) for each fat jet
  vector<PseudoJet> subjets_1, subjets_2;

  // subjets from fat jet 1 
  XConePlugin plugin_xcone_sub1(3, 0.4, 2.0);
  JetDefinition jet_def_sub1(&plugin_xcone_sub1);
  ClusterSequenceArea clust_seq_sub1(particle_in_fat1, jet_def_sub1, area_def);
  subjets_1 = sorted_by_pt(clust_seq_sub1.inclusive_jets(0));

  // subjets from fat jet 2 
  XConePlugin plugin_xcone_sub2(3, 0.4, 2.0);
  JetDefinition jet_def_sub2(&plugin_xcone_sub2);
  ClusterSequenceArea clust_seq_sub2(particle_in_fat2, jet_def_sub2, area_def);
  subjets_2 = sorted_by_pt(clust_seq_sub2.inclusive_jets(0));
  ////

  // set TopJets with subjets and JetArea
  double jet1_area = fatjets[0].area();
  double jet2_area = fatjets[1].area();
  vector<double> subjet1_area;
  vector<double> subjet2_area;
  for (unsigned int j = 0; j < subjets_1.size(); ++j) subjet1_area.push_back(subjets_1[j].area());
  for (unsigned int k = 0; k < subjets_2.size(); ++k) subjet2_area.push_back(subjets_2[k].area());

  _xcone33TopJets.push_back(ConvertPsjToTopJet(fatjets[0], subjets_1, jet1_area, subjet1_area, sd_mass1));
  _xcone33TopJets.push_back(ConvertPsjToTopJet(fatjets[1], subjets_2, jet2_area, subjet2_area, sd_mass2));
  ////


  // delete pseudojets and lists
  subjets_1.clear();
  subjets_2.clear();
  fatjets.clear();
  particle_in_fat1.clear();
  particle_in_fat2.clear();
  ////
}
vector<TopJet> UniversalJetCluster::GetXCone33Jets()
{
  return _xcone33TopJets;
}


// ---------------------------------------------------------------
// Converters

// Convert PFParticle to PseudoJet
PseudoJet UniversalJetCluster::ConvertPFToPsj(PFParticle * pfp)
{
  PseudoJet psj(pfp->v4().X(), pfp->v4().Y(), pfp->v4().Z(), pfp->v4().T());
  return psj;
}

// Convert to Jets with Area
Jet UniversalJetCluster::ConvertPsjToJet(const PseudoJet & psj, double jet_area)
{
  Jet jet;
  jet.set_pt(psj.pt());
  jet.set_eta(psj.eta());
  jet.set_phi(psj.phi());
  jet.set_energy(psj.E());
  jet.set_jetArea(jet_area);
  return jet;
}

// Convert to Jets
Jet UniversalJetCluster::ConvertPsjToJet(const PseudoJet & psj)
{
  Jet jet;
  jet.set_pt(psj.pt());
  jet.set_eta(psj.eta());
  jet.set_phi(psj.phi());
  jet.set_energy(psj.E());
  return jet;
}

// Convert to TopJets with groomed Nsubjettiness
TopJet UniversalJetCluster::ConvertPsjToTopJet(const PseudoJet & psj, const vector<PseudoJet> &subpsj, double tau1, double tau2, double tau3)
{
  TopJet topjet;
  topjet.set_pt(psj.pt());
  topjet.set_eta(psj.eta());
  topjet.set_phi(psj.phi());
  topjet.set_energy(psj.E());
  topjet.set_tau1_groomed(tau1);
  topjet.set_tau2_groomed(tau2);
  topjet.set_tau3_groomed(tau3);
  for (unsigned int i = 0; i < subpsj.size(); ++i) 
    {
      topjet.add_subjet(ConvertPsjToJet(subpsj[i]));
    }
  return topjet;
}

// Convert to Topjets with Area and softdrop mass
TopJet UniversalJetCluster::ConvertPsjToTopJet(const PseudoJet & psj, const vector<PseudoJet> &subpsj, double jet_area, vector<double> subjet_area, float sd_mass)
{
  TopJet topjet;
  topjet.set_pt(psj.pt());
  topjet.set_eta(psj.eta());
  topjet.set_phi(psj.phi());
  topjet.set_energy(psj.E());
  topjet.set_jetArea(jet_area);
  topjet.set_softdropmass(sd_mass);

  for (unsigned int i = 0; i < subpsj.size(); ++i) 
    {
      topjet.add_subjet(ConvertPsjToJet(subpsj[i], subjet_area[i]));
    }
  return topjet;
}

