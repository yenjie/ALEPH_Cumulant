#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TBranch.h"
#include "TClass.h"
#include "TFile.h"
#include "TLeaf.h"
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
      bool SelfTest = false;
      double LabPtMin = 0.4;
      double ThrustPtMin = 0.4;
      double SubeventEtaBoundary = 0.5;
      double TwoSubeventEtaBoundary = 0.0;
      double EtaGapMin = 2.0;
      std::vector<int> MultiplicityEdges;
   };

   struct AxisHistograms
   {
      TH1D *EventCount = nullptr;
      TH1D *PairCapableCount = nullptr;
      TH1D *QuadCapableCount = nullptr;
      TH1D *HexCapableCount = nullptr;
      TH1D *OctCapableCount = nullptr;
      TH1D *EtaGapPairCapableCount = nullptr;
      TH1D *TwoSubPairCapableCount = nullptr;
      TH1D *TwoSubQuadCapableCount = nullptr;
      TH1D *TwoSubHexCapableCount = nullptr;
      TH1D *TwoSubOctCapableCount = nullptr;
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
      TH1D *SumNum2EtaGap = nullptr;
      TH1D *SumDen2EtaGap = nullptr;
      TH1D *SumNum2TwoSub = nullptr;
      TH1D *SumDen2TwoSub = nullptr;
      TH1D *SumNum4TwoSub = nullptr;
      TH1D *SumDen4TwoSub = nullptr;
      TH1D *SumNum6TwoSub = nullptr;
      TH1D *SumDen6TwoSub = nullptr;
      TH1D *SumNum8TwoSub = nullptr;
      TH1D *SumDen8TwoSub = nullptr;
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

   std::string LeafTypeName(TTree *tree, const std::string &name)
   {
      if (tree == nullptr)
         return "";
      TLeaf *leaf = tree->GetLeaf(name.c_str());
      if (leaf == nullptr)
         return "";
      return leaf->GetTypeName();
   }

   bool LeafTypeIsFloat(const std::string &typeName)
   {
      return typeName == "Float_t" || typeName == "float" || typeName == "Float32_t";
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
         event = EventData();
         if (Tree->GetEntry(entry) <= 0)
            return false;

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
         ArrayPwflagIsFloat = LeafTypeIsFloat(LeafTypeName(Tree, "pwflag"));
         if (ArrayPwflagIsFloat)
            Tree->SetBranchAddress("pwflag", ArrayPwflagFloat);
         else
            Tree->SetBranchAddress("pwflag", ArrayPwflagShort);
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
            const short pwflag = ArrayPwflagIsFloat
               ? static_cast<short>(std::lround(ArrayPwflagFloat[i]))
               : ArrayPwflagShort[i];
            event.Pwflag.push_back(pwflag);
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
      Short_t ArrayPwflagShort[MaxArrayParticles] = {};
      Float_t ArrayPwflagFloat[MaxArrayParticles] = {};
      bool ArrayPwflagIsFloat = false;
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

   CorrelatorContribution ComputeEtaGapPairCorrelation(const std::vector<TrackSummary> &tracks,
      int harmonic, double minAbsDeltaEta)
   {
      CorrelatorContribution result;
      result.Multiplicity = static_cast<int>(tracks.size());
      if (result.Multiplicity < 2)
         return result;

      for (int i = 0; i < result.Multiplicity; ++i)
      {
         for (int j = i + 1; j < result.Multiplicity; ++j)
         {
            if (std::abs(tracks[i].Eta - tracks[j].Eta) <= minAbsDeltaEta)
               continue;

            result.Num += 2.0 * std::cos(static_cast<double>(harmonic) *
               (tracks[i].Phi - tracks[j].Phi));
            result.Den += 2.0;
         }
      }

      result.Valid = result.Den > 0.0;
      return result;
   }

   using Complex = std::complex<double>;

   std::array<Complex, MaxHalfCorrelation + 1> ElementarySymmetricProducts(
      const std::vector<Complex> &values)
   {
      std::array<Complex, MaxHalfCorrelation + 1> products = {};
      products[0] = Complex(1.0, 0.0);
      for (const Complex &value : values)
      {
         for (int order = MaxHalfCorrelation; order >= 1; --order)
            products[order] += products[order - 1] * value;
      }
      return products;
   }

   CumulantContributions ComputeTwoSubeventCumulantContributions(
      const std::vector<TrackSummary> &tracks, int harmonic, double etaBoundary)
   {
      CumulantContributions result;
      std::vector<Complex> first;
      std::vector<Complex> second;
      first.reserve(tracks.size());
      second.reserve(tracks.size());

      for (const TrackSummary &track : tracks)
      {
         const Complex q = std::polar(1.0, static_cast<double>(harmonic) * track.Phi);
         if (track.Eta < -etaBoundary)
            first.push_back(q);
         else if (track.Eta > etaBoundary)
            second.push_back(q);
      }

      result.Multiplicity = static_cast<int>(std::min(first.size(), second.size()));
      const int maxHalf = std::min(MaxHalfCorrelation, result.Multiplicity);
      if (maxHalf <= 0)
         return result;

      const auto firstProducts = ElementarySymmetricProducts(first);
      const auto secondProducts = ElementarySymmetricProducts(second);
      std::array<double, MaxHalfCorrelation + 1> numerators = {};
      std::array<double, MaxHalfCorrelation + 1> denominators = {};

      for (int half = 1; half <= maxHalf; ++half)
      {
         const double ordering = static_cast<double>(Factorials[half] * Factorials[half]);
         numerators[half] = ordering * std::real(firstProducts[half] * std::conj(secondProducts[half]));
         denominators[half] = FallingFactorial(static_cast<int>(first.size()), half) *
            FallingFactorial(static_cast<int>(second.size()), half);
      }

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

   double BruteForceOrderedNumerator(const std::vector<double> &phis, int harmonic, int half)
   {
      const int multiplicity = static_cast<int>(phis.size());
      const int order = 2 * half;
      if (multiplicity < order)
         return 0.0;

      using Complex = std::complex<double>;
      Complex sum(0.0, 0.0);
      std::vector<bool> used(multiplicity, false);

      std::function<void(int, Complex)> recurse = [&](int depth, Complex value)
      {
         if (depth == order)
         {
            sum += value;
            return;
         }

         const int sign = (depth < half) ? 1 : -1;
         for (int i = 0; i < multiplicity; ++i)
         {
            if (used[i])
               continue;
            used[i] = true;
            recurse(depth + 1, value * std::polar(1.0,
               static_cast<double>(sign * harmonic) * phis[i]));
            used[i] = false;
         }
      };

      recurse(0, Complex(1.0, 0.0));
      return std::real(sum);
   }

   double BruteForceTwoSubeventNumerator(const std::vector<TrackSummary> &tracks,
      int harmonic, int half, double etaBoundary)
   {
      std::vector<double> first;
      std::vector<double> second;
      for (const TrackSummary &track : tracks)
      {
         if (track.Eta < -etaBoundary)
            first.push_back(track.Phi);
         else if (track.Eta > etaBoundary)
            second.push_back(track.Phi);
      }

      if (static_cast<int>(first.size()) < half || static_cast<int>(second.size()) < half)
         return 0.0;

      auto orderedProducts = [harmonic, half](const std::vector<double> &phis, int sign)
      {
         Complex sum(0.0, 0.0);
         std::vector<bool> used(phis.size(), false);
         std::function<void(int, Complex)> recurse = [&](int depth, Complex value)
         {
            if (depth == half)
            {
               sum += value;
               return;
            }
            for (int i = 0; i < static_cast<int>(phis.size()); ++i)
            {
               if (used[i])
                  continue;
               used[i] = true;
               recurse(depth + 1, value * std::polar(1.0,
                  static_cast<double>(sign * harmonic) * phis[i]));
               used[i] = false;
            }
         };
         recurse(0, Complex(1.0, 0.0));
         return sum;
      };

      return std::real(orderedProducts(first, +1) * orderedProducts(second, -1));
   }

   double BruteForceV224Numerator(const std::vector<TrackSummary> &tracks)
   {
      using Complex = std::complex<double>;
      Complex sum(0.0, 0.0);
      const int multiplicity = static_cast<int>(tracks.size());
      for (int i = 0; i < multiplicity; ++i)
      {
         for (int j = 0; j < multiplicity; ++j)
         {
            if (j == i)
               continue;
            for (int k = 0; k < multiplicity; ++k)
            {
               if (k == i || k == j)
                  continue;
               sum += std::polar(1.0,
                  2.0 * tracks[i].Phi + 2.0 * tracks[j].Phi - 4.0 * tracks[k].Phi);
            }
         }
      }
      return std::real(sum);
   }

   bool NearlyEqual(double lhs, double rhs, double scale)
   {
      const double tolerance = 1e-9 * std::max(1.0, scale);
      return std::abs(lhs - rhs) <= tolerance;
   }

   bool RunCorrelatorSelfTest()
   {
      for (int harmonic : {1, 2, 3})
      {
         for (int multiplicity = 0; multiplicity <= 8; ++multiplicity)
         {
            std::vector<double> phis;
            for (int i = 0; i < multiplicity; ++i)
               phis.push_back(0.17 + 0.31 * i + 0.013 * i * i + 0.07 * harmonic);

            std::array<double, MaxHalfCorrelation + 1> numerators = {};
            std::array<double, MaxHalfCorrelation + 1> denominators = {};
            FillOrderedCorrelators(phis, harmonic, numerators, denominators);

            for (int half = 1; half <= std::min(MaxHalfCorrelation, multiplicity / 2); ++half)
            {
               const double brute = BruteForceOrderedNumerator(phis, harmonic, half);
               const double expectedDenominator = FallingFactorial(multiplicity, 2 * half);
               if (!NearlyEqual(numerators[half], brute, expectedDenominator) ||
                  !NearlyEqual(denominators[half], expectedDenominator, expectedDenominator))
               {
                  std::cerr << "Self-test failed for harmonic=" << harmonic
                     << " multiplicity=" << multiplicity
                     << " half=" << half
                     << " numerator=" << numerators[half]
                     << " brute=" << brute
                     << " denominator=" << denominators[half]
                     << " expectedDenominator=" << expectedDenominator << std::endl;
                  return false;
               }
            }
         }
      }

      for (int harmonic : {1, 2, 3})
      {
         for (int multiplicity = 2; multiplicity <= 10; ++multiplicity)
         {
            std::vector<TrackSummary> tracks;
            for (int i = 0; i < multiplicity; ++i)
               tracks.push_back({-1.2 + 2.4 * (i + 0.5) / multiplicity,
                  0.09 + 0.23 * i + 0.017 * i * i + 0.05 * harmonic});

            const CorrelatorContribution etaGap =
               ComputeEtaGapPairCorrelation(tracks, harmonic, 0.7);
            double bruteEtaGapNum = 0.0;
            double bruteEtaGapDen = 0.0;
            for (int i = 0; i < multiplicity; ++i)
            {
               for (int j = 0; j < multiplicity; ++j)
               {
                  if (i == j || std::abs(tracks[i].Eta - tracks[j].Eta) <= 0.7)
                     continue;
                  bruteEtaGapNum += std::cos(static_cast<double>(harmonic) *
                     (tracks[i].Phi - tracks[j].Phi));
                  bruteEtaGapDen += 1.0;
               }
            }
            const double etaGapScale = std::max(1.0, bruteEtaGapDen);
            if (!NearlyEqual(etaGap.Num, bruteEtaGapNum, etaGapScale) ||
               !NearlyEqual(etaGap.Den, bruteEtaGapDen, etaGapScale) ||
               etaGap.Valid != (bruteEtaGapDen > 0.0))
            {
               std::cerr << "Self-test failed for eta-gap pair harmonic=" << harmonic
                  << " multiplicity=" << multiplicity
                  << " numerator=" << etaGap.Num
                  << " brute=" << bruteEtaGapNum
                  << " denominator=" << etaGap.Den
                  << " expectedDenominator=" << bruteEtaGapDen << std::endl;
               return false;
            }

            const CumulantContributions production =
               ComputeTwoSubeventCumulantContributions(tracks, harmonic, 0.0);
            const std::array<double, MaxHalfCorrelation + 1> nums = {{
               0.0, production.Num2, production.Num4, production.Num6, production.Num8}};
            const std::array<double, MaxHalfCorrelation + 1> dens = {{
               0.0, production.Den2, production.Den4, production.Den6, production.Den8}};

            for (int half = 1; half <= std::min(MaxHalfCorrelation, multiplicity / 2); ++half)
            {
               int firstCount = 0;
               int secondCount = 0;
               for (const TrackSummary &track : tracks)
               {
                  if (track.Eta < 0.0)
                     firstCount += 1;
                  else if (track.Eta > 0.0)
                     secondCount += 1;
               }

               const double brute = BruteForceTwoSubeventNumerator(tracks, harmonic, half, 0.0);
               const double expectedDenominator = FallingFactorial(firstCount, half) *
                  FallingFactorial(secondCount, half);
               const double scale = std::max(1.0, expectedDenominator);
               if (!NearlyEqual(dens[half], expectedDenominator, scale) ||
                  (dens[half] > 0.0 && !NearlyEqual(nums[half], brute, scale)))
               {
                  std::cerr << "Self-test failed for two-subevent harmonic=" << harmonic
                     << " multiplicity=" << multiplicity
                     << " half=" << half
                     << " numerator=" << nums[half]
                     << " brute=" << brute
                     << " denominator=" << dens[half]
                     << " expectedDenominator=" << expectedDenominator << std::endl;
                  return false;
               }
            }
         }
      }

      for (int multiplicity = 3; multiplicity <= 8; ++multiplicity)
      {
         std::vector<TrackSummary> tracks;
         for (int i = 0; i < multiplicity; ++i)
            tracks.push_back({-1.0 + 0.3 * i, 0.11 + 0.37 * i + 0.019 * i * i});

         const CorrelatorContribution production = ComputeFullEventV224(tracks);
         const double brute = BruteForceV224Numerator(tracks);
         const double expectedDenominator = FallingFactorial(multiplicity, 3);
         if (!production.Valid || !NearlyEqual(production.Num, brute, expectedDenominator) ||
            !NearlyEqual(production.Den, expectedDenominator, expectedDenominator))
         {
            std::cerr << "Self-test failed for v224 multiplicity=" << multiplicity
               << " numerator=" << production.Num
               << " brute=" << brute
               << " denominator=" << production.Den
               << " expectedDenominator=" << expectedDenominator << std::endl;
            return false;
         }
      }

      std::cout << "Correlator self-test passed" << std::endl;
      return true;
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
      const std::string xTitle = ";N_{trk}^{offline};";

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
      h.EtaGapPairCapableCount = MakeHistogram("hEtaGapPairCapableCount" + suffix,
         "Eta-gap pair-capable events, " + axis.Label + xTitle + "events", multBins);
      h.TwoSubPairCapableCount = MakeHistogram("hTwoSubPairCapableCount" + suffix,
         "Two-subevent pair-capable events, " + axis.Label + xTitle + "events", multBins);
      h.TwoSubQuadCapableCount = MakeHistogram("hTwoSubQuadCapableCount" + suffix,
         "Two-subevent four-particle-capable events, " + axis.Label + xTitle + "events", multBins);
      h.TwoSubHexCapableCount = MakeHistogram("hTwoSubHexCapableCount" + suffix,
         "Two-subevent six-particle-capable events, " + axis.Label + xTitle + "events", multBins);
      h.TwoSubOctCapableCount = MakeHistogram("hTwoSubOctCapableCount" + suffix,
         "Two-subevent eight-particle-capable events, " + axis.Label + xTitle + "events", multBins);
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
      h.SumNum2EtaGap = MakeHistogram("hSumNum2EtaGap" + suffix,
         "Sum numerator <2> with |#Delta#eta| gap, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen2EtaGap = MakeHistogram("hSumDen2EtaGap" + suffix,
         "Sum denominator <2> with |#Delta#eta| gap, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum2TwoSub = MakeHistogram("hSumNum2TwoSub" + suffix,
         "Sum two-subevent numerator <2>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen2TwoSub = MakeHistogram("hSumDen2TwoSub" + suffix,
         "Sum two-subevent denominator <2>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum4TwoSub = MakeHistogram("hSumNum4TwoSub" + suffix,
         "Sum two-subevent numerator <4>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen4TwoSub = MakeHistogram("hSumDen4TwoSub" + suffix,
         "Sum two-subevent denominator <4>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum6TwoSub = MakeHistogram("hSumNum6TwoSub" + suffix,
         "Sum two-subevent numerator <6>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen6TwoSub = MakeHistogram("hSumDen6TwoSub" + suffix,
         "Sum two-subevent denominator <6>, " + axis.Label + xTitle + "sum", multBins);
      h.SumNum8TwoSub = MakeHistogram("hSumNum8TwoSub" + suffix,
         "Sum two-subevent numerator <8>, " + axis.Label + xTitle + "sum", multBins);
      h.SumDen8TwoSub = MakeHistogram("hSumDen8TwoSub" + suffix,
         "Sum two-subevent denominator <8>, " + axis.Label + xTitle + "sum", multBins);

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

   void FillEtaGapPairHistograms(AxisHistograms &hist, int bin,
      const CorrelatorContribution &contribution)
   {
      if (contribution.Valid)
      {
         hist.EtaGapPairCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum2EtaGap->AddBinContent(bin, contribution.Num);
         hist.SumDen2EtaGap->AddBinContent(bin, contribution.Den);
      }
   }

   void FillTwoSubeventCumulantHistograms(AxisHistograms &hist, int bin,
      const CumulantContributions &contribution)
   {
      if (contribution.Den2 > 0.0)
      {
         hist.TwoSubPairCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum2TwoSub->AddBinContent(bin, contribution.Num2);
         hist.SumDen2TwoSub->AddBinContent(bin, contribution.Den2);
      }
      if (contribution.Den4 > 0.0)
      {
         hist.TwoSubQuadCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum4TwoSub->AddBinContent(bin, contribution.Num4);
         hist.SumDen4TwoSub->AddBinContent(bin, contribution.Den4);
      }
      if (contribution.Den6 > 0.0)
      {
         hist.TwoSubHexCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum6TwoSub->AddBinContent(bin, contribution.Num6);
         hist.SumDen6TwoSub->AddBinContent(bin, contribution.Den6);
      }
      if (contribution.Den8 > 0.0)
      {
         hist.TwoSubOctCapableCount->AddBinContent(bin, 1.0);
         hist.SumNum8TwoSub->AddBinContent(bin, contribution.Num8);
         hist.SumDen8TwoSub->AddBinContent(bin, contribution.Den8);
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
      hist.EtaGapPairCapableCount->Write();
      hist.TwoSubPairCapableCount->Write();
      hist.TwoSubQuadCapableCount->Write();
      hist.TwoSubHexCapableCount->Write();
      hist.TwoSubOctCapableCount->Write();
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
      hist.SumNum2EtaGap->Write();
      hist.SumDen2EtaGap->Write();
      hist.SumNum2TwoSub->Write();
      hist.SumDen2TwoSub->Write();
      hist.SumNum4TwoSub->Write();
      hist.SumDen4TwoSub->Write();
      hist.SumNum6TwoSub->Write();
      hist.SumDen6TwoSub->Write();
      hist.SumNum8TwoSub->Write();
      hist.SumDen8TwoSub->Write();
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
      options.SelfTest = cl.GetBool("SelfTest", false);
      options.LabPtMin = cl.GetDouble("LabPtMin", 0.4);
      options.ThrustPtMin = cl.GetDouble("ThrustPtMin", 0.4);
      options.SubeventEtaBoundary = cl.GetDouble("SubeventEtaBoundary", 0.5);
      options.TwoSubeventEtaBoundary = cl.GetDouble("TwoSubeventEtaBoundary", 0.0);
      options.EtaGapMin = cl.GetDouble("EtaGapMin", 2.0);
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
      if (options.SelfTest)
         return RunCorrelatorSelfTest() ? 0 : 1;

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
         const CorrelatorContribution beamEtaGap = ComputeEtaGapPairCorrelation(
            summary.BeamTracks, options.Harmonic, options.EtaGapMin);
         const CorrelatorContribution thrustEtaGap = ComputeEtaGapPairCorrelation(
            summary.ThrustTracks, options.Harmonic, options.EtaGapMin);
         const CumulantContributions beamTwoSub = ComputeTwoSubeventCumulantContributions(
            summary.BeamTracks, options.Harmonic, options.TwoSubeventEtaBoundary);
         const CumulantContributions thrustTwoSub = ComputeTwoSubeventCumulantContributions(
            summary.ThrustTracks, options.Harmonic, options.TwoSubeventEtaBoundary);
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
            FillEtaGapPairHistograms(histograms[BeamAxisIndex], bin, beamEtaGap);
            FillTwoSubeventCumulantHistograms(histograms[BeamAxisIndex], bin, beamTwoSub);
            FillV224Histograms(histograms[BeamAxisIndex], bin, beamV224, beamV224ThreeSub);

            histograms[ThrustAxisIndex].EventCount->AddBinContent(bin, 1.0);
            FillCumulantHistograms(histograms[ThrustAxisIndex], bin, thrust);
            FillEtaGapPairHistograms(histograms[ThrustAxisIndex], bin, thrustEtaGap);
            FillTwoSubeventCumulantHistograms(histograms[ThrustAxisIndex], bin, thrustTwoSub);
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
         ",TwoSubeventEtaBoundary=" + std::to_string(options.TwoSubeventEtaBoundary) +
         ",EtaGapMin=" + std::to_string(options.EtaGapMin) +
         ",MultiplicityBins=" + JoinEdges(multBins) +
         ",StartEntry=" + std::to_string(options.StartEntry) +
         ",EndEntry=" + std::to_string(endEntry);
      TNamed metadata("AnalysisOptions", optionSummary.c_str());
      metadata.Write();

      TNamed source("SourceNotes",
         "Charged-particle selection follows StudyMult pwflag 0,1,2; all-particle, eta-gap pair, and two-subevent cumulant sums are mergeable across chunks.");
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

