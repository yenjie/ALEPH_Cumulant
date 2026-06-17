#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1D.h"
#include "TKey.h"
#include "TNamed.h"
#include "TObject.h"

#include "CommandLine.h"

namespace
{
   using HistMap = std::map<std::string, std::unique_ptr<TH1D>>;

   struct AxisSummary
   {
      std::string Name;
      std::string Label;
   };

   const AxisSummary Axes[2] = {
      {"beam", "beam axis"},
      {"thrust", "thrust axis"},
   };

   std::vector<std::string> CleanInputList(const std::vector<std::string> &raw)
   {
      std::vector<std::string> result;
      for (const std::string &item : raw)
      {
         if (!item.empty())
            result.push_back(item);
      }
      return result;
   }

   std::string Join(const std::vector<std::string> &items, const std::string &separator)
   {
      std::string result;
      for (std::size_t i = 0; i < items.size(); ++i)
      {
         if (i != 0)
            result += separator;
         result += items[i];
      }
      return result;
   }

   void AddHistogram(HistMap &target, const TH1D &source)
   {
      const std::string name = source.GetName();
      auto iter = target.find(name);
      if (iter == target.end())
      {
         TH1D *clone = static_cast<TH1D *>(source.Clone(name.c_str()));
         clone->SetDirectory(nullptr);
         target[name].reset(clone);
      }
      else
      {
         iter->second->Add(&source);
      }
   }

   HistMap ReadHistograms(const std::string &fileName)
   {
      TFile inputFile(fileName.c_str(), "READ");
      if (inputFile.IsZombie())
         throw std::runtime_error("Failed to open input file: " + fileName);

      HistMap result;
      TIter nextKey(inputFile.GetListOfKeys());
      while (TKey *key = static_cast<TKey *>(nextKey()))
      {
         std::unique_ptr<TObject> object(key->ReadObj());
         if (!object || !object->InheritsFrom(TH1D::Class()))
            continue;
         AddHistogram(result, *static_cast<TH1D *>(object.get()));
      }
      return result;
   }

   HistMap SumHistograms(const std::vector<HistMap> &inputs, int skipIndex = -1)
   {
      HistMap result;
      for (int i = 0; i < static_cast<int>(inputs.size()); ++i)
      {
         if (i == skipIndex)
            continue;
         for (const auto &item : inputs[i])
            AddHistogram(result, *item.second);
      }
      return result;
   }

   const TH1D *GetHistogram(const HistMap &histograms, const std::string &name, bool required = true)
   {
      auto iter = histograms.find(name);
      if (iter == histograms.end())
      {
         if (required)
            throw std::runtime_error("Missing histogram " + name);
         return nullptr;
      }
      return iter->second.get();
   }

   std::unique_ptr<TH1D> CloneEmpty(const TH1D &source,
      const std::string &name, const std::string &title)
   {
      std::unique_ptr<TH1D> hist(static_cast<TH1D *>(source.Clone(name.c_str())));
      hist->SetTitle(title.c_str());
      hist->Reset("ICES");
      hist->SetDirectory(nullptr);
      return hist;
   }

   double Ratio(const TH1D *num, const TH1D *den, int bin)
   {
      if (num == nullptr || den == nullptr)
         return 0.0;
      const double denominator = den->GetBinContent(bin);
      if (denominator == 0.0)
         return 0.0;
      return num->GetBinContent(bin) / denominator;
   }

   bool HasRatio(const TH1D *num, const TH1D *den, int bin)
   {
      return num != nullptr && den != nullptr && den->GetBinContent(bin) != 0.0;
   }

   void SetIfFinite(TH1D &hist, int bin, double value)
   {
      if (std::isfinite(value))
         hist.SetBinContent(bin, value);
   }

   void AddOutput(std::vector<std::unique_ptr<TH1D>> &outputs, std::unique_ptr<TH1D> hist)
   {
      outputs.push_back(std::move(hist));
   }

