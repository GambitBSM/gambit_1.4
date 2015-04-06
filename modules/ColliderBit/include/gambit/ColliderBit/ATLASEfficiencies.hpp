#pragma once

// Functions that do super fast detector simulation based on four vector smearing
// Note that the Delphes efficiency functions will eventually be replaced by paramterisations of the ATLAS reconstruction efficiencies
// Written by Martin White, January 2015, martin.white@adelaide.edu.au

#include <random>
#include "gambit/ColliderBit/Utils.hpp"
#include "HEPUtils/MathUtils.h"
#include "HEPUtils/BinnedFn.h"
#include "HEPUtils/Event.h"

namespace Gambit {
  namespace ColliderBit {


    /// @name ATLAS detector efficiency functions
    //@{


    /// @todo Make binned fns static to avoid re-creating


    inline void applyDelphesElectronTrackingEff(std::vector<Particle*>& electrons) {
      // Function that mimics the DELPHES electron tracking efficiency

      HEPUtils::BinnedFn2D<double> _elTrackEff2d({{0,1.5,2.5,10.}}, {{0,0.1,1.0,100,10000}},
                                                 {{0., 0.73, 0.95, 0.99,
                                                   0., 0.5,  0.83, 0.90,
                                                   0., 0.,   0.,   0.}});

      // Loop over the electrons and only keep those that pass the random efficiency cut
      /// @todo Replace by filter_if + lambda
      std::vector<Particle*> survivors;
      for (Particle* e : electrons) {
        // std::cout << e->abseta() << " " << e->pT() << std::endl;
        if (random_bool(_elTrackEff2d, e->abseta(), e->pT())) survivors.push_back(e); else delete e;
      }
      electrons = survivors;
    }


    inline void applyDelphesElectronEff(std::vector<Particle*>& electrons) {
      // Function that mimics the DELPHES electron efficiency
      // Should be applied after the electron energy smearing

      HEPUtils::BinnedFn2D<double> _elEff2d({{0,1.5,2.5,10.}}, {{0,10.,10000.}},
                                            {{0., 0.95, 0., 0.85, 0.,0.}});

      // Now loop over the electrons and only keep those that pass the random efficiency cut
      /// @todo Replace by filter_if + lambda
      std::vector<Particle*> survivors;
      for (Particle* e : electrons) {
        if (random_bool(_elEff2d, e->abseta(), e->pT())) survivors.push_back(e); else delete e;
      }
      electrons = survivors;
    }


    inline void applyDelphesMuonTrackEff(std::vector<Particle*>& muons) {

      HEPUtils::BinnedFn2D<double> _muTrackEff2d({{0,1.5,2.5,10.}}, {{0,0.1,1.0,10000.}},
                                                 {{0., 0.75, 0.99,
                                                       0.,0.70,0.98,
                                                       0.,0.,0.}});

      // Now loop over the muons and only keep those that pass the random efficiency cut
      /// @todo Replace by filter_if + lambda
      std::vector<Particle*> survivors;
      for (Particle* mu : muons) {
        // std::cout << "MUON PT " << mu_pt << " ETA " << mu_eta << " trackeff " << _muTrackEff2d.get_at(fabs(mu_eta), fabs(mu_pt)) << std::endl;
        if (random_bool(_muTrackEff2d, mu->abseta(), mu->pT())) survivors.push_back(mu); else delete mu;
      }
      muons = survivors;
    }


    inline void applyDelphesMuonEff(std::vector<Particle*>& muons) {

      HEPUtils::BinnedFn2D<double> _muEff2d({{0,1.5,2.7,10.}}, {{0,10.0,10000.}},
                                            {{0., 0.95, 0.,0.85,0.,0.}});

      // Now loop over the muons and only keep those that pass the random efficiency cut
      /// @todo Replace by filter_if + lambda
      std::vector<Particle*> survivors;
      for (Particle* mu : muons) {
        //std::cout << "MUON PT " << mu_pt << " ETA " << mu_eta << " eff " << _muEff2d.get_at(fabs(mu_eta), fabs(mu_pt)) << std::endl;
        if (random_bool(_muEff2d, mu->abseta(), mu->pT())) survivors.push_back(mu); else delete mu;
      }
      muons = survivors;
    }


    inline void applyTauEfficiency(std::vector<Particle*>& taus) {

      HEPUtils::BinnedFn2D<double> _tauEff2d({{0,10.}}, {{0,10000.}},
                                             {{0.4}});

      /// @todo Replace by filter_if + lambda
      std::vector<Particle*> survivors;
      for (Particle* tau : taus) {
        if (random_bool(_tauEff2d, tau->abseta(), tau->pT())) survivors.push_back(tau); else delete tau;
      }
      taus = survivors;
    }


