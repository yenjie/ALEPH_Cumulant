#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TBranch.h"
#include "TClass.h"
#include "TFile.h"
#include "TH1D.h"
#include "TNamed.h"
#include "TTree.h"
#include "TVector3.h"

#include "CommandLine.h"
#include "ThrustTools.h"

namespace
{
   constexpr int MaxHalfCorrelation = 4;
   constexpr int AxisCount = 2;
   constexpr int BeamAxisIndex = 0;
   constexpr int ThrustAxisIndex = 1;
   constexpr int SubeventCount = 3;
   constexpr int MaxArrayParticles = 4096;
   constexpr std::array<int, MaxHalfCorrelation + 1> Factorials = {{1, 1, 2, 6, 24}};

   struct MultiplicityBin
   {
      int Low = 0;
      int High = 0;
   };

   struct AxisConfig
   {
      std::string Name;
      std::string Label;
   };

   const std::array<AxisConfig, AxisCount> AxisConfigs = {{
      {"beam", "beam axis"},
      {"thrust", "thrust axis"},
   }};

   struct TrackSummary
   {
      double Eta = 0.0;
      double Phi = 0.0;
   };

   struct EventData
   {
      bool PassEventSelection = true;
      std::vector<float> Px;
      std::vector<float> Py;
      std::vector<float> Pz;
      std::vector<short> Pwflag;
      std::vector<bool> HighPurity;
   };

   struct EventSummary
   {
      int LabMultiplicity = 0;
      std::vector<double> BeamPhi;
      std::vector<double> ThrustPhi;
      std::vector<TrackSummary> BeamTracks;
      std::vector<TrackSummary> ThrustTracks;
   };

   struct CumulantContributions
   {
      double Num2 = 0.0;
      double Den2 = 0.0;
      double Num4 = 0.0;
      double Den4 = 0.0;
      double Num6 = 0.0;
      double Den6 = 0.0;
      double Num8 = 0.0;
      double Den8 = 0.0;
      int Multiplicity = 0;
   };

   struct CorrelatorContribution
   {
      double Num = 0.0;
      double Den = 0.0;
      int Multiplicity = 0;
      bool Valid = false;
   };

   struct AnalysisOptions
   {
      std::string InputFileName;
      std::string OutputFileName;
      std::string TreeName;
      std::string InputFormat;
      long long StartEntry = 0;
      long long EndEntry = -1;
      int MaxEvents = -1;
      int Harmonic = 2;
      bool PrintEntriesOnly = false;
      bool UsePassEventSelection = true;
      bool RequireHighPurity = false;
      bool ThrustChargedOnly = false;
      bool UseWideTailBinning = false;
      double LabPtMin = 0.4;
      double ThrustPtMin = 0.4;
      double SubeventEtaBoundary = 0.5;
      std::vector<int> MultiplicityEdges;
   };

   struct AxisHistograms
   {
      TH1D *EventCount = nullptr;
      TH1D *PairCapableCount = nullptr;
      TH1D *QuadCapableCount = nullptr;
      TH1D *HexCapableCount = nullptr;
      TH1D *OctCapableCount = nullptr;
      TH1D *FullV224CapableCount = nullptr;
      TH1D *ThreeSubV224CapableCount = nullptr;
      TH1D *SumNum2 = nullptr;
      TH1D *SumDen2 = nullptr;
      TH1D *SumNum4 = nullptr;
      TH1D *SumDen4 = nullptr;
      TH1D *SumNum6 = nullptr;
      TH1D *SumDen6 = nullptr;
      TH1D *SumNum8 = nullptr;
      TH1D *SumDen8 = nullptr;
      TH1D *SumNumV224 = nullptr;
      TH1D *SumDenV224 = nullptr;
      TH1D *SumNumV224ThreeSub = nullptr;
      TH1D *SumDenV224ThreeSub = nullptr;
   };

   enum class InputFormat
   {
      Auto,
      Vector,
      Array
   };

   InputFormat ParseInputFormat(const std::string &format)
   {
      if (format == "auto")
         return InputFormat::Auto;
      if (format == "vector")
         return InputFormat::Vector;
      if (format == "array")
         return InputFormat::Array;
      throw std::runtime_error("Unknown --InputFormat=" + format + " (expected auto, vector, or array)");
   }

   bool BranchExists(TTree *tree, const std::string &name)
   {
      return tree != nullptr && tree->GetBranch(name.c_str()) != nullptr;
   }

