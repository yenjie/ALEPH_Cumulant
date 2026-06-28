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
   constexpr int MaxHarmonic = 3;

   struct MultiplicityBin
   {
      int Low;
      int High;
   };

   struct BinScheme
   {
      std::string Name;
      std::vector<MultiplicityBin> Bins;
   };

   struct Track
   {
      double Eta = 0.0;
      double Phi = 0.0;
   };

   struct Accumulator
   {
      double Events = 0.0;
      double PairDen = 0.0;
      std::array<double, MaxHarmonic + 1> Num = {};
   };

   struct Options
   {
      std::string Input;
      std::string Output;
      std::string Tree = "t";
      std::string Sample = "Pythia WW";
      std::string Setting = "unknown";
      long long StartEntry = 0;
      long long EndEntry = -1;
      int Chunks = 16;
      double LabPtMin = 0.4;
      double ThrustPtMin = 0.4;
      double EtaGapMin = 1.6;
      double EtaGapMax = 3.2;
      bool UsePassEventSelection = true;
      bool RawOutput = false;
      int BlockLabel = 0;
   };

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

   Options ParseOptions(int argc, char *argv[])
   {
      CommandLine cl(argc, argv);
      Options options;
      options.Input = cl.Get("Input", "");
      options.Output = cl.Get("Output", "WW200_Generator_SignedShovingTest/ww200_signed_VnDelta_one_sample.csv");
      options.Tree = cl.Get("Tree", "t");
      options.Sample = cl.Get("Sample", "Pythia WW");
      options.Setting = cl.Get("Setting", "unknown");
      options.StartEntry = std::max(0LL, cl.GetLongLong("StartEntry", 0));
      options.EndEntry = cl.GetLongLong("EndEntry", -1);
      options.Chunks = std::max(1, cl.GetInt("Chunks", 16));
      options.RawOutput = cl.GetBool("RawOutput", false);
      options.BlockLabel = cl.GetInt("BlockLabel", 0);
      options.LabPtMin = cl.GetDouble("LabPtMin", 0.4);
      options.ThrustPtMin = cl.GetDouble("ThrustPtMin", 0.4);
      options.EtaGapMin = cl.GetDouble("EtaGapMin", 1.6);
      options.EtaGapMax = cl.GetDouble("EtaGapMax", 3.2);
      options.UsePassEventSelection = cl.GetBool("UsePassEventSelection", true);
      if (options.Input.empty())
         throw std::runtime_error("--Input is required");
      return options;
   }

   double JackknifeError(const std::vector<double> &leaveOneValues)
   {
      const int n = static_cast<int>(leaveOneValues.size());
      if (n < 2)
         return 0.0;
      double mean = 0.0;
      int count = 0;
      for (double value : leaveOneValues)
      {
         if (!std::isfinite(value))
            continue;
         mean += value;
         ++count;
      }
      if (count < 2)
         return 0.0;
      mean /= static_cast<double>(count);

      double sum = 0.0;
      for (double value : leaveOneValues)
      {
         if (!std::isfinite(value))
            continue;
         const double diff = value - mean;
         sum += diff * diff;
      }
      return std::sqrt(static_cast<double>(count - 1) / static_cast<double>(count) * sum);
   }

   double Ratio(double num, double den)
   {
      if (den == 0.0)
         return std::numeric_limits<double>::quiet_NaN();
      return num / den;
   }

   void AddPairs(const std::vector<Track> &tracks, double etaMin, double etaMax,
      std::array<double, MaxHarmonic + 1> &num, double &den)
   {
      const int nTrack = static_cast<int>(tracks.size());
      for (int i = 0; i < nTrack; ++i)
      {
         for (int j = i + 1; j < nTrack; ++j)
         {
            const double deta = std::abs(tracks[i].Eta - tracks[j].Eta);
            if (!(deta > etaMin && deta < etaMax))
               continue;
            const double dphi = tracks[i].Phi - tracks[j].Phi;
            den += 2.0;
            for (int n = 1; n <= MaxHarmonic; ++n)
               num[n] += 2.0 * std::cos(static_cast<double>(n) * dphi);
         }
      }
   }

   std::vector<int> MatchingBins(int multiplicity, const std::vector<MultiplicityBin> &bins)
   {
      std::vector<int> result;
      for (int i = 0; i < static_cast<int>(bins.size()); ++i)
         if (InBin(multiplicity, bins[i]))
            result.push_back(i);
      return result;
   }

   int BlockIndex(long long entry, long long start, long long end, int blocks)
   {
      const long long span = std::max(1LL, end - start);
      int block = static_cast<int>(((entry - start) * blocks) / span);
      if (block < 0)
         block = 0;
      if (block >= blocks)
         block = blocks - 1;
      return block;
   }
}