   void BuildAxisSummary(const HistMap &input, const AxisSummary &axis,
      std::vector<std::unique_ptr<TH1D>> &outputs)
   {
      const std::string suffix = "_" + axis.Name;
      const TH1D *sumNum2 = GetHistogram(input, "hSumNum2" + suffix);
      const TH1D *sumDen2 = GetHistogram(input, "hSumDen2" + suffix);
      const TH1D *sumNum4 = GetHistogram(input, "hSumNum4" + suffix);
      const TH1D *sumDen4 = GetHistogram(input, "hSumDen4" + suffix);
      const TH1D *sumNum6 = GetHistogram(input, "hSumNum6" + suffix);
      const TH1D *sumDen6 = GetHistogram(input, "hSumDen6" + suffix);
      const TH1D *sumNum8 = GetHistogram(input, "hSumNum8" + suffix);
      const TH1D *sumDen8 = GetHistogram(input, "hSumDen8" + suffix);

      auto corr2 = CloneEmpty(*sumDen2, "hCorr2" + suffix,
         "<2> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<2>");
      auto corr4 = CloneEmpty(*sumDen2, "hCorr4" + suffix,
         "<4> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<4>");
      auto corr6 = CloneEmpty(*sumDen2, "hCorr6" + suffix,
         "<6> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<6>");
      auto corr8 = CloneEmpty(*sumDen2, "hCorr8" + suffix,
         "<8> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<8>");

      auto c2 = CloneEmpty(*sumDen2, "hC2_2" + suffix,
         "c_{2}{2}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{2}");
      auto c4 = CloneEmpty(*sumDen2, "hC2_4" + suffix,
         "c_{2}{4}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{4}");
      auto c6 = CloneEmpty(*sumDen2, "hC2_6" + suffix,
         "c_{2}{6}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{6}");
      auto c8 = CloneEmpty(*sumDen2, "hC2_8" + suffix,
         "c_{2}{8}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{8}");

      auto v22 = CloneEmpty(*sumDen2, "hV2_2" + suffix,
         "v_{2}{2}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{2}");
      auto v24 = CloneEmpty(*sumDen2, "hV2_4" + suffix,
         "v_{2}{4}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{4}");
      auto v26 = CloneEmpty(*sumDen2, "hV2_6" + suffix,
         "v_{2}{6}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{6}");
      auto v28 = CloneEmpty(*sumDen2, "hV2_8" + suffix,
         "v_{2}{8}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{8}");

      for (int bin = 1; bin <= sumDen2->GetNbinsX(); ++bin)
      {
         const bool has2 = HasRatio(sumNum2, sumDen2, bin);
         const bool has4 = HasRatio(sumNum4, sumDen4, bin);
         const bool has6 = HasRatio(sumNum6, sumDen6, bin);
         const bool has8 = HasRatio(sumNum8, sumDen8, bin);

         const double two = Ratio(sumNum2, sumDen2, bin);
         const double four = Ratio(sumNum4, sumDen4, bin);
         const double six = Ratio(sumNum6, sumDen6, bin);
         const double eight = Ratio(sumNum8, sumDen8, bin);

         if (has2)
         {
            SetIfFinite(*corr2, bin, two);
            SetIfFinite(*c2, bin, two);
            if (two >= 0.0)
               SetIfFinite(*v22, bin, std::sqrt(two));
         }
         if (has2 && has4)
         {
            SetIfFinite(*corr4, bin, four);
            const double cumulant4 = four - 2.0 * two * two;
            SetIfFinite(*c4, bin, cumulant4);
            if (cumulant4 < 0.0)
               SetIfFinite(*v24, bin, std::pow(-cumulant4, 0.25));
         }
         if (has2 && has4 && has6)
         {
            SetIfFinite(*corr6, bin, six);
            const double cumulant6 = six - 9.0 * four * two + 12.0 * two * two * two;
            SetIfFinite(*c6, bin, cumulant6);
            if (cumulant6 > 0.0)
               SetIfFinite(*v26, bin, std::pow(cumulant6 / 4.0, 1.0 / 6.0));
         }
         if (has2 && has4 && has6 && has8)
         {
            SetIfFinite(*corr8, bin, eight);
            const double cumulant8 = eight
               - 16.0 * six * two
               - 18.0 * four * four
               + 144.0 * four * two * two
               - 144.0 * two * two * two * two;
            SetIfFinite(*c8, bin, cumulant8);
            if (cumulant8 < 0.0)
               SetIfFinite(*v28, bin, std::pow(-cumulant8 / 33.0, 1.0 / 8.0));
         }
      }

      AddOutput(outputs, std::move(corr2));
      AddOutput(outputs, std::move(corr4));
      AddOutput(outputs, std::move(corr6));
      AddOutput(outputs, std::move(corr8));
      AddOutput(outputs, std::move(c2));
      AddOutput(outputs, std::move(c4));
      AddOutput(outputs, std::move(c6));
      AddOutput(outputs, std::move(c8));
      AddOutput(outputs, std::move(v22));
      AddOutput(outputs, std::move(v24));
      AddOutput(outputs, std::move(v26));
      AddOutput(outputs, std::move(v28));

      const TH1D *sumNum2EtaGap = GetHistogram(input, "hSumNum2EtaGap" + suffix, false);
      const TH1D *sumDen2EtaGap = GetHistogram(input, "hSumDen2EtaGap" + suffix, false);
      if (sumNum2EtaGap != nullptr && sumDen2EtaGap != nullptr)
      {
         auto corr2EtaGap = CloneEmpty(*sumDen2EtaGap, "hCorr2EtaGap" + suffix,
            "<2> correlation with |#Delta#eta| gap, " + axis.Label +
            ";lab selected charged multiplicity bin;<2>_{|#Delta#eta| gap}");
         auto v22EtaGap = CloneEmpty(*sumDen2EtaGap, "hV2_2EtaGap" + suffix,
            "v_{2}{2} with |#Delta#eta| gap, " + axis.Label +
            ";lab selected charged multiplicity bin;v_{2}{2}_{|#Delta#eta| gap}");
         auto v22EtaGapRatio = CloneEmpty(*sumDen2EtaGap, "hV2_2EtaGapOverInclusive" + suffix,
            "v_{2}{2, |#Delta#eta| gap} / v_{2}{2}, " + axis.Label +
            ";lab selected charged multiplicity bin;v_{2}{2}_{|#Delta#eta| gap}/v_{2}{2}");
         for (int bin = 1; bin <= sumDen2EtaGap->GetNbinsX(); ++bin)
         {
            if (!HasRatio(sumNum2EtaGap, sumDen2EtaGap, bin))
               continue;

            const bool hasInclusive = HasRatio(sumNum2, sumDen2, bin);
            const double twoInclusive = hasInclusive ? Ratio(sumNum2, sumDen2, bin) : 0.0;
            const double twoEtaGap = Ratio(sumNum2EtaGap, sumDen2EtaGap, bin);
            SetIfFinite(*corr2EtaGap, bin, twoEtaGap);
            if (twoEtaGap >= 0.0)
               SetIfFinite(*v22EtaGap, bin, std::sqrt(twoEtaGap));
            if (hasInclusive && twoInclusive > 0.0 && twoEtaGap >= 0.0)
               SetIfFinite(*v22EtaGapRatio, bin, std::sqrt(twoEtaGap / twoInclusive));
         }
         AddOutput(outputs, std::move(corr2EtaGap));
         AddOutput(outputs, std::move(v22EtaGap));
         AddOutput(outputs, std::move(v22EtaGapRatio));
      }

      const TH1D *sumNumV224 = GetHistogram(input, "hSumNumV224" + suffix, false);
      const TH1D *sumDenV224 = GetHistogram(input, "hSumDenV224" + suffix, false);
      const TH1D *sumNumV224ThreeSub = GetHistogram(input, "hSumNumV224ThreeSub" + suffix, false);
      const TH1D *sumDenV224ThreeSub = GetHistogram(input, "hSumDenV224ThreeSub" + suffix, false);

      if (sumNumV224 != nullptr && sumDenV224 != nullptr)
      {
         auto v224 = CloneEmpty(*sumDenV224, "hV224" + suffix,
            "<e^{i(2#phi_{1}+2#phi_{2}-4#phi_{3})}>, " + axis.Label +
            ";lab selected charged multiplicity bin;v224");
         for (int bin = 1; bin <= sumDenV224->GetNbinsX(); ++bin)
         {
            if (HasRatio(sumNumV224, sumDenV224, bin))
               SetIfFinite(*v224, bin, Ratio(sumNumV224, sumDenV224, bin));
         }
         AddOutput(outputs, std::move(v224));
      }

      if (sumNumV224ThreeSub != nullptr && sumDenV224ThreeSub != nullptr)
      {
         auto v224ThreeSub = CloneEmpty(*sumDenV224ThreeSub, "hV224ThreeSub" + suffix,
            "Three-subevent <e^{i(2#phi_{1}+2#phi_{2}-4#phi_{3})}>, " + axis.Label +
            ";lab selected charged multiplicity bin;v224");
         for (int bin = 1; bin <= sumDenV224ThreeSub->GetNbinsX(); ++bin)
         {
            if (HasRatio(sumNumV224ThreeSub, sumDenV224ThreeSub, bin))
               SetIfFinite(*v224ThreeSub, bin, Ratio(sumNumV224ThreeSub, sumDenV224ThreeSub, bin));
         }
         AddOutput(outputs, std::move(v224ThreeSub));
      }
   }

