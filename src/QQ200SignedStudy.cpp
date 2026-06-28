#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"

#include "CommandLine.h"
#include "ThrustTools.h"

namespace
{
   constexpr int AxisCount = 2;
   constexpr int MaxHarmonic = 3;

   struct MultiplicityBin
   {
      int Low;
      int High;
   };

   struct Track
   {
      double Pt = 0.0;
      double Eta = 0.0;
      double Phi = 0.0;
   };

   struct C2Case
   {
      std::string ObservableType;
      std::string Name;
      double GapValue = 0.0;
      int Mode = 0; // 0 inclusive, 1 rapidity gap, 2 two subevent
      double PtMin = 0.4;
      double PtMax = -1.0;
      double EtaGapMin = 0.0;
      double TwoSubBoundary = 0.0;
   };

   struct Accumulator
   {
      double Events = 0.0;
      double PairDen = 0.0;
      double Num = 0.0;
   };

   struct VnAccumulator
   {
      double Events = 0.0;
      double PairDen = 0.0;
      std::array<double, MaxHarmonic + 1> Num = {};
   };

   struct Options
   {
      std::string Input;
      std::string OutputC2;
      std::string OutputVn;
      std::string Tree = "t";
      std::string Sample = "Pythia qqbar 200 GeV";
      std::string Setting = "unknown";
      long long StartEntry = 0;
      long long EndEntry = -1;
      int BlockLabel = 0;
      double MultiplicityPtMin = 0.4;
      bool UsePassEventSelection = true;
   };

   const std::vector<MultiplicityBin> StandardBins = {
      {0, 10}, {10, 15}, {15, 20}, {20, 25}, {25, 30}, {30, 35}, {35, 40}, {40, 999}
   };

   const std::vector<MultiplicityBin> HighBins = {
      {30, 35}, {35, 40}, {40, 45}, {45, 50}, {50, 999}
   };

   std::string AxisName(int axis)
   {
      return axis == 0 ? "beam" : "thrust";
   }

   bool InBin(int value, const MultiplicityBin &bin)
   {
      return value >= bin.Low && value < bin.High;
   }

   std::string BinLabel(const MultiplicityBin &bin)
   {
      return std::to_string(bin.Low) + "_" + std::to_string(bin.High);
   }

   std::string BinText(const MultiplicityBin &bin)
   {
      return "[" + std::to_string(bin.Low) + "," +
         (bin.High >= 999 ? std::string("inf") : std::to_string(bin.High)) + ")";
   }

   int FindBin(int multiplicity, const std::vector<MultiplicityBin> &bins)
   {
      for (int i = 0; i < static_cast<int>(bins.size()); ++i)
         if (InBin(multiplicity, bins[i]))
            return i;
      return -1;
   }

   Options ParseOptions(int argc, char *argv[])
   {
      CommandLine cl(argc, argv);
      Options options;
      options.Input = cl.Get("Input", "");
      options.OutputC2 = cl.Get("OutputC2", "");
      options.OutputVn = cl.Get("OutputVn", "");
      options.Tree = cl.Get("Tree", "t");
      options.Sample = cl.Get("Sample", "Pythia qqbar 200 GeV");
      options.Setting = cl.Get("Setting", "unknown");
      options.StartEntry = std::max(0LL, cl.GetLongLong("StartEntry", 0));
      options.EndEntry = cl.GetLongLong("EndEntry", -1);
      options.BlockLabel = cl.GetInt("BlockLabel", 0);
      options.MultiplicityPtMin = cl.GetDouble("MultiplicityPtMin", 0.4);
      options.UsePassEventSelection = cl.GetBool("UsePassEventSelection", true);
      if (options.Input.empty())
         throw std::runtime_error("--Input is required");
      if (options.OutputC2.empty())
         throw std::runtime_error("--OutputC2 is required");
      if (options.OutputVn.empty())
         throw std::runtime_error("--OutputVn is required");
      return options;
   }

   bool InPtRange(const Track &track, const C2Case &c)
   {
      if (!(track.Pt > c.PtMin))
         return false;
      if (c.PtMax > 0.0 && !(track.Pt < c.PtMax))
         return false;
      return true;
   }