    inline void smearElectronEnergy(std::vector<Particle*>& electrons) {
      // Function that mimics the DELPHES electron energy resolution
      // We need to smear E, then recalculate pT, then reset 4 vector

      std::random_device rd;
      std::mt19937 gen(rd());

      HEPUtils::BinnedFn2D<double> coeffE2({{0,2.5,3.,5.}}, {{0,0.1,25.,10000.}},
                                           {{0.,0.015*0.015,0.005*0.005,
                                             0.005*0.005,0.005*0.005,0.005*0.005,
                                             0.107*0.107,0.107*0.107,0.107*0.107}});

      HEPUtils::BinnedFn2D<double> coeffE({{0,2.5,3.,5.}}, {{0,0.1,25.,10000.}},
                                          {{0.,0.,0.05*0.05,
                                            0.05*0.05,0.05*0.05,0.05*0.05,
                                            2.08*2.08,2.08*2.08,2.08*2.08}});

      HEPUtils::BinnedFn2D<double> coeffC({{0,2.5,3.,5.}}, {{0,0.1,25.,10000.}},
                                          {{0.,0.,0.25*0.25,
                                            0.25*0.25,0.25*0.25,0.25*0.25,
                                            0.,0.,0.}});

      // Now loop over the electrons and smear the 4-vectors
      for (Particle* e : electrons) {

        // Look up / calculate resolution
        const double c1 = coeffE2.get_at(e->abseta(), e->pT());
        const double c2 = coeffE.get_at(e->abseta(), e->pT());
        const double c3 = coeffC.get_at(e->abseta(), e->pT());
        const double resolution = sqrt(c1*sqr(e->E()) + c2*e->E() + c3);

        // Smear by a Gaussian centered on the current energy, with width given by the resolution
        std::normal_distribution<> d(e->E(), resolution);
        double smeared_E = d(gen);
        if (smeared_E < 0) smeared_E = 0;
        // double smeared_pt = smeared_E/cosh(e->eta()); ///< @todo Should be cosh(|eta|)?
        // std::cout << "BEFORE eta " << electron->eta() << std::endl;
        e->set_mom(P4::mkEtaPhiME(e->eta(), e->phi(), e->mass(), smeared_E));
        // std::cout << "AFTER eta " << electron->eta() << std::endl;
      }
    }


    inline void smearMuonMomentum(std::vector<Particle*>& muons) {
      // Function that mimics the DELPHES muon momentum resolution
      // We need to smear pT, then recalculate E, then reset 4 vector

      std::random_device rd;
      std::mt19937 gen(rd());

      HEPUtils::BinnedFn2D<double> _muEff({{0,1.5,2.5}}, {{0,0.1,1.,10.,200.,10000.}},
                                          {{0.,0.03,0.02,0.03,0.05,
                                            0.,0.04,0.03,0.04,0.05}});

      // Now loop over the muons and smear the 4-vectors
      for (Particle* mu : muons) {
        // Look up resolution
        const double resolution = _muEff.get_at(mu->abseta(), mu->pT());

        // Smear by a Gaussian centered on the current energy, with width given by the resolution
        std::normal_distribution<> d(mu->pT(), resolution*mu->pT());
        double smeared_pt = d(gen);
        if (smeared_pt < 0) smeared_pt = 0;
        // const double smeared_E = smeared_pt*cosh(mu->eta()); ///< @todo Should be cosh(|eta|)?
        // std::cout << "Muon pt " << mu_pt << " smeared " << smeared_pt << endl;
        mu->set_mom(P4::mkEtaPhiMPt(mu->eta(), mu->phi(), mu->mass(), smeared_pt));
      }
    }


    inline void smearJets(std::vector<HEPUtils::Jet*>& jets) {
      // Function that mimics the DELPHES muon momentum resolution
      // We need to smear pT, then recalculate E, then reset 4 vector

      std::random_device rd;
      std::mt19937 gen(rd());

      HEPUtils::BinnedFn2D<double> _jetRes({{0,10.}}, {{0,10000.}},
                                           {{0.03}});

      // Now loop over the jets and smear the 4-vectors
      for (HEPUtils::Jet* jet : jets) {
        // Look up resolution
        double resolution = _jetRes.get_at(jet->abseta(), jet->pT());

        // Smear by a Gaussian centered on 1 with width given by the (fractional) resolution
        std::normal_distribution<> d(1.,resolution);
        double smear_factor = d(gen);
        jet->set_mom(P4::mkXYZM(jet->mom().px()*smear_factor, jet->mom().py()*smear_factor, jet->mom().pz()*smear_factor, jet->mass()));
      }
    }

    inline void smearTaus(std::vector<HEPUtils::Particle*>& taus) {
      // Function that applies jet energy scale to hadronic taus
      // We need to smear pT, then recalculate E, then reset 4 vector
      // Basically the same function as above, except it takes a vector of particles

      std::random_device rd;
      std::mt19937 gen(rd());

      HEPUtils::BinnedFn2D<double> _jetRes({{0,10.}}, {{0,10000.}},
                                           {{0.03}});

      // Now loop over the taus and smear the 4-vectors
      for (HEPUtils::Particle* tau : taus) {
        // Look up resolution
        double resolution = _jetRes.get_at(tau->abseta(), tau->pT());

        // Smear by a Gaussian centered on 1 with width given by the (fractional) resolution
        std::normal_distribution<> d(1.,resolution);
        double smear_factor = d(gen);
        tau->set_mom(P4::mkXYZM(tau->mom().px()*smear_factor, tau->mom().py()*smear_factor, tau->mom().pz()*smear_factor, tau->mass()));
      }
    }