   bool BranchIsVector(TTree *tree, const std::string &name)
   {
      if (!BranchExists(tree, name))
         return false;

      TBranch *branch = tree->GetBranch(name.c_str());
      TClass *expectedClass = nullptr;
      EDataType expectedType = kOther_t;
      branch->GetExpectedType(expectedClass, expectedType);
      if (expectedClass == nullptr)
         return false;

      const std::string className = expectedClass->GetName();
      return className.find("vector") != std::string::npos;
   }

   void RequireBranch(TTree *tree, const std::string &name)
   {
      if (!BranchExists(tree, name))
         throw std::runtime_error("Required branch \"" + name + "\" is missing");
   }

   void EnableBranch(TTree *tree, const std::string &name)
   {
      if (BranchExists(tree, name))
         tree->SetBranchStatus(name.c_str(), 1);
   }

   class AlephTreeReader
   {
   public:
      AlephTreeReader(TTree *tree, InputFormat requestedFormat, bool requireHighPurity)
         : Tree(tree), RequireHighPurity(requireHighPurity)
      {
         if (Tree == nullptr)
            throw std::runtime_error("Cannot initialize reader with a null TTree");

         Format = ResolveFormat(requestedFormat);
         InitializeBranches();
      }

      const std::string &FormatName() const
      {
         return FormatLabel;
      }

      bool GetEntry(long long entry, EventData &event)
      {
         Tree->GetEntry(entry);
         event = EventData();

         if (HasPassEventSelection)
            event.PassEventSelection = PassEventSelection;

         if (Format == InputFormat::Vector)
            return FillVectorEvent(event);
         return FillArrayEvent(event);
      }

   private:
      InputFormat ResolveFormat(InputFormat requestedFormat)
      {
         if (requestedFormat == InputFormat::Vector || requestedFormat == InputFormat::Array)
            return requestedFormat;

         RequireBranch(Tree, "px");
         return BranchIsVector(Tree, "px") ? InputFormat::Vector : InputFormat::Array;
      }

      void InitializeBranches()
      {
         Tree->SetBranchStatus("*", 0);
         Tree->SetCacheSize(64 * 1024 * 1024);
         Tree->SetCacheLearnEntries(100);

         HasPassEventSelection = BranchExists(Tree, "passEventSelection");
         HasHighPurity = BranchExists(Tree, "highPurity");

         if (RequireHighPurity && !HasHighPurity)
            throw std::runtime_error("--RequireHighPurity was requested, but branch \"highPurity\" is missing");

         if (Format == InputFormat::Vector)
         {
            FormatLabel = "vector";
            RequireBranch(Tree, "px");
            RequireBranch(Tree, "py");
            RequireBranch(Tree, "pz");
            RequireBranch(Tree, "pwflag");

            EnableBranch(Tree, "passEventSelection");
            EnableBranch(Tree, "px");
            EnableBranch(Tree, "py");
            EnableBranch(Tree, "pz");
            EnableBranch(Tree, "pwflag");
            EnableBranch(Tree, "highPurity");

            Tree->SetBranchAddress("px", &VectorPx);
            Tree->SetBranchAddress("py", &VectorPy);
            Tree->SetBranchAddress("pz", &VectorPz);
            Tree->SetBranchAddress("pwflag", &VectorPwflag);
            if (HasPassEventSelection)
               Tree->SetBranchAddress("passEventSelection", &PassEventSelection);
            if (HasHighPurity)
               Tree->SetBranchAddress("highPurity", &VectorHighPurity);
            return;
         }

         FormatLabel = "array";
         RequireBranch(Tree, "nParticle");
         RequireBranch(Tree, "px");
         RequireBranch(Tree, "py");
         RequireBranch(Tree, "pz");
         RequireBranch(Tree, "pwflag");

         EnableBranch(Tree, "passEventSelection");
         EnableBranch(Tree, "nParticle");
         EnableBranch(Tree, "px");
         EnableBranch(Tree, "py");
         EnableBranch(Tree, "pz");
         EnableBranch(Tree, "pwflag");
         EnableBranch(Tree, "highPurity");

         Tree->SetBranchAddress("nParticle", &ArrayNParticle);
         Tree->SetBranchAddress("px", ArrayPx);
         Tree->SetBranchAddress("py", ArrayPy);
         Tree->SetBranchAddress("pz", ArrayPz);
         Tree->SetBranchAddress("pwflag", ArrayPwflag);
         if (HasPassEventSelection)
            Tree->SetBranchAddress("passEventSelection", &PassEventSelection);
         if (HasHighPurity)
            Tree->SetBranchAddress("highPurity", ArrayHighPurity);
      }