   void BuildTwoSubeventAxisSummary(const HistMap &input, const AxisSummary &axis,
      std::vector<std::unique_ptr<TH1D>> &outputs)
   {
      const std::string suffix = "_" + axis.Name;
      const TH1D *sumNum2 = GetHistogram(input, "hSumNum2TwoSub" + suffix, false);
      const TH1D *sumDen2 = GetHistogram(input, "hSumDen2TwoSub" + suffix, false);
      const TH1D *sumNum4 = GetHistogram(input, "hSumNum4TwoSub" + suffix, false);
      const TH1D *sumDen4 = GetHistogram(input, "hSumDen4TwoSub" + suffix, false);
      const TH1D *sumNum6 = GetHistogram(input, "hSumNum6TwoSub" + suffix, false);
      const TH1D *sumDen6 = GetHistogram(input, "hSumDen6TwoSub" + suffix, false);
      const TH1D *sumNum8 = GetHistogram(input, "hSumNum8TwoSub" + suffix, false);
      const TH1D *sumDen8 = GetHistogram(input, "hSumDen8TwoSub" + suffix, false);

      if (sumNum2 == nullptr || sumDen2 == nullptr)
         return;

      auto corr2 = CloneEmpty(*sumDen2, "hCorr2TwoSub" + suffix,
         "Two-subevent <2> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<2>_{2sub}");
      auto corr4 = CloneEmpty(*sumDen2, "hCorr4TwoSub" + suffix,
         "Two-subevent <4> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<4>_{2sub}");
      auto corr6 = CloneEmpty(*sumDen2, "hCorr6TwoSub" + suffix,
         "Two-subevent <6> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<6>_{2sub}");
      auto corr8 = CloneEmpty(*sumDen2, "hCorr8TwoSub" + suffix,
         "Two-subevent <8> correlation, " + axis.Label + ";lab selected charged multiplicity bin;<8>_{2sub}");

      auto c2 = CloneEmpty(*sumDen2, "hC2TwoSub_2" + suffix,
         "Two-subevent c_{2}{2}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{2}_{2sub}");
      auto c4 = CloneEmpty(*sumDen2, "hC2TwoSub_4" + suffix,
         "Two-subevent c_{2}{4}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{4}_{2sub}");
      auto c6 = CloneEmpty(*sumDen2, "hC2TwoSub_6" + suffix,
         "Two-subevent c_{2}{6}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{6}_{2sub}");
      auto c8 = CloneEmpty(*sumDen2, "hC2TwoSub_8" + suffix,
         "Two-subevent c_{2}{8}, " + axis.Label + ";lab selected charged multiplicity bin;c_{2}{8}_{2sub}");

      auto v22 = CloneEmpty(*sumDen2, "hV2TwoSub_2" + suffix,
         "Two-subevent v_{2}{2}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{2}_{2sub}");
      auto v24 = CloneEmpty(*sumDen2, "hV2TwoSub_4" + suffix,
         "Two-subevent v_{2}{4}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{4}_{2sub}");
      auto v26 = CloneEmpty(*sumDen2, "hV2TwoSub_6" + suffix,
         "Two-subevent v_{2}{6}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{6}_{2sub}");
      auto v28 = CloneEmpty(*sumDen2, "hV2TwoSub_8" + suffix,
         "Two-subevent v_{2}{8}, " + axis.Label + ";lab selected charged multiplicity bin;v_{2}{8}_{2sub}");

      for (int bin = 1; bin <= sumDen2->GetNbinsX(); ++bin)
      {
         const bool has2 = HasRatio(sumNum2, sumDen2, bin);
         const bool has4 = HasRatio(sumNum4, sumDen4, bin);
         const bool has6 = HasRatio(sumNum6, sumDen6, bin);
         const bool has8 = HasRatio(sumNum8, sumDen8, bin);

         const double two = Ratio(sumNum2, sumDen2, bin);
         const double four = Ratio(sumNum4, sumDen4, bin);
         const double six = Ratio(sumNum6, sumDen6, bin);
         const double eight = Ratio(sumNum8, sumDen8, bin);

         if (has2)
         {
            SetIfFinite(*corr2, bin, two);
            SetIfFinite(*c2, bin, two);
            if (two >= 0.0)
               SetIfFinite(*v22, bin, std::sqrt(two));
         }
         if (has2 && has4)
         {
            SetIfFinite(*corr4, bin, four);
            const double cumulant4 = four - 2.0 * two * two;
            SetIfFinite(*c4, bin, cumulant4);
            if (cumulant4 < 0.0)
               SetIfFinite(*v24, bin, std::pow(-cumulant4, 0.25));
         }
         if (has2 && has4 && has6)
         {
            SetIfFinite(*corr6, bin, six);
            const double cumulant6 = six - 9.0 * four * two + 12.0 * two * two * two;
            SetIfFinite(*c6, bin, cumulant6);
            if (cumulant6 > 0.0)
               SetIfFinite(*v26, bin, std::pow(cumulant6 / 4.0, 1.0 / 6.0));
         }
         if (has2 && has4 && has6 && has8)
         {
            SetIfFinite(*corr8, bin, eight);
            const double cumulant8 = eight
               - 16.0 * six * two
               - 18.0 * four * four
               + 144.0 * four * two * two
               - 144.0 * two * two * two * two;
            SetIfFinite(*c8, bin, cumulant8);
            if (cumulant8 < 0.0)
               SetIfFinite(*v28, bin, std::pow(-cumulant8 / 33.0, 1.0 / 8.0));
         }
      }

      AddOutput(outputs, std::move(corr2));
      AddOutput(outputs, std::move(corr4));
      AddOutput(outputs, std::move(corr6));
      AddOutput(outputs, std::move(corr8));
      AddOutput(outputs, std::move(c2));
      AddOutput(outputs, std::move(c4));
      AddOutput(outputs, std::move(c6));
      AddOutput(outputs, std::move(c8));
      AddOutput(outputs, std::move(v22));
      AddOutput(outputs, std::move(v24));
      AddOutput(outputs, std::move(v26));
      AddOutput(outputs, std::move(v28));
   }