    /// Efficiency function for Medium ID electrons
    ///
    /// Numbers digitised from 8 TeV note (ATLAS-CONF-2014-032)
    inline void applyMediumIDElectronSelection(std::vector<Particle*>& electrons) {

      /// @todo Avoid re-building these big binned functions every time this is called: use statics and initialise the BinnedFn1Ds only once. C++11 would help...

      const double binedges_E10_15_list[] = {0., 0.0494681, 0.453578, 1.10675, 1.46298, 1.78963, 2.2766, 2.5};
      const std::vector<double>  binedges_E10_15(binedges_E10_15_list, binedges_E10_15_list+8);
      const double binedgevalues_E10_15_list[] = {0.730435, 0.730435, 0.782609, 0.776812, 0.765217, 0.773913, 0.77971, 0.776812};
      const std::vector<double>  binedgevalues_E10_15(binedgevalues_E10_15_list, binedgevalues_E10_15_list+8);
      const std::vector<double> binvalues_E10_15 = makeBinValues(binedgevalues_E10_15);
      const BinnedFn1D<double> _eff_E10_15(binedges_E10_15, binvalues_E10_15);

      const double binedges_E15_20_list[] = {0., 0.0533175, 0.456161, 1.1019, 1.46327, 1.78318, 2.26303, 2.5};
      const std::vector<double>  binedges_E15_20(binedges_E15_20_list, binedges_E15_20_list+8);
      const double binedgevalues_E15_20_list[] = {0.77971, 0.77971, 0.82029, 0.817391, 0.701449, 0.797101, 0.828986, 0.828986};
      const std::vector<double>  binedgevalues_E15_20(binedgevalues_E15_20_list, binedgevalues_E15_20_list+8);
      const std::vector<double> binvalues_E15_20 = makeBinValues(binedgevalues_E15_20);
      const BinnedFn1D<double> _eff_E15_20(binedges_E15_20, binvalues_E15_20);

      const double binedges_E20_25_list[] =  {-2.5, -2.45249, -2.21496, -1.94181, -1.6924, -1.46675, -1.26485, -0.991686, -0.682898, -0.350356, -0.0415677, 0.0653207, 0.362233, 0.718527, 0.97981, 1.2886, 1.45487, 1.68052, 1.94181, 2.23872, 2.45249, 2.5};
      const std::vector<double>  binedges_E20_25(binedges_E20_25_list, binedges_E20_25_list+22);
      const double binedgevalues_E20_25_list[] =  {0.827811, 0.82572, 0.790414, 0.798585, 0.779843, 0.727974, 0.802447, 0.798192, 0.812561, 0.812423, 0.808153, 0.779115, 0.822483, 0.816123, 0.795304, 0.793105, 0.772326, 0.778446, 0.794906, 0.78857, 0.821617, 0.821593};
      const std::vector<double>  binedgevalues_E20_25(binedgevalues_E20_25_list, binedgevalues_E20_25_list+22);
      const std::vector<double> binvalues_E20_25 = makeBinValues(binedgevalues_E20_25);
      const BinnedFn1D<double> _eff_E20_25(binedges_E20_25, binvalues_E20_25);

      const double binedges_E25_30_list[] = {-2.5, -2.45249, -2.22684, -1.92993, -1.6924, -1.46675, -1.26485, -0.97981, -0.694774, -0.33848, -0.0534442, 0.0771971, 0.33848, 0.74228, 1.00356, 1.26485, 1.46675, 1.6924, 1.94181, 2.22684, 2.45249, 2.5};
      const std::vector<double>  binedges_E25_30(binedges_E25_30_list, binedges_E25_30_list+22);
      const double binedgevalues_E25_30_list[] = {0.84095, 0.838892, 0.8286, 0.830801, 0.818436, 0.76037, 0.841463, 0.83535, 0.850008, 0.852233, 0.837812, 0.82748, 0.854592, 0.854759, 0.838251, 0.844591, 0.76782, 0.815688, 0.836563, 0.824219, 0.838853, 0.838877};
      const std::vector<double>  binedgevalues_E25_30(binedgevalues_E25_30_list, binedgevalues_E25_30_list+22);
      const std::vector<double> binvalues_E25_30 = makeBinValues(binedgevalues_E25_30);
      const BinnedFn1D<double> _eff_E25_30(binedges_E25_30, binvalues_E25_30);

      const double binedges_E30_35_list[] = {-2.5, -2.44062, -2.21496, -1.92993, -1.68052, -1.46675, -1.27672, -0.991686, -0.706651, -0.350356, -0.0534442, 0.0771971, 0.350356, 0.706651, 0.97981, 1.2886, 1.47862, 1.68052, 1.94181, 2.23872, 2.44062, 2.5};
      const std::vector<double>  binedges_E30_35(binedges_E30_35_list, binedges_E30_35_list+22);
      const double binedgevalues_E30_35_list[] = {0.849263, 0.849233, 0.840831, 0.853176, 0.844763, 0.771974, 0.873676, 0.865249, 0.877593, 0.883677, 0.869013, 0.856496, 0.879231, 0.883238, 0.870661, 0.870533, 0.779059, 0.839213, 0.84949, 0.834827, 0.834743, 0.834718};
      const std::vector<double>  binedgevalues_E30_35(binedgevalues_E30_35_list, binedgevalues_E30_35_list+22);
      const std::vector<double> binvalues_E30_35 = makeBinValues(binedgevalues_E30_35);
      const BinnedFn1D<double> _eff_E30_35(binedges_E30_35, binvalues_E30_35);

      const double binedges_E35_40_list[] = {-2.5, -2.44841, -2.23431, -1.94914, -1.69969, -1.46336, -1.28359, -0.998664, -0.713488, -0.357087, -0.0723338, 0.0580256, 0.343519, 0.699955, 1.0085, 1.26989, 1.45836, 1.685, 1.93451, 2.23118, 2.46818, 2.5};
      const std::vector<double>  binedges_E35_40(binedges_E35_40_list, binedges_E35_40_list+22);
      const double binedgevalues_E35_40_list[] = {0.836795, 0.84095, 0.859644, 0.867953, 0.87003, 0.799407, 0.894955, 0.888724, 0.897033, 0.903264, 0.886647, 0.87003, 0.897033, 0.905341, 0.890801, 0.897033, 0.805638, 0.863798, 0.87003, 0.85549, 0.824332, 0.826409};
      const std::vector<double>  binedgevalues_E35_40(binedgevalues_E35_40_list, binedgevalues_E35_40_list+22);
      const std::vector<double> binvalues_E35_40 = makeBinValues(binedgevalues_E35_40);
      const BinnedFn1D<double> _eff_E35_40(binedges_E35_40, binvalues_E35_40);

      const double binedges_E40_45_list[] = {-2.5, -2.45261, -2.22749, -1.93128, -1.68246, -1.46919, -1.27962, -0.995261, -0.7109, -0.343602, -0.0592417, 0.0473934, 0.35545, 0.699052, 0.983412, 1.27962, 1.4455, 1.69431, 1.94313, 2.22749, 2.44076, 2.5};
      const std::vector<double>  binedges_E40_45(binedges_E40_45_list, binedges_E40_45_list+22);
      const double binedgevalues_E40_45_list[] = {0.836795, 0.836795, 0.87003, 0.882493, 0.897033, 0.84095, 0.911573, 0.89911, 0.907418, 0.909496, 0.89911, 0.888724, 0.907418, 0.91365, 0.89911, 0.907418, 0.843027, 0.890801, 0.882493, 0.87003, 0.816024, 0.816024};
      const std::vector<double>  binedgevalues_E40_45(binedgevalues_E40_45_list, binedgevalues_E40_45_list+22);
      const std::vector<double> binvalues_E40_45 = makeBinValues(binedgevalues_E40_45);
      const BinnedFn1D<double> _eff_E40_45(binedges_E40_45, binvalues_E40_45);

      const double binedges_E45_50_list[] = {-2.5, -2.46086, -2.22192, -1.93675, -1.68709, -1.46211, -1.27124, -0.986416, -0.689328, -0.356822, -0.0482438, 0.0584337, 0.355838, 0.712203, 0.996992, 1.28217, 1.45947, 1.68576, 1.93499, 2.21988, 2.44378, 2.5};
      const std::vector<double>  binedges_E45_50(binedges_E45_50_list, binedges_E45_50_list+22);
      const double binedgevalues_E45_50_list[] = {0.807101, 0.807101, 0.889941, 0.898225, 0.912722, 0.873373, 0.923077, 0.910651, 0.921006, 0.918935, 0.906509, 0.894083, 0.923077, 0.927219, 0.912722, 0.921006, 0.871302, 0.90858, 0.898225, 0.889941, 0.786391, 0.786391};
      const std::vector<double>  binedgevalues_E45_50(binedgevalues_E45_50_list, binedgevalues_E45_50_list+22);
      const std::vector<double>  binvalues_E45_50 =  makeBinValues(binedgevalues_E45_50);
      const BinnedFn1D<double> _eff_E45_50 = {binedges_E45_50, binvalues_E45_50};

      const double binedges_E50_60_list[] = {-2.5, -2.44076, -2.21564, -1.93128, -1.69431, -1.46919, -1.26777, -0.983412, -0.7109, -0.35545, -0.0592417, 0.0592417, 0.35545, 0.7109, 0.983412, 1.27962, 1.46919, 1.68246, 1.91943, 2.22749, 2.44076, 2.5};
      const std::vector<double>  binedges_E50_60(binedges_E50_60_list, binedges_E50_60_list+22);
      const double binedgevalues_E50_60_list[] = {0.785417, 0.785417, 0.891667, 0.9, 0.916667, 0.877083, 0.927083, 0.91875, 0.91875, 0.922917, 0.90625, 0.9, 0.922917, 0.929167, 0.920833, 0.925, 0.885417, 0.9125, 0.904167, 0.885417, 0.7625, 0.7625};
      const std::vector<double> binedgevalues_E50_60(binedgevalues_E50_60_list, binedgevalues_E50_60_list+22);
      const std::vector<double>  binvalues_E50_60 = makeBinValues(binedgevalues_E50_60);
      const BinnedFn1D<double> _eff_E50_60 = {binedges_E50_60, binvalues_E50_60};

      const double binedges_E60_80_list[] = {-2.5, -2.44933, -2.22119, -1.9353, -1.68491, -1.47115, -1.2682, -0.982628, -0.696912, -0.351494, -0.0423579, 0.0526683, 0.350856, 0.719871, 1.00552, 1.29137, 1.46938, 1.69596, 1.94572, 2.24305, 2.45479, 2.5};
      const std::vector<double>  binedges_E60_80(binedges_E60_80_list, binedges_E60_80_list+22);
      const double binedgevalues_E60_80_list[] = {0.779163, 0.779188, 0.893866, 0.904402, 0.927423, 0.896262, 0.92968, 0.921466, 0.921585, 0.932145, 0.909357, 0.896897, 0.930355, 0.928425, 0.924377, 0.93283, 0.899571, 0.922582, 0.908102, 0.89156, 0.741648, 0.741678};
      const std::vector<double> binedgevalues_E60_80(binedgevalues_E60_80_list, binedgevalues_E60_80_list+22);
      const std::vector<double>  binvalues_E60_80 = makeBinValues(binedgevalues_E60_80);
      const BinnedFn1D<double> _eff_E60_80 = {binedges_E60_80, binvalues_E60_80};

      const double binedges_E80_list[] = {-2.5, -2.45835, -2.22058, -1.94663, -1.68451, -1.44712, -1.27961, -0.970161, -0.708258, -0.351259, -0.0537477, 0.0532884, 0.351188, 0.72041, 0.994111, 1.29176, 1.4815, 1.70839, 1.93419, 2.21998, 2.45825, 2.5};
      const std::vector<double>  binedges_E80(binedges_E80_list, binedges_E80_list+22);
      const double binedgevalues_E80_list[] = {0.951041, 0.948944, 0.930151, 0.938346, 0.9507, 0.909058, 0.95884, 0.954557, 0.954449, 0.945992, 0.939637, 0.933361, 0.949854, 0.960086, 0.953741, 0.955695, 0.911996, 0.953445, 0.930502, 0.934538, 0.944824, 0.9448};
      const std::vector<double> binedgevalues_E80(binedgevalues_E80_list, binedgevalues_E80_list+22);
      const std::vector<double>  binvalues_E80 = makeBinValues(binedgevalues_E80);
      const BinnedFn1D<double> _eff_E80 = {binedges_E80, binvalues_E80};

      // Now loop over the electrons and only keep those that pass the random efficiency cut
      /// @todo This is duplicated below -- split into a single util fn (in unnamed namespace?) when binned fns are static
      std::vector<Particle*> survivors;
      for (Particle* electron : electrons ) {
        const double e_pt = electron->pT();
        const double e_eta = electron->eta();
        bool hasTag = false;
        if (fabs(e_eta) < 2.5 && e_pt >= 10) {
          if (HEPUtils::in_range(e_pt, 10, 15)) hasTag = random_bool(_eff_E10_15, fabs(e_eta));
          else if (HEPUtils::in_range(e_pt, 15, 20)) hasTag = random_bool(_eff_E15_20, fabs(e_eta));
          else if (HEPUtils::in_range(e_pt, 20, 25)) hasTag = random_bool(_eff_E20_25, e_eta);
          else if (HEPUtils::in_range(e_pt, 25, 30)) hasTag = random_bool(_eff_E25_30, e_eta);
          else if (HEPUtils::in_range(e_pt, 30, 35)) hasTag = random_bool(_eff_E30_35, e_eta);
          else if (HEPUtils::in_range(e_pt, 35, 40)) hasTag = random_bool(_eff_E35_40, e_eta);
          else if (HEPUtils::in_range(e_pt, 40, 45)) hasTag = random_bool(_eff_E40_45, e_eta);
          else if (HEPUtils::in_range(e_pt, 45, 50)) hasTag = random_bool(_eff_E45_50, e_eta);
          else if (HEPUtils::in_range(e_pt, 50, 60)) hasTag = random_bool(_eff_E50_60, e_eta);
          else if (HEPUtils::in_range(e_pt, 60, 80)) hasTag = random_bool(_eff_E60_80, e_eta);
          else hasTag = random_bool(_eff_E80, e_eta);
        }
        if (hasTag) survivors.push_back(electron);
      }
      electrons = survivors;
    }