      bool FillVectorEvent(EventData &event)
      {
         if (VectorPx == nullptr || VectorPy == nullptr || VectorPz == nullptr || VectorPwflag == nullptr)
            return false;
         if (VectorPx->size() != VectorPy->size() || VectorPx->size() != VectorPz->size() ||
            VectorPx->size() != VectorPwflag->size())
         {
            return false;
         }

         event.Px = *VectorPx;
         event.Py = *VectorPy;
         event.Pz = *VectorPz;
         event.Pwflag = *VectorPwflag;
         event.HighPurity.assign(event.Px.size(), true);
         if (HasHighPurity && VectorHighPurity != nullptr && VectorHighPurity->size() == event.Px.size())
            event.HighPurity.assign(VectorHighPurity->begin(), VectorHighPurity->end());
         return true;
      }

      bool FillArrayEvent(EventData &event)
      {
         if (ArrayNParticle < 0)
            return false;

         const int count = std::min(ArrayNParticle, MaxArrayParticles);
         event.Px.reserve(count);
         event.Py.reserve(count);
         event.Pz.reserve(count);
         event.Pwflag.reserve(count);
         event.HighPurity.reserve(count);

         for (int i = 0; i < count; ++i)
         {
            event.Px.push_back(ArrayPx[i]);
            event.Py.push_back(ArrayPy[i]);
            event.Pz.push_back(ArrayPz[i]);
            event.Pwflag.push_back(ArrayPwflag[i]);
            event.HighPurity.push_back(HasHighPurity ? static_cast<bool>(ArrayHighPurity[i]) : true);
         }
         return true;
      }

      TTree *Tree = nullptr;
      InputFormat Format = InputFormat::Auto;
      std::string FormatLabel = "auto";
      bool RequireHighPurity = false;
      bool HasPassEventSelection = false;
      bool HasHighPurity = false;

      Bool_t PassEventSelection = true;

      std::vector<float> *VectorPx = nullptr;
      std::vector<float> *VectorPy = nullptr;
      std::vector<float> *VectorPz = nullptr;
      std::vector<short> *VectorPwflag = nullptr;
      std::vector<bool> *VectorHighPurity = nullptr;

      Int_t ArrayNParticle = 0;
      Float_t ArrayPx[MaxArrayParticles] = {};
      Float_t ArrayPy[MaxArrayParticles] = {};
      Float_t ArrayPz[MaxArrayParticles] = {};
      Short_t ArrayPwflag[MaxArrayParticles] = {};
      Bool_t ArrayHighPurity[MaxArrayParticles] = {};
   };

   std::vector<MultiplicityBin> ResolveMultiplicityBins(const AnalysisOptions &options)
   {
      std::vector<int> edges = options.MultiplicityEdges;
      if (edges.empty())
      {
         edges = options.UseWideTailBinning
            ? std::vector<int>{0, 10, 15, 20, 25, 30, 35, 40, 45, 999}
            : std::vector<int>{0, 10, 15, 20, 25, 30, 35, 40, 999};
      }

      if (edges.size() < 2)
         throw std::runtime_error("At least two multiplicity bin edges are required");

      std::vector<MultiplicityBin> bins;
      for (std::size_t i = 0; i + 1 < edges.size(); ++i)
      {
         if (edges[i + 1] <= edges[i])
            throw std::runtime_error("Multiplicity bin edges must be strictly increasing");
         bins.push_back({edges[i], edges[i + 1]});
      }
      return bins;
   }

   std::string MultLabel(const MultiplicityBin &bin)
   {
      return std::to_string(bin.Low) + "_" + std::to_string(bin.High);
   }

   std::vector<int> FindMultiplicityBins(int multiplicity, const std::vector<MultiplicityBin> &multBins)
   {
      std::vector<int> bins;
      for (int i = 0; i < static_cast<int>(multBins.size()); ++i)
      {
         if (multiplicity >= multBins[i].Low && multiplicity < multBins[i].High)
            bins.push_back(i);
      }
      return bins;
   }