   void AddInclusive(const std::vector<Track> &tracks, const C2Case &c, Accumulator &acc)
   {
      double qx = 0.0;
      double qy = 0.0;
      int n = 0;
      for (const Track &track : tracks)
      {
         if (!InPtRange(track, c))
            continue;
         qx += std::cos(2.0 * track.Phi);
         qy += std::sin(2.0 * track.Phi);
         ++n;
      }
      if (n < 2)
         return;
      acc.PairDen += static_cast<double>(n) * static_cast<double>(n - 1);
      acc.Num += qx * qx + qy * qy - static_cast<double>(n);
   }

   void AddRapidityGap(const std::vector<Track> &tracks, const C2Case &c, Accumulator &acc)
   {
      const int count = static_cast<int>(tracks.size());
      for (int i = 0; i < count; ++i)
      {
         if (!InPtRange(tracks[i], c))
            continue;
         for (int j = i + 1; j < count; ++j)
         {
            if (!InPtRange(tracks[j], c))
               continue;
            if (!(std::abs(tracks[i].Eta - tracks[j].Eta) > c.EtaGapMin))
               continue;
            acc.PairDen += 2.0;
            acc.Num += 2.0 * std::cos(2.0 * (tracks[i].Phi - tracks[j].Phi));
         }
      }
   }

   void AddTwoSubevent(const std::vector<Track> &tracks, const C2Case &c, Accumulator &acc)
   {
      double ax = 0.0;
      double ay = 0.0;
      double bx = 0.0;
      double by = 0.0;
      int na = 0;
      int nb = 0;
      for (const Track &track : tracks)
      {
         if (!InPtRange(track, c))
            continue;
         const double cx = std::cos(2.0 * track.Phi);
         const double sy = std::sin(2.0 * track.Phi);
         if (track.Eta < -c.TwoSubBoundary)
         {
            ax += cx;
            ay += sy;
            ++na;
         }
         else if (track.Eta > c.TwoSubBoundary)
         {
            bx += cx;
            by += sy;
            ++nb;
         }
      }
      if (na == 0 || nb == 0)
         return;
      acc.PairDen += static_cast<double>(na) * static_cast<double>(nb);
      acc.Num += ax * bx + ay * by;
   }

   void AddVnDelta(const std::vector<Track> &tracks, VnAccumulator &acc)
   {
      const int count = static_cast<int>(tracks.size());
      for (int i = 0; i < count; ++i)
      {
         if (!(tracks[i].Pt > 0.4))
            continue;
         for (int j = i + 1; j < count; ++j)
         {
            if (!(tracks[j].Pt > 0.4))
               continue;
            const double deta = std::abs(tracks[i].Eta - tracks[j].Eta);
            if (!(deta > 1.6 && deta < 3.2))
               continue;
            const double dphi = tracks[i].Phi - tracks[j].Phi;
            acc.PairDen += 2.0;
            for (int n = 1; n <= MaxHarmonic; ++n)
               acc.Num[n] += 2.0 * std::cos(static_cast<double>(n) * dphi);
         }
      }
   }
}