    /// Efficiency function for Tight ID electrons
    ///
    /// Numbers digitised from 8 TeV note (ATLAS-CONF-2014-032)
    inline void applyTightIDElectronSelection(std::vector<Particle*>& electrons) {

      /// @todo Avoid re-building these big binned functions every time this is called: use statics and initialise the BinnedFn1Ds only once. C++11 would help...

      const double binedges_E10_15_list[] = {0., 0.0485903, 0.458914, 1.10009, 1.46117, 1.78881, 2.27013, 2.5};
      const std::vector<double>  binedges_E10_15(binedges_E10_15_list, binedges_E10_15_list+8);
      const double binedgevalues_E10_15_list[] = {0.57971, 0.582609, 0.681159, 0.655072, 0.46087, 0.634783, 0.689855, 0.689855};
      const std::vector<double>  binedgevalues_E10_15(binedgevalues_E10_15_list, binedgevalues_E10_15_list+8);
      const std::vector<double> binvalues_E10_15 = makeBinValues(binedgevalues_E10_15);
      const BinnedFn1D<double> _eff_E10_15(binedges_E10_15, binvalues_E10_15);

      const double binedges_E15_20_list[] = {0., 0.0533175, 0.450237, 1.09597, 1.46327, 1.78318, 2.26896, 2.5};
      const std::vector<double>  binedges_E15_20(binedges_E15_20_list, binedges_E15_20_list+8);
      const double binedgevalues_E15_20_list[] = {0.631884, 0.628986, 0.727536, 0.701449, 0.565217, 0.666667, 0.733333, 0.733333};
      const std::vector<double>  binedgevalues_E15_20(binedgevalues_E15_20_list, binedgevalues_E15_20_list+8);
      const std::vector<double> binvalues_E15_20 = makeBinValues(binedgevalues_E15_20);
      const BinnedFn1D<double> _eff_E15_20(binedges_E15_20, binvalues_E15_20);

      const double binedges_E20_25_list[] =  {-2.5, -2.44062, -2.22684, -1.92993, -1.66865, -1.45487, -1.26485, -0.967933, -0.706651, -0.350356, -0.0415677, 0.0653207, 0.362233, 0.718527, 0.991686, 1.27672, 1.47862, 1.6924, 1.92993, 2.22684, 2.46437, 2.5};
      const std::vector<double>  binedges_E20_25(binedges_E20_25_list, binedges_E20_25_list+22);
      const double binedgevalues_E20_25_list[] = {0.678698, 0.678674, 0.70965, 0.65361, 0.655573, 0.599567, 0.6844, 0.694632, 0.729731, 0.731654, 0.665254, 0.640358, 0.743785, 0.733282, 0.697962, 0.672992, 0.585926, 0.660394, 0.652011, 0.703663, 0.670429, 0.668338};
      const std::vector<double>  binedgevalues_E20_25(binedgevalues_E20_25_list, binedgevalues_E20_25_list+22);
      const std::vector<double> binvalues_E20_25 = makeBinValues(binedgevalues_E20_25);
      const BinnedFn1D<double> _eff_E20_25(binedges_E20_25, binvalues_E20_25);

      const double binedges_E25_30_list[] = {-2.5, -2.44062, -2.22684, -1.91805, -1.68052, -1.45487, -1.27672, -0.97981, -0.706651, -0.350356, -0.0415677, 0.0771971, 0.362233, 0.718527, 0.991686, 1.30048, 1.47862, 1.6924, 1.94181, 2.22684, 2.46437, 2.5};
      const std::vector<double>  binedges_E25_30(binedges_E25_30_list, binedges_E25_30_list+22);
      const double binedgevalues_E25_30_list[] = {0.678932, 0.681034, 0.737205, 0.683328, 0.695889, 0.633669, 0.720983, 0.733569, 0.758609, 0.769142, 0.69657, 0.688311, 0.771515, 0.771663, 0.734388, 0.717899, 0.636964, 0.699368, 0.689086, 0.730747, 0.67684, 0.67686};
      const std::vector<double>  binedgevalues_E25_30(binedgevalues_E25_30_list, binedgevalues_E25_30_list+22);
      const std::vector<double> binvalues_E25_30 = makeBinValues(binedgevalues_E25_30);
      const BinnedFn1D<double> _eff_E25_30(binedges_E25_30, binvalues_E25_30);

      const double binedges_E30_35_list[] = {-2.5, -2.45249, -2.21496, -1.94181, -1.68052, -1.47862, -1.27672, -0.97981, -0.706651, -0.33848, -0.0415677, 0.0534442, 0.362233, 0.718527, 1.00356, 1.27672, 1.46675, 1.68052, 1.95368, 2.23872, 2.45249, 2.5};
      const std::vector<double>  binedges_E30_35(binedges_E30_35_list, binedges_E30_35_list+22);
      const double binedgevalues_E30_35_list[] = {0.691395, 0.691375, 0.749436, 0.716089, 0.726366, 0.653582, 0.749047, 0.771772, 0.800739, 0.802663, 0.731916, 0.71526, 0.802372, 0.810532, 0.773025, 0.75214, 0.656512, 0.722892, 0.712393, 0.745509, 0.670643, 0.6727};
      const std::vector<double>  binedgevalues_E30_35(binedgevalues_E30_35_list, binedgevalues_E30_35_list+22);
      const std::vector<double> binvalues_E30_35 = makeBinValues(binedgevalues_E30_35);
      const BinnedFn1D<double> _eff_E30_35(binedges_E30_35, binvalues_E30_35);

      const double binedges_E35_40_list[] = {-2.5, -2.46296, -2.22413, -1.93966, -1.7017, -1.47721, -1.28567, -0.988409, -0.714721, -0.334744, -0.0510125, 0.0437527, 0.342215, 0.710598, 0.971211, 1.27968, 1.45638, 1.68306, 1.94399, 2.21764, 2.44185, 2.5};
      const std::vector<double>  binedges_E35_40(binedges_E35_40_list, binedges_E35_40_list+22);
      const double binedgevalues_E35_40_list[] = {0.683086, 0.683086, 0.759941, 0.726706, 0.751632, 0.683086, 0.772404, 0.793175, 0.824332, 0.820178, 0.743323, 0.728783, 0.820178, 0.832641, 0.793175, 0.774481, 0.689318, 0.749555, 0.728783, 0.757864, 0.6727, 0.6727};
      const std::vector<double>  binedgevalues_E35_40(binedgevalues_E35_40_list, binedgevalues_E35_40_list+22);
      const std::vector<double> binvalues_E35_40 = makeBinValues(binedgevalues_E35_40);
      const BinnedFn1D<double> _eff_E35_40(binedges_E35_40, binvalues_E35_40);

      const double binedges_E40_45_list[] = {-2.5, -2.45261, -2.21564, -1.94313, -1.69431, -1.45735, -1.27962, -0.983412, -0.7109, -0.35545, -0.0592417, 0.0473934, 0.35545, 0.699052, 0.983412, 1.26777, 1.45735, 1.67062, 1.93128, 2.20379, 2.45261, 2.5};
      const std::vector<double>  binedges_E40_45(binedges_E40_45_list, binedges_E40_45_list+22);
      const double binedgevalues_E40_45_list[] = {0.693472, 0.693472, 0.782789, 0.757864, 0.784866, 0.726706, 0.797329, 0.803561, 0.836795, 0.805638, 0.747478, 0.735015, 0.805638, 0.843027, 0.807715, 0.797329, 0.732938, 0.780712, 0.762018, 0.782789, 0.674777, 0.674777};
      const std::vector<double> binedgevalues_E40_45(binedgevalues_E40_45_list, binedgevalues_E40_45_list+22);
      const std::vector<double> binvalues_E40_45 = makeBinValues(binedgevalues_E40_45);
      const BinnedFn1D<double> _eff_E40_45(binedges_E40_45, binvalues_E40_45);

      const double binedges_E45_50_list[] = {-2.5, -2.46311, -2.22329, -1.93875, -1.70073, -1.47585, -1.273, -0.976015, -0.714205, -0.358403, -0.0625448, 0.0560444, 0.354151, 0.711078, 0.98364, 1.28045, 1.45768, 1.68407, 1.94493, 2.20653, 2.4415, 2.5};
      const std::vector<double>  binedges_E45_50(binedges_E45_50_list, binedges_E45_50_list+22);
      const double binedgevalues_E45_50_list[] = {0.674556, 0.674556, 0.809172, 0.780178, 0.809172, 0.763609, 0.819527, 0.823669, 0.854734, 0.82574, 0.763609, 0.753254, 0.823669, 0.860947, 0.82574, 0.819527, 0.76568, 0.809172, 0.78432, 0.802959, 0.651775, 0.651775};
      const std::vector<double>  binedgevalues_E45_50(binedgevalues_E45_50_list, binedgevalues_E45_50_list+22);
      const std::vector<double>  binvalues_E45_50 =  makeBinValues(binedgevalues_E45_50);
      const BinnedFn1D<double> _eff_E45_50 = {binedges_E45_50, binvalues_E45_50};

      const double binedges_E50_60_list[] = {-2.5, -2.45261, -2.21564, -1.93128, -1.68246, -1.45735, -1.27962, -0.995261, -0.699052, -0.343602, -0.0592417, 0.0592417, 0.35545, 0.699052, 0.983412, 1.26777, 1.4455, 1.68246, 1.94313, 2.21564, 2.45261, 2.5};
      const std::vector<double>  binedges_E50_60(binedges_E50_60_list, binedges_E50_60_list+22);
      const double binedgevalues_E50_60_list[] = {0.6625, 0.6625, 0.810417, 0.795833, 0.81875, 0.779167, 0.839583, 0.84375, 0.860417, 0.841667, 0.777083, 0.764583, 0.841667, 0.877083, 0.85, 0.839583, 0.785417, 0.816667, 0.8, 0.804167, 0.64375, 0.64375};
      const std::vector<double> binedgevalues_E50_60(binedgevalues_E50_60_list, binedgevalues_E50_60_list+22);
      const std::vector<double>  binvalues_E50_60 = makeBinValues(binedgevalues_E50_60);
      const BinnedFn1D<double> _eff_E50_60 = {binedges_E50_60, binvalues_E50_60};

      const double binedges_E60_80_list[] = {-2.5, -2.46326, -2.22265, -1.93711, -1.69844, -1.47299, -1.28152, -0.995631, -0.709702, -0.364674, -0.0564949, 0.0504716, 0.349652, 0.707116, 0.980538, 1.27812, 1.46757, 1.69447, 1.94394, 2.24157, 2.45288, 2.5};
      const std::vector<double>  binedges_E60_80(binedges_E60_80_list, binedges_E60_80_list+22);
      const double binedgevalues_E60_80_list[] = {0.660412, 0.660432, 0.808449, 0.798151, 0.831584, 0.787928, 0.846341, 0.856877, 0.869496, 0.85714, 0.778101, 0.767729, 0.859521, 0.87842, 0.855617, 0.853658, 0.79332, 0.835081, 0.803935, 0.804059, 0.629147, 0.629172};
      const std::vector<double> binedgevalues_E60_80(binedgevalues_E60_80_list, binedgevalues_E60_80_list+22);
      const std::vector<double>  binvalues_E60_80 = makeBinValues(binedgevalues_E60_80);
      const BinnedFn1D<double> _eff_E60_80 = {binedges_E60_80, binvalues_E60_80};

      const double binedges_E80_list[] = {-2.5, -2.45987, -2.22149, -1.94797, -1.69748, -1.47206, -1.29251, -0.994818, -0.709105, -0.352212, -0.0558319, 0.0513809, 0.374044, 0.719562, 0.981359, 1.27873, 1.46843, 1.70723, 1.9449, 2.20712, 2.45676, 2.5};
      const std::vector<double>  binedges_E80(binedges_E80_list, binedges_E80_list+22);
      const double binedgevalues_E80_list[] = {0.859652, 0.859627, 0.876145, 0.859415, 0.888391, 0.8426, 0.900685, 0.904716, 0.904597, 0.889909, 0.817086, 0.821195, 0.893762, 0.910235, 0.903895, 0.889231, 0.843455, 0.884899, 0.859875, 0.87846, 0.857585, 0.85756};
      const std::vector<double> binedgevalues_E80(binedgevalues_E80_list, binedgevalues_E80_list+22);
      const std::vector<double>  binvalues_E80 = makeBinValues(binedgevalues_E80);
      const BinnedFn1D<double> _eff_E80 = {binedges_E80, binvalues_E80};

      // Now loop over the electrons and only keep those that pass the random efficiency cut
      /// @todo This is an exact duplication of the above filtering code -- split into a single util fn (in unnamed namespace?) when binned fns are static
      std::vector<Particle*> survivors;
      for (Particle* electron : electrons ) {
        const double e_pt = electron->pT();
        const double e_eta = electron->eta();
        bool hasTag = false;
        if (fabs(e_eta) < 2.5 && e_pt >= 10) {
          if (HEPUtils::in_range(e_pt, 10, 15))hasTag = random_bool(_eff_E10_15, fabs(e_eta));
          else if (HEPUtils::in_range(e_pt, 15, 20)) hasTag = random_bool(_eff_E15_20, fabs(e_eta));
          else if (HEPUtils::in_range(e_pt, 20, 25)) hasTag = random_bool(_eff_E20_25, e_eta);
          else if (HEPUtils::in_range(e_pt, 25, 30)) hasTag = random_bool(_eff_E25_30, e_eta);
          else if (HEPUtils::in_range(e_pt, 30, 35)) hasTag = random_bool(_eff_E30_35, e_eta);
          else if (HEPUtils::in_range(e_pt, 35, 40)) hasTag = random_bool(_eff_E35_40, e_eta);
          else if (HEPUtils::in_range(e_pt, 40, 45)) hasTag = random_bool(_eff_E40_45, e_eta);
          else if (HEPUtils::in_range(e_pt, 45, 50)) hasTag = random_bool(_eff_E45_50, e_eta);
          else if (HEPUtils::in_range(e_pt, 50, 60)) hasTag = random_bool(_eff_E50_60, e_eta);
          else if (HEPUtils::in_range(e_pt, 60, 80)) hasTag = random_bool(_eff_E60_80, e_eta);
          else hasTag = random_bool(_eff_E80, e_eta);
        }
        if (hasTag) survivors.push_back(electron);
      }
      electrons = survivors;
    }

    //@}


  }
}