   bool BuildEventSummary(const EventData &event, const AnalysisOptions &options, EventSummary &summary)
   {
      summary = EventSummary();

      if (options.UsePassEventSelection && !event.PassEventSelection)
         return false;
      if (event.Px.size() != event.Py.size() || event.Px.size() != event.Pz.size() ||
         event.Px.size() != event.Pwflag.size() || event.Px.size() != event.HighPurity.size())
      {
         return false;
      }

      std::vector<TVector3> thrustInput;
      thrustInput.reserve(event.Px.size());
      for (int i = 0; i < static_cast<int>(event.Px.size()); ++i)
      {
         if (options.ThrustChargedOnly && !AlephCumulant::IsChargedPwflag(event.Pwflag[i]))
            continue;
         if (options.RequireHighPurity && !event.HighPurity[i])
            continue;

         const TVector3 momentum(event.Px[i], event.Py[i], event.Pz[i]);
         if (momentum.Mag2() > 0.0)
            thrustInput.push_back(momentum);
      }

      const TVector3 thrustAxis = AlephCumulant::ComputeThrustAxis(thrustInput);
      const bool hasThrustAxis = thrustAxis.Mag2() > 0.0;

      for (int i = 0; i < static_cast<int>(event.Px.size()); ++i)
      {
         if (!AlephCumulant::IsChargedPwflag(event.Pwflag[i]))
            continue;
         if (options.RequireHighPurity && !event.HighPurity[i])
            continue;

         const TVector3 momentum(event.Px[i], event.Py[i], event.Pz[i]);
         if (momentum.Mag2() <= 0.0)
            continue;

         const double labPt = momentum.Pt();
         if (std::isfinite(labPt) && labPt >= options.LabPtMin)
         {
            summary.BeamPhi.push_back(momentum.Phi());
            summary.BeamTracks.push_back({momentum.Eta(), momentum.Phi()});
         }

         if (hasThrustAxis)
         {
            const double thrustPt = AlephCumulant::PtFromAxis(thrustAxis, momentum);
            if (std::isfinite(thrustPt) && thrustPt >= options.ThrustPtMin)
            {
               const double eta = AlephCumulant::EtaFromAxis(thrustAxis, momentum);
               const double phi = AlephCumulant::PhiFromAxis(thrustAxis, momentum);
               if (std::isfinite(eta) && std::isfinite(phi))
               {
                  summary.ThrustPhi.push_back(phi);
                  summary.ThrustTracks.push_back({eta, phi});
               }
            }
         }
      }

      summary.LabMultiplicity = static_cast<int>(summary.BeamPhi.size());
      return true;
   }

   double FallingFactorial(int multiplicity, int order)
   {
      if (multiplicity < order)
         return 0.0;

      double result = 1.0;
      for (int i = 0; i < order; ++i)
         result *= static_cast<double>(multiplicity - i);
      return result;
   }

   void FillOrderedCorrelators(const std::vector<double> &phis, int harmonic,
      std::array<double, MaxHalfCorrelation + 1> &numerators,
      std::array<double, MaxHalfCorrelation + 1> &denominators)
   {
      numerators.fill(0.0);
      denominators.fill(0.0);

      const int multiplicity = static_cast<int>(phis.size());
      const int maxHalf = std::min(MaxHalfCorrelation, multiplicity / 2);
      if (maxHalf <= 0)
         return;

      using Complex = std::complex<double>;
      using Matrix = std::array<std::array<Complex, MaxHalfCorrelation + 1>, MaxHalfCorrelation + 1>;

      Matrix dp = {};
      dp[0][0] = Complex(1.0, 0.0);

      for (double phi : phis)
      {
         const Complex positive = std::polar(1.0, static_cast<double>(harmonic) * phi);
         const Complex negative = std::conj(positive);
         Matrix next = dp;

         for (int plus = 0; plus <= maxHalf; ++plus)
         {
            for (int minus = 0; minus <= maxHalf; ++minus)
            {
               const Complex value = dp[plus][minus];
               if (std::norm(value) <= 0.0)
                  continue;

               if (plus + 1 <= maxHalf)
                  next[plus + 1][minus] += value * positive;
               if (minus + 1 <= maxHalf)
                  next[plus][minus + 1] += value * negative;
            }
         }

         dp = next;
      }

      for (int half = 1; half <= maxHalf; ++half)
      {
         numerators[half] = std::real(dp[half][half]) *
            static_cast<double>(Factorials[half] * Factorials[half]);
         denominators[half] = FallingFactorial(multiplicity, 2 * half);
      }
   }

   CumulantContributions ComputeCumulantContributions(const std::vector<double> &phis, int harmonic)
   {
      CumulantContributions result;
      result.Multiplicity = static_cast<int>(phis.size());

      std::array<double, MaxHalfCorrelation + 1> numerators = {};
      std::array<double, MaxHalfCorrelation + 1> denominators = {};
      FillOrderedCorrelators(phis, harmonic, numerators, denominators);

      result.Num2 = numerators[1];
      result.Den2 = denominators[1];
      result.Num4 = numerators[2];
      result.Den4 = denominators[2];
      result.Num6 = numerators[3];
      result.Den6 = denominators[3];
      result.Num8 = numerators[4];
      result.Den8 = denominators[4];
      return result;
   }

