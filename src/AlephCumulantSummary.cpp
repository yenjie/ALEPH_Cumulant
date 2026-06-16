#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "TFile.h"
#include "TH1D.h"
#include "TNamed.h"

#include "CommandLine.h"

namespace
{
   struct AxisSummary
   {
      std::string Name;
      std::string Label;
   };

   const AxisSummary Axes[2] = {
      {"beam", "beam axis"},
      {"thrust", "thrust axis"},
   };

   TH1D *GetHistogram(TFile &file, const std::string &name, bool required = true)
   {
      TH1D *hist = nullptr;
      file.GetObject(name.c_str(), hist);
      if (hist == nullptr && required)
         throw std::runtime_error("Missing histogram " + name);
      return hist;
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

   void WriteAxisSummary(TFile &inputFile, const AxisSummary &axis)
   {
      const std::string suffix = "_" + axis.Name;
      TH1D *sumNum2 = GetHistogram(inputFile, "hSumNum2" + suffix);
      TH1D *sumDen2 = GetHistogram(inputFile, "hSumDen2" + suffix);
      TH1D *sumNum4 = GetHistogram(inputFile, "hSumNum4" + suffix);
      TH1D *sumDen4 = GetHistogram(inputFile, "hSumDen4" + suffix);
      TH1D *sumNum6 = GetHistogram(inputFile, "hSumNum6" + suffix);
      TH1D *sumDen6 = GetHistogram(inputFile, "hSumDen6" + suffix);
      TH1D *sumNum8 = GetHistogram(inputFile, "hSumNum8" + suffix);
      TH1D *sumDen8 = GetHistogram(inputFile, "hSumDen8" + suffix);

      auto corr2 = CloneEmpty(*sumDen2, "hCorr2" + suffix,
         "<2> correlation, " + axis.Label + ";charged multiplicity bin;<2>");
      auto corr4 = CloneEmpty(*sumDen2, "hCorr4" + suffix,
         "<4> correlation, " + axis.Label + ";charged multiplicity bin;<4>");
      auto corr6 = CloneEmpty(*sumDen2, "hCorr6" + suffix,
         "<6> correlation, " + axis.Label + ";charged multiplicity bin;<6>");
      auto corr8 = CloneEmpty(*sumDen2, "hCorr8" + suffix,
         "<8> correlation, " + axis.Label + ";charged multiplicity bin;<8>");

      auto c2 = CloneEmpty(*sumDen2, "hC2_2" + suffix,
         "c_{2}{2}, " + axis.Label + ";charged multiplicity bin;c_{2}{2}");
      auto c4 = CloneEmpty(*sumDen2, "hC2_4" + suffix,
         "c_{2}{4}, " + axis.Label + ";charged multiplicity bin;c_{2}{4}");
      auto c6 = CloneEmpty(*sumDen2, "hC2_6" + suffix,
         "c_{2}{6}, " + axis.Label + ";charged multiplicity bin;c_{2}{6}");
      auto c8 = CloneEmpty(*sumDen2, "hC2_8" + suffix,
         "c_{2}{8}, " + axis.Label + ";charged multiplicity bin;c_{2}{8}");

      auto v22 = CloneEmpty(*sumDen2, "hV2_2" + suffix,
         "v_{2}{2}, " + axis.Label + ";charged multiplicity bin;v_{2}{2}");
      auto v24 = CloneEmpty(*sumDen2, "hV2_4" + suffix,
         "v_{2}{4}, " + axis.Label + ";charged multiplicity bin;v_{2}{4}");
      auto v26 = CloneEmpty(*sumDen2, "hV2_6" + suffix,
         "v_{2}{6}, " + axis.Label + ";charged multiplicity bin;v_{2}{6}");
      auto v28 = CloneEmpty(*sumDen2, "hV2_8" + suffix,
         "v_{2}{8}, " + axis.Label + ";charged multiplicity bin;v_{2}{8}");

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

      corr2->Write();
      corr4->Write();
      corr6->Write();
      corr8->Write();
      c2->Write();
      c4->Write();
      c6->Write();
      c8->Write();
      v22->Write();
      v24->Write();
      v26->Write();
      v28->Write();

      TH1D *sumNumV224 = GetHistogram(inputFile, "hSumNumV224" + suffix, false);
      TH1D *sumDenV224 = GetHistogram(inputFile, "hSumDenV224" + suffix, false);
      TH1D *sumNumV224ThreeSub = GetHistogram(inputFile, "hSumNumV224ThreeSub" + suffix, false);
      TH1D *sumDenV224ThreeSub = GetHistogram(inputFile, "hSumDenV224ThreeSub" + suffix, false);

      if (sumNumV224 != nullptr && sumDenV224 != nullptr)
      {
         auto v224 = CloneEmpty(*sumDenV224, "hV224" + suffix,
            "<e^{i(2#phi_{1}+2#phi_{2}-4#phi_{3})}>, " + axis.Label +
            ";charged multiplicity bin;v224");
         for (int bin = 1; bin <= sumDenV224->GetNbinsX(); ++bin)
         {
            if (HasRatio(sumNumV224, sumDenV224, bin))
               SetIfFinite(*v224, bin, Ratio(sumNumV224, sumDenV224, bin));
         }
         v224->Write();
      }

      if (sumNumV224ThreeSub != nullptr && sumDenV224ThreeSub != nullptr)
      {
         auto v224ThreeSub = CloneEmpty(*sumDenV224ThreeSub, "hV224ThreeSub" + suffix,
            "Three-subevent <e^{i(2#phi_{1}+2#phi_{2}-4#phi_{3})}>, " + axis.Label +
            ";charged multiplicity bin;v224");
         for (int bin = 1; bin <= sumDenV224ThreeSub->GetNbinsX(); ++bin)
         {
            if (HasRatio(sumNumV224ThreeSub, sumDenV224ThreeSub, bin))
               SetIfFinite(*v224ThreeSub, bin, Ratio(sumNumV224ThreeSub, sumDenV224ThreeSub, bin));
         }
         v224ThreeSub->Write();
      }
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      const std::string inputFileName = cl.Get("Input", "output/aleph_charged_cumulants_merged.root");
      const std::string outputFileName = cl.Get("Output", "output/aleph_charged_cumulants_summary.root");

      TFile inputFile(inputFileName.c_str(), "READ");
      if (inputFile.IsZombie())
      {
         std::cerr << "Failed to open input file: " << inputFileName << std::endl;
         return 1;
      }

      TFile outputFile(outputFileName.c_str(), "RECREATE");
      if (outputFile.IsZombie())
      {
         std::cerr << "Failed to create output file: " << outputFileName << std::endl;
         return 1;
      }

      outputFile.cd();
      for (const AxisSummary &axis : Axes)
         WriteAxisSummary(inputFile, axis);

      TNamed metadata("SummarySource", inputFileName.c_str());
      metadata.Write();
      outputFile.Close();

      std::cout << "Wrote " << outputFileName << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}