   std::vector<std::unique_ptr<TH1D>> BuildSummaries(const HistMap &input)
   {
      std::vector<std::unique_ptr<TH1D>> outputs;
      for (const AxisSummary &axis : Axes)
      {
         BuildAxisSummary(input, axis, outputs);
         BuildTwoSubeventAxisSummary(input, axis, outputs);
      }
      return outputs;
   }

   TH1D *FindOutput(std::vector<std::unique_ptr<TH1D>> &outputs, const std::string &name)
   {
      for (auto &hist : outputs)
      {
         if (hist->GetName() == name)
            return hist.get();
      }
      return nullptr;
   }

   void ApplyJackknifeErrors(std::vector<std::unique_ptr<TH1D>> &central,
      const std::vector<HistMap> &inputBlocks)
   {
      const int blockCount = static_cast<int>(inputBlocks.size());
      if (blockCount < 2)
         return;

      std::vector<std::vector<std::unique_ptr<TH1D>>> leaveOneSummaries;
      leaveOneSummaries.reserve(blockCount);
      for (int skip = 0; skip < blockCount; ++skip)
      {
         HistMap leaveOne = SumHistograms(inputBlocks, skip);
         leaveOneSummaries.push_back(BuildSummaries(leaveOne));
      }

      for (auto &centralHist : central)
      {
         const std::string centralName = centralHist->GetName();
         const bool requiresValidRoot = centralName.rfind("hV2_", 0) == 0 ||
            centralName.rfind("hV2TwoSub_", 0) == 0;

         for (int bin = 1; bin <= centralHist->GetNbinsX(); ++bin)
         {
            if (requiresValidRoot && centralHist->GetBinContent(bin) == 0.0)
               continue;

            double mean = 0.0;
            int valid = 0;
            for (auto &sample : leaveOneSummaries)
            {
               TH1D *sampleHist = FindOutput(sample, centralHist->GetName());
               if (sampleHist == nullptr)
                  continue;
               const double value = sampleHist->GetBinContent(bin);
               if (!std::isfinite(value))
                  continue;
               if (requiresValidRoot && value == 0.0)
                  continue;
               mean += value;
               valid += 1;
            }

            if (requiresValidRoot && valid != blockCount)
            {
               centralHist->SetBinContent(bin, 0.0);
               centralHist->SetBinError(bin, 0.0);
               continue;
            }
            if (valid < 2)
               continue;
            mean /= static_cast<double>(valid);

            double varianceSum = 0.0;
            for (auto &sample : leaveOneSummaries)
            {
               TH1D *sampleHist = FindOutput(sample, centralHist->GetName());
               if (sampleHist == nullptr)
                  continue;
               const double value = sampleHist->GetBinContent(bin);
               if (!std::isfinite(value))
                  continue;
               if (requiresValidRoot && value == 0.0)
                  continue;
               const double delta = value - mean;
               varianceSum += delta * delta;
            }

            const double error = std::sqrt((static_cast<double>(valid) - 1.0) /
               static_cast<double>(valid) * varianceSum);
            if (std::isfinite(error))
               centralHist->SetBinError(bin, error);
         }
      }
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      std::vector<std::string> inputFileNames;
      if (cl.Has("Inputs"))
         inputFileNames = CleanInputList(cl.GetStringVector("Inputs", ""));
      else
         inputFileNames.push_back(cl.Get("Input", "output/aleph_charged_cumulants_merged.root"));

      if (inputFileNames.empty())
         throw std::runtime_error("No input files were provided");

      const std::string outputFileName = cl.Get("Output", "output/aleph_charged_cumulants_summary.root");

      std::vector<HistMap> inputBlocks;
      inputBlocks.reserve(inputFileNames.size());
      for (const std::string &fileName : inputFileNames)
         inputBlocks.push_back(ReadHistograms(fileName));

      HistMap total = SumHistograms(inputBlocks);
      std::vector<std::unique_ptr<TH1D>> outputs = BuildSummaries(total);
      ApplyJackknifeErrors(outputs, inputBlocks);

      TFile outputFile(outputFileName.c_str(), "RECREATE");
      if (outputFile.IsZombie())
      {
         std::cerr << "Failed to create output file: " << outputFileName << std::endl;
         return 1;
      }

      outputFile.cd();
      for (auto &hist : outputs)
         hist->Write();

      const std::string sourceSummary = Join(inputFileNames, ",");
      TNamed metadata("SummarySource", sourceSummary.c_str());
      metadata.Write();

      const std::string errorSummary = inputFileNames.size() >= 2
         ? "bin errors are delete-one-block jackknife errors from input chunk files"
         : "bin errors are not evaluated for a single merged input file; pass --Inputs chunk1.root,chunk2.root,...";
      TNamed errorMetadata("StatisticalErrorMethod", errorSummary.c_str());
      errorMetadata.Write();

      outputFile.Close();

      std::cout << "Wrote " << outputFileName << std::endl;
      if (inputFileNames.size() >= 2)
         std::cout << "Set jackknife errors from " << inputFileNames.size() << " input blocks" << std::endl;
      else
         std::cout << "No statistical errors set; provide chunk files through --Inputs for jackknife errors" << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}