   CorrelatorContribution ComputeFullEventV224(const std::vector<TrackSummary> &tracks)
   {
      CorrelatorContribution result;
      result.Multiplicity = static_cast<int>(tracks.size());
      if (result.Multiplicity < 3)
         return result;

      using Complex = std::complex<double>;
      Complex q2(0.0, 0.0);
      Complex q4(0.0, 0.0);
      for (const TrackSummary &track : tracks)
      {
         q2 += std::polar(1.0, 2.0 * track.Phi);
         q4 += std::polar(1.0, 4.0 * track.Phi);
      }

      const double m = static_cast<double>(result.Multiplicity);
      const Complex numerator = q2 * q2 * std::conj(q4)
         - q4 * std::conj(q4)
         - 2.0 * q2 * std::conj(q2)
         + Complex(2.0 * m, 0.0);

      result.Num = std::real(numerator);
      result.Den = m * (m - 1.0) * (m - 2.0);
      result.Valid = result.Den > 0.0;
      return result;
   }

   int FindSubevent(double eta, double etaBoundary)
   {
      if (eta < -etaBoundary)
         return 0;
      if (eta > etaBoundary)
         return 2;
      return 1;
   }

   CorrelatorContribution ComputeThreeSubeventV224(const std::vector<TrackSummary> &tracks,
      double etaBoundary)
   {
      CorrelatorContribution result;
      result.Multiplicity = static_cast<int>(tracks.size());

      using Complex = std::complex<double>;
      std::array<Complex, SubeventCount> q2 = {};
      std::array<Complex, SubeventCount> q4 = {};
      std::array<int, SubeventCount> counts = {{0, 0, 0}};

      for (const TrackSummary &track : tracks)
      {
         const int index = FindSubevent(track.Eta, etaBoundary);
         q2[index] += std::polar(1.0, 2.0 * track.Phi);
         q4[index] += std::polar(1.0, 4.0 * track.Phi);
         counts[index] += 1;
      }

      if (counts[0] <= 0 || counts[1] <= 0 || counts[2] <= 0)
         return result;

      const Complex numerator =
         q2[0] * q2[1] * std::conj(q4[2]) +
         q2[0] * q2[2] * std::conj(q4[1]) +
         q2[1] * q2[2] * std::conj(q4[0]);

      result.Num = std::real(numerator);
      result.Den = 3.0 * static_cast<double>(counts[0] * counts[1] * counts[2]);
      result.Valid = result.Den > 0.0;
      return result;
   }

   TH1D *MakeHistogram(const std::string &name, const std::string &title,
      const std::vector<MultiplicityBin> &multBins)
   {
      TH1D *hist = new TH1D(name.c_str(), title.c_str(),
         static_cast<int>(multBins.size()), 0.0, static_cast<double>(multBins.size()));
      hist->Sumw2(false);
      for (int i = 0; i < static_cast<int>(multBins.size()); ++i)
         hist->GetXaxis()->SetBinLabel(i + 1, MultLabel(multBins[i]).c_str());
      return hist;
   }