int main(int argc, char *argv[])
{
   try
   {
      const Options options = ParseOptions(argc, argv);

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

      const std::vector<BinScheme> schemes = {
         {"standard", {{0, 10}, {10, 15}, {15, 20}, {20, 25}, {25, 30}, {30, 35}, {35, 40}, {40, 999}}},
         {"high_multiplicity", {{30, 35}, {35, 40}, {40, 45}, {45, 50}, {50, 999}}},
      };

      std::vector<std::vector<std::vector<Accumulator>>> blocks;
      blocks.resize(options.Chunks);
      for (auto &block : blocks)
      {
         block.resize(schemes.size());
         for (int s = 0; s < static_cast<int>(schemes.size()); ++s)
            block[s].resize(schemes[s].Bins.size());
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
         for (int i = 0; i < static_cast<int>(px->size()); ++i)
         {
            const TVector3 p((*px)[i], (*py)[i], (*pz)[i]);
            if (p.Mag2() > 0.0)
               thrustInput.push_back(p);
         }
         const TVector3 thrustAxis = AlephCumulant::ComputeThrustAxis(thrustInput);
         if (thrustAxis.Mag2() <= 0.0)
            continue;

         int labMultiplicity = 0;
         std::vector<Track> thrustTracks;
         thrustTracks.reserve(px->size());
         for (int i = 0; i < static_cast<int>(px->size()); ++i)
         {
            if (!AlephCumulant::IsChargedPwflag((*pwflag)[i]))
               continue;

            const TVector3 p((*px)[i], (*py)[i], (*pz)[i]);
            if (p.Mag2() <= 0.0)
               continue;

            if (p.Pt() > options.LabPtMin)
               ++labMultiplicity;

            const double thrustPt = AlephCumulant::PtFromAxis(thrustAxis, p);
            if (!(thrustPt > options.ThrustPtMin))
               continue;

            const double eta = AlephCumulant::EtaFromAxis(thrustAxis, p);
            const double phi = AlephCumulant::PhiFromAxis(thrustAxis, p);
            if (std::isfinite(eta) && std::isfinite(phi))
               thrustTracks.push_back({eta, phi});
         }

         const int block = BlockIndex(entry, options.StartEntry, endEntry, options.Chunks);
         for (int schemeIndex = 0; schemeIndex < static_cast<int>(schemes.size()); ++schemeIndex)
         {
            for (int binIndex : MatchingBins(labMultiplicity, schemes[schemeIndex].Bins))
            {
               Accumulator &acc = blocks[block][schemeIndex][binIndex];
               acc.Events += 1.0;
               AddPairs(thrustTracks, options.EtaGapMin, options.EtaGapMax, acc.Num, acc.PairDen);
            }
         }

         if ((entry - options.StartEntry + 1) % 100000 == 0)
            std::cout << "Processed " << (entry - options.StartEntry + 1)
               << " / " << (endEntry - options.StartEntry) << " entries" << std::endl;
      }

      std::ofstream out(options.Output);
      if (!out)
         throw std::runtime_error("Failed to create " + options.Output);
      out << std::setprecision(12);
      if (options.RawOutput)
      {
         out << "sample,shoving_setting,axis,bin_scheme,multiplicity_bin,multiplicity_bin_text,"
             << "n,block,number_of_events,number_of_valid_pairs,numerator\n";
      }
      else
      {
         out << "sample,shoving_setting,axis,bin_scheme,multiplicity_bin,multiplicity_bin_text,"
             << "n,number_of_events,number_of_valid_pairs,VnDelta,statistical_uncertainty,signed_vn_proxy\n";
      }

      for (int schemeIndex = 0; schemeIndex < static_cast<int>(schemes.size()); ++schemeIndex)
      {
         const BinScheme &scheme = schemes[schemeIndex];
         for (int binIndex = 0; binIndex < static_cast<int>(scheme.Bins.size()); ++binIndex)
         {
            Accumulator central;
            for (int block = 0; block < options.Chunks; ++block)
            {
               const Accumulator &source = blocks[block][schemeIndex][binIndex];
               central.Events += source.Events;
               central.PairDen += source.PairDen;
               for (int n = 1; n <= MaxHarmonic; ++n)
                  central.Num[n] += source.Num[n];
            }

            for (int n = 1; n <= MaxHarmonic; ++n)
            {
               if (options.RawOutput)
               {
                  out << '"' << options.Sample << '"' << ','
                      << options.Setting << ",thrust,"
                      << scheme.Name << ','
                      << BinLabel(scheme.Bins[binIndex]) << ','
                      << '"' << BinText(scheme.Bins[binIndex]) << '"' << ','
                      << n << ','
                      << options.BlockLabel << ','
                      << central.Events << ','
                      << central.PairDen << ','
                      << central.Num[n] << '\n';
                  continue;
               }

               std::vector<double> leaveOne;
               leaveOne.reserve(options.Chunks);
               for (int skip = 0; skip < options.Chunks; ++skip)
               {
                  double num = 0.0;
                  double den = 0.0;
                  for (int block = 0; block < options.Chunks; ++block)
                  {
                     if (block == skip)
                        continue;
                     const Accumulator &source = blocks[block][schemeIndex][binIndex];
                     num += source.Num[n];
                     den += source.PairDen;
                  }
                  leaveOne.push_back(Ratio(num, den));
               }

               const double vn = Ratio(central.Num[n], central.PairDen);
               const double error = JackknifeError(leaveOne);
               const double proxy = std::isfinite(vn) ? std::copysign(std::sqrt(std::abs(vn)), vn) :
                  std::numeric_limits<double>::quiet_NaN();

               out << '"' << options.Sample << '"' << ','
                   << options.Setting << ",thrust,"
                   << scheme.Name << ','
                   << BinLabel(scheme.Bins[binIndex]) << ','
                   << '"' << BinText(scheme.Bins[binIndex]) << '"' << ','
                   << n << ','
                   << central.Events << ','
                   << central.PairDen << ','
                   << vn << ','
                   << error << ','
                   << proxy << '\n';
            }
         }
      }
      std::cout << "Wrote " << options.Output << std::endl;
   }
   catch (const std::exception &e)
   {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