int main(int argc, char *argv[])
{
   try
   {
      const Options options = ParseOptions(argc, argv);

      const std::vector<C2Case> cases = {
         {"inclusive", "inclusive", 0.0, 0, 0.4, -1.0, 0.0, 0.0},
         {"pt_selection", "pt_1_to_3_GeV", 0.0, 0, 1.0, 3.0, 0.0, 0.0},
         {"rapidity_gap", "abs_delta_eta_gt_1p0", 1.0, 1, 0.4, -1.0, 1.0, 0.0},
         {"rapidity_gap", "abs_delta_eta_gt_1p6", 1.6, 1, 0.4, -1.0, 1.6, 0.0},
         {"rapidity_gap", "abs_delta_eta_gt_2p0", 2.0, 1, 0.4, -1.0, 2.0, 0.0},
         {"rapidity_gap", "abs_delta_eta_gt_2p5", 2.5, 1, 0.4, -1.0, 2.5, 0.0},
         {"rapidity_gap", "abs_delta_eta_gt_3p0", 3.0, 1, 0.4, -1.0, 3.0, 0.0},
         {"two_subevent", "eta_lt_minus_0p0_vs_eta_gt_0p0", 0.0, 2, 0.4, -1.0, 0.0, 0.0},
         {"two_subevent", "eta_lt_minus_0p5_vs_eta_gt_0p5", 0.5, 2, 0.4, -1.0, 0.0, 0.5},
         {"two_subevent", "eta_lt_minus_0p8_vs_eta_gt_0p8", 0.8, 2, 0.4, -1.0, 0.0, 0.8},
         {"two_subevent", "eta_lt_minus_1p0_vs_eta_gt_1p0", 1.0, 2, 0.4, -1.0, 0.0, 1.0},
      };

      TFile inputFile(options.Input.c_str(), "READ");
      if (inputFile.IsZombie())
         throw std::runtime_error("Failed to open " + options.Input);

      TTree *tree = nullptr;
      inputFile.GetObject(options.Tree.c_str(), tree);
      if (tree == nullptr)
         throw std::runtime_error("Failed to find tree " + options.Tree + " in " + options.Input);

      std::vector<float> *px = nullptr;
      std::vector<float> *py = nullptr;
      std::vector<float> *pz = nullptr;
      std::vector<short> *pwflag = nullptr;
      bool passEventSelection = true;

      tree->SetBranchStatus("*", 0);
      tree->SetBranchStatus("px", 1);
      tree->SetBranchStatus("py", 1);
      tree->SetBranchStatus("pz", 1);
      tree->SetBranchStatus("pwflag", 1);
      tree->SetBranchAddress("px", &px);
      tree->SetBranchAddress("py", &py);
      tree->SetBranchAddress("pz", &pz);
      tree->SetBranchAddress("pwflag", &pwflag);
      if (tree->GetBranch("passEventSelection") != nullptr)
      {
         tree->SetBranchStatus("passEventSelection", 1);
         tree->SetBranchAddress("passEventSelection", &passEventSelection);
      }

      const long long totalEntries = tree->GetEntries();
      long long endEntry = options.EndEntry;
      if (endEntry < 0 || endEntry > totalEntries)
         endEntry = totalEntries;
      if (options.StartEntry >= endEntry)
         throw std::runtime_error("Empty entry range");

      std::vector<std::vector<std::vector<Accumulator>>> c2Acc;
      c2Acc.resize(cases.size());
      for (auto &caseAcc : c2Acc)
      {
         caseAcc.resize(AxisCount);
         for (auto &axisAcc : caseAcc)
            axisAcc.resize(StandardBins.size());
      }

      std::vector<std::vector<VnAccumulator>> vnStandard(AxisCount);
      std::vector<std::vector<VnAccumulator>> vnHigh(AxisCount);
      for (int axis = 0; axis < AxisCount; ++axis)
      {
         vnStandard[axis].resize(StandardBins.size());
         vnHigh[axis].resize(HighBins.size());
      }

      for (long long entry = options.StartEntry; entry < endEntry; ++entry)
      {
         tree->GetEntry(entry);
         if (options.UsePassEventSelection && !passEventSelection)
            continue;
         if (px == nullptr || py == nullptr || pz == nullptr || pwflag == nullptr)
            continue;
         if (px->size() != py->size() || px->size() != pz->size() || px->size() != pwflag->size())
            continue;

         std::vector<TVector3> thrustInput;
         thrustInput.reserve(px->size());
         std::vector<TVector3> chargedMomenta;
         chargedMomenta.reserve(px->size());

         int multiplicity = 0;
         for (int i = 0; i < static_cast<int>(px->size()); ++i)
         {
            const TVector3 p((*px)[i], (*py)[i], (*pz)[i]);
            if (p.Mag2() > 0.0)
               thrustInput.push_back(p);
            if (!AlephCumulant::IsChargedPwflag((*pwflag)[i]) || p.Mag2() <= 0.0)
               continue;
            chargedMomenta.push_back(p);
            if (p.Pt() > options.MultiplicityPtMin)
               ++multiplicity;
         }

         const int standardBin = FindBin(multiplicity, StandardBins);
         if (standardBin < 0)
            continue;
         const int highBin = FindBin(multiplicity, HighBins);

         const TVector3 thrustAxis = AlephCumulant::ComputeThrustAxis(thrustInput);
         if (thrustAxis.Mag2() <= 0.0)
            continue;

         std::array<std::vector<Track>, AxisCount> tracks;
         for (const TVector3 &p : chargedMomenta)
         {
            if (p.Pt() > 0.0)
               tracks[0].push_back({p.Pt(), p.Eta(), p.Phi()});

            const double thrustPt = AlephCumulant::PtFromAxis(thrustAxis, p);
            const double thrustEta = AlephCumulant::EtaFromAxis(thrustAxis, p);
            const double thrustPhi = AlephCumulant::PhiFromAxis(thrustAxis, p);
            if (std::isfinite(thrustPt) && std::isfinite(thrustEta) && std::isfinite(thrustPhi))
               tracks[1].push_back({thrustPt, thrustEta, thrustPhi});
         }

         for (int caseIndex = 0; caseIndex < static_cast<int>(cases.size()); ++caseIndex)
         {
            const C2Case &c = cases[caseIndex];
            for (int axis = 0; axis < AxisCount; ++axis)
            {
               Accumulator &acc = c2Acc[caseIndex][axis][standardBin];
               acc.Events += 1.0;
               if (c.Mode == 0)
                  AddInclusive(tracks[axis], c, acc);
               else if (c.Mode == 1)
                  AddRapidityGap(tracks[axis], c, acc);
               else
                  AddTwoSubevent(tracks[axis], c, acc);
            }
         }

         for (int axis = 0; axis < AxisCount; ++axis)
         {
            vnStandard[axis][standardBin].Events += 1.0;
            AddVnDelta(tracks[axis], vnStandard[axis][standardBin]);
            if (highBin >= 0)
            {
               vnHigh[axis][highBin].Events += 1.0;
               AddVnDelta(tracks[axis], vnHigh[axis][highBin]);
            }
         }

         if ((entry - options.StartEntry + 1) % 50000 == 0)
            std::cout << "Processed " << (entry - options.StartEntry + 1)
               << " / " << (endEntry - options.StartEntry) << " entries" << std::endl;
      }

      std::ofstream c2Out(options.OutputC2);
      if (!c2Out)
         throw std::runtime_error("Failed to create " + options.OutputC2);
      c2Out << std::setprecision(12);
      c2Out << "sample,shoving_setting,axis,observable_type,case,gap_value,multiplicity_bin,"
            << "multiplicity_bin_text,block,number_of_events,number_of_valid_pairs,numerator\n";
      for (int caseIndex = 0; caseIndex < static_cast<int>(cases.size()); ++caseIndex)
      {
         const C2Case &c = cases[caseIndex];
         for (int axis = 0; axis < AxisCount; ++axis)
         {
            for (int bin = 0; bin < static_cast<int>(StandardBins.size()); ++bin)
            {
               const Accumulator &acc = c2Acc[caseIndex][axis][bin];
               c2Out << '"' << options.Sample << '"' << ','
                     << options.Setting << ','
                     << AxisName(axis) << ','
                     << c.ObservableType << ','
                     << c.Name << ','
                     << c.GapValue << ','
                     << BinLabel(StandardBins[bin]) << ','
                     << '"' << BinText(StandardBins[bin]) << '"' << ','
                     << options.BlockLabel << ','
                     << acc.Events << ','
                     << acc.PairDen << ','
                     << acc.Num << '\n';
            }
         }
      }

      std::ofstream vnOut(options.OutputVn);
      if (!vnOut)
         throw std::runtime_error("Failed to create " + options.OutputVn);
      vnOut << std::setprecision(12);
      vnOut << "sample,shoving_setting,axis,bin_scheme,multiplicity_bin,multiplicity_bin_text,"
            << "n,block,number_of_events,number_of_valid_pairs,numerator\n";
      auto writeVn = [&](const std::string &scheme, const std::vector<MultiplicityBin> &bins,
                         const std::vector<std::vector<VnAccumulator>> &accs) {
         for (int axis = 0; axis < AxisCount; ++axis)
         {
            for (int bin = 0; bin < static_cast<int>(bins.size()); ++bin)
            {
               const VnAccumulator &acc = accs[axis][bin];
               for (int n = 1; n <= MaxHarmonic; ++n)
               {
                  vnOut << '"' << options.Sample << '"' << ','
                        << options.Setting << ','
                        << AxisName(axis) << ','
                        << scheme << ','
                        << BinLabel(bins[bin]) << ','
                        << '"' << BinText(bins[bin]) << '"' << ','
                        << n << ','
                        << options.BlockLabel << ','
                        << acc.Events << ','
                        << acc.PairDen << ','
                        << acc.Num[n] << '\n';
               }
            }
         }
      };
      writeVn("standard", StandardBins, vnStandard);
      writeVn("high_multiplicity", HighBins, vnHigh);

      std::cout << "Wrote " << options.OutputC2 << " and " << options.OutputVn << std::endl;
   }
   catch (const std::exception &e)
   {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