   AxisHistograms MakeAxisHistograms(const AxisConfig &axis, const std::vector<MultiplicityBin> &multBins)
   {
      AxisHistograms h;
      const std::string suffix = "_" + axis.Name;
      const std::string xTitle = ";charged multiplicity bin;";

      h.EventCount = MakeHistogram("hEventCount" + suffix,
         "Selected events, " + axis.Label + xTitle + "events", multBins);
      h.PairCapableCount = MakeHistogram("hPairCapableCount" + suffix,
         "Pair-capable events, " + axis.Label + xTitle + "events", multBins);
      h.QuadCapableCount = MakeHistogram("hQuadCapableCount" + suffix,
         "Four-particle-capable events, " + axis.Label + xTitle + "events", multBins);
      h.HexCapableCount = MakeHistogram("hHexCapableCount" + suffix,
         "Six-particle-capable events, " + axis.Label + xTitle + "events", multBins);
      h.OctCapableCount = MakeHistogram("hOctCapableCount" + suffix,
         "Eight-particle-capable events, " + axis.Label + xTitle + "events", multBins);
      h.FullV224CapableCount = MakeHistogram("hFullV224CapableCount" + suffix,
         "Full-event v224-capable events, " + axis.Label + xTitle + "events", multBins);
      h.ThreeSubV224CapableCount = MakeHistogram("hThreeSubV224CapableCount" + suffix,
         "Three-subevent v224-capable events, " + axis.Label + xTitle + "events", multBins);

      h.SumNum2 = MakeHistogram("hSumNum2" + suffix, "Sum numerator <2>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen2 = MakeHistogram("hSumDen2" + suffix, "Sum denominator <2>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum4 = MakeHistogram("hSumNum4" + suffix, "Sum numerator <4>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen4 = MakeHistogram("hSumDen4" + suffix, "Sum denominator <4>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum6 = MakeHistogram("hSumNum6" + suffix, "Sum numerator <6>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen6 = MakeHistogram("hSumDen6" + suffix, "Sum denominator <6>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum8 = MakeHistogram("hSumNum8" + suffix, "Sum numerator <8>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen8 = MakeHistogram("hSumDen8" + suffix, "Sum denominator <8>, " + axis.Label + xTitle + "sum", multBins);

      h.SumNumV224 = MakeHistogram("hSumNumV224" + suffix,
         "Sum numerator <e^{i(2#phi_{1}+2#phi_{2}-4#phi_{3})}>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDenV224 = MakeHistogram("hSumDenV224" + suffix,
         "Sum denominator v224, " + axis.Label + xTitle + "sum", multBins);
      h.SumNumV224ThreeSub = MakeHistogram("hSumNumV224ThreeSub" + suffix,
         "Sum numerator three-subevent v224, " + axis.Label + xTitle + "sum", multBins);
      h.SumDenV224ThreeSub = MakeHistogram("hSumDenV224ThreeSub" + suffix,
         "Sum denominator three-subevent v224, " + axis.Label + xTitle + "sum", multBins);
      return h;
   }

   void FillCumulantHistograms(AxisHistograms &hist, int bin, const CumulantContributions &contribution)
   {
      if (contribution.Den2 > 0.0)
      {
         hist.PairCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum2->AddBinContent(bin, contribution.Num2);
         hist.SumDen2->AddBinContent(bin, contribution.Den2);
      }
      if (contribution.Den4 > 0.0)
      {
         hist.QuadCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum4->AddBinContent(bin, contribution.Num4);
         hist.SumDen4->AddBinContent(bin, contribution.Den4);
      }
      if (contribution.Den6 > 0.0)
      {
         hist.HexCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum6->AddBinContent(bin, contribution.Num6);
         hist.SumDen6->AddBinContent(bin, contribution.Den6);
      }
      if (contribution.Den8 > 0.0)
      {
         hist.OctCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum8->AddBinContent(bin, contribution.Num8);
         hist.SumDen8->AddBinContent(bin, contribution.Den8);
      }
   }

   void FillV224Histograms(AxisHistograms &hist, int bin,
      const CorrelatorContribution &fullEvent,
      const CorrelatorContribution &threeSubevent)
   {
      if (fullEvent.Valid)
      {
         hist.FullV224CapableCount->AddBinContent(bin, 1.0);
         hist.SumNumV224->AddBinContent(bin, fullEvent.Num);
         hist.SumDenV224->AddBinContent(bin, fullEvent.Den);
      }
      if (threeSubevent.Valid)
      {
         hist.ThreeSubV224CapableCount->AddBinContent(bin, 1.0);
         hist.SumNumV224ThreeSub->AddBinContent(bin, threeSubevent.Num);
         hist.SumDenV224ThreeSub->AddBinContent(bin, threeSubevent.Den);
      }
   }

   void WriteAxisHistograms(const AxisHistograms &hist)
   {
      hist.EventCount->Write();
      hist.PairCapableCount->Write();
      hist.QuadCapableCount->Write();
      hist.HexCapableCount->Write();
      hist.OctCapableCount->Write();
      hist.FullV224CapableCount->Write();
      hist.ThreeSubV224CapableCount->Write();
      hist.SumNum2->Write();
      hist.SumDen2->Write();
      hist.SumNum4->Write();
      hist.SumDen4->Write();
      hist.SumNum6->Write();
      hist.SumDen6->Write();
      hist.SumNum8->Write();
      hist.SumDen8->Write();
      hist.SumNumV224->Write();
      hist.SumDenV224->Write();
      hist.SumNumV224ThreeSub->Write();
      hist.SumDenV224ThreeSub->Write();
   }

   std::string JoinEdges(const std::vector<MultiplicityBin> &bins)
   {
      if (bins.empty())
         return "";

      std::string result = std::to_string(bins.front().Low);
      for (const MultiplicityBin &bin : bins)
         result += "," + std::to_string(bin.High);
      return result;
   }

   AnalysisOptions ParseOptions(int argc, char *argv[])
   {
      CommandLine cl(argc, argv);
      AnalysisOptions options;
      options.InputFileName = cl.Get("Input",
         "/raid5/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root");
      options.OutputFileName = cl.Get("Output", "output/aleph_charged_cumulants.root");
      options.TreeName = cl.Get("Tree", "t");
      options.InputFormat = cl.Get("InputFormat", "auto");
      options.StartEntry = std::max(0LL, cl.GetLongLong("StartEntry", 0));
      options.EndEntry = cl.GetLongLong("EndEntry", -1);
      options.MaxEvents = cl.GetInt("MaxEvents", -1);
      options.Harmonic = cl.GetInt("Harmonic", 2);
      options.PrintEntriesOnly = cl.GetBool("PrintEntries", false);
      options.UsePassEventSelection = cl.GetBool("UsePassEventSelection", true);
      options.RequireHighPurity = cl.GetBool("RequireHighPurity", false);
      options.ThrustChargedOnly = cl.GetBool("ThrustChargedOnly", false);
      options.UseWideTailBinning = cl.GetBool("UseWideTailBinning", false);
      options.LabPtMin = cl.GetDouble("LabPtMin", 0.4);
      options.ThrustPtMin = cl.GetDouble("ThrustPtMin", 0.4);
      options.SubeventEtaBoundary = cl.GetDouble("SubeventEtaBoundary", 0.5);
      if (cl.Has("MultiplicityBins"))
         options.MultiplicityEdges = cl.GetIntVector("MultiplicityBins", "");
      return options;
   }
}

int main(int argc, char *argv[])
{
   try
   {
      const AnalysisOptions options = ParseOptions(argc, argv);
      const std::vector<MultiplicityBin> multBins = ResolveMultiplicityBins(options);

      TFile inputFile(options.InputFileName.c_str(), "READ");
      if (inputFile.IsZombie())
      {
         std::cerr << "Failed to open input file: " << options.InputFileName << std::endl;
         return 1;
      }

      TTree *tree = nullptr;
      inputFile.GetObject(options.TreeName.c_str(), tree);
      if (tree == nullptr)
      {
         std::cerr << "Failed to find tree \"" << options.TreeName
            << "\" in " << options.InputFileName << std::endl;
         return 1;
      }

      if (options.PrintEntriesOnly)
      {
         std::cout << tree->GetEntries() << std::endl;
         return 0;
      }

      AlephTreeReader reader(tree, ParseInputFormat(options.InputFormat), options.RequireHighPurity);
      std::cout << "Reading " << options.InputFileName << " tree " << options.TreeName
         << " with " << reader.FormatName() << " branches" << std::endl;

      const long long totalEntries = tree->GetEntries();
      long long endEntry = options.EndEntry;
      if (endEntry < 0 || endEntry > totalEntries)
         endEntry = totalEntries;
      if (options.MaxEvents >= 0)
         endEntry = std::min(endEntry, options.StartEntry + static_cast<long long>(options.MaxEvents));

      if (options.StartEntry >= endEntry)
      {
         std::cerr << "Requested event range is empty: ["
            << options.StartEntry << ", " << endEntry << ")" << std::endl;
         return 1;
      }

      TFile outputFile(options.OutputFileName.c_str(), "RECREATE");
      if (outputFile.IsZombie())
      {
         std::cerr << "Failed to create output file: " << options.OutputFileName << std::endl;
         return 1;
      }

      std::array<AxisHistograms, AxisCount> histograms;
      for (int axis = 0; axis < AxisCount; ++axis)
         histograms[axis] = MakeAxisHistograms(AxisConfigs[axis], multBins);

      TH1D hProcessed("hProcessedEventCount",
         "Processed event counts;step;events", 8, 0.0, 8.0);
      hProcessed.GetXaxis()->SetBinLabel(1, "scanned");
      hProcessed.GetXaxis()->SetBinLabel(2, "readable");
      hProcessed.GetXaxis()->SetBinLabel(3, "selected");
      hProcessed.GetXaxis()->SetBinLabel(4, "selected in mult bin");
      hProcessed.GetXaxis()->SetBinLabel(5, "four-particle any axis");
      hProcessed.GetXaxis()->SetBinLabel(6, "six-particle any axis");
      hProcessed.GetXaxis()->SetBinLabel(7, "eight-particle any axis");
      hProcessed.GetXaxis()->SetBinLabel(8, "v224 any axis");

      TH1D hMultiplicity("hSelectedMultiplicity",
         "Selected charged-particle multiplicity;N_{ch};events", 120, 0.0, 120.0);
      TH1D hThrustMultiplicity("hSelectedThrustMultiplicity",
         "Selected charged-particle multiplicity around thrust axis;N_{ch};events", 120, 0.0, 120.0);

      EventData event;
      for (long long entry = options.StartEntry; entry < endEntry; ++entry)
      {
         hProcessed.Fill(0.5);
         if (!reader.GetEntry(entry, event))
            continue;

         hProcessed.Fill(1.5);

         EventSummary summary;
         if (!BuildEventSummary(event, options, summary))
            continue;

         hProcessed.Fill(2.5);
         hMultiplicity.Fill(summary.LabMultiplicity);
         hThrustMultiplicity.Fill(static_cast<double>(summary.ThrustPhi.size()));

         const std::vector<int> multMatches = FindMultiplicityBins(summary.LabMultiplicity, multBins);
         if (multMatches.empty())
            continue;

         hProcessed.Fill(3.5);

         const CumulantContributions beam = ComputeCumulantContributions(summary.BeamPhi, options.Harmonic);
         const CumulantContributions thrust = ComputeCumulantContributions(summary.ThrustPhi, options.Harmonic);
         const CorrelatorContribution beamV224 = ComputeFullEventV224(summary.BeamTracks);
         const CorrelatorContribution thrustV224 = ComputeFullEventV224(summary.ThrustTracks);
         const CorrelatorContribution beamV224ThreeSub =
            ComputeThreeSubeventV224(summary.BeamTracks, options.SubeventEtaBoundary);
         const CorrelatorContribution thrustV224ThreeSub =
            ComputeThreeSubeventV224(summary.ThrustTracks, options.SubeventEtaBoundary);

         if (beam.Multiplicity >= 4 || thrust.Multiplicity >= 4)
            hProcessed.Fill(4.5);
         if (beam.Multiplicity >= 6 || thrust.Multiplicity >= 6)
            hProcessed.Fill(5.5);
         if (beam.Multiplicity >= 8 || thrust.Multiplicity >= 8)
            hProcessed.Fill(6.5);
         if (beamV224.Valid || thrustV224.Valid || beamV224ThreeSub.Valid || thrustV224ThreeSub.Valid)
            hProcessed.Fill(7.5);

         for (int multIndex : multMatches)
         {
            const int bin = multIndex + 1;

            histograms[BeamAxisIndex].EventCount->AddBinContent(bin, 1.0);
            FillCumulantHistograms(histograms[BeamAxisIndex], bin, beam);
            FillV224Histograms(histograms[BeamAxisIndex], bin, beamV224, beamV224ThreeSub);

            histograms[ThrustAxisIndex].EventCount->AddBinContent(bin, 1.0);
            FillCumulantHistograms(histograms[ThrustAxisIndex], bin, thrust);
            FillV224Histograms(histograms[ThrustAxisIndex], bin, thrustV224, thrustV224ThreeSub);
         }

         if ((entry - options.StartEntry + 1) % 100000 == 0)
            std::cout << "Processed " << (entry - options.StartEntry + 1)
               << " / " << (endEntry - options.StartEntry) << " entries" << std::endl;
      }

      outputFile.cd();
      hProcessed.Write();
      hMultiplicity.Write();
      hThrustMultiplicity.Write();
      for (const AxisHistograms &axisHistograms : histograms)
         WriteAxisHistograms(axisHistograms);

      const std::string optionSummary =
         "Input=" + options.InputFileName +
         ",Tree=" + options.TreeName +
         ",InputFormat=" + reader.FormatName() +
         ",UsePassEventSelection=" + std::string(options.UsePassEventSelection ? "1" : "0") +
         ",RequireHighPurity=" + std::string(options.RequireHighPurity ? "1" : "0") +
         ",ThrustChargedOnly=" + std::string(options.ThrustChargedOnly ? "1" : "0") +
         ",LabPtMin=" + std::to_string(options.LabPtMin) +
         ",ThrustPtMin=" + std::to_string(options.ThrustPtMin) +
         ",Harmonic=" + std::to_string(options.Harmonic) +
         ",SubeventEtaBoundary=" + std::to_string(options.SubeventEtaBoundary) +
         ",MultiplicityBins=" + JoinEdges(multBins) +
         ",StartEntry=" + std::to_string(options.StartEntry) +
         ",EndEntry=" + std::to_string(endEntry);
      TNamed metadata("AnalysisOptions", optionSummary.c_str());
      metadata.Write();

      TNamed source("SourceNotes",
         "Charged-particle selection follows StudyMult pwflag 0,1,2; cumulant sums are mergeable across chunks.");
      source.Write();

      outputFile.Close();
      std::cout << "Wrote " << options.OutputFileName << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}

