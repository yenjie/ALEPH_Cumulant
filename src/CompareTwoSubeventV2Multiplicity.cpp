#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TLine.h"
#include "TPad.h"
#include "TStyle.h"
#include "TLatex.h"

#include "CommandLine.h"

namespace
{
   struct Quantity
   {
      std::string HistName;
      std::string Label;
   };

   struct RatioResult
   {
      double Value = 0.0;
      double Error = 0.0;
      bool Valid = false;
   };

   struct RatioRange
   {
      double Min = 0.8;
      double Max = 1.2;
      bool Found = false;
   };

   const std::vector<Quantity> Quantities = {
      {"hV2TwoSub_2", "v_{2}{2}"},
      {"hV2TwoSub_4", "v_{2}{4}"},
      {"hV2TwoSub_6", "v_{2}{6}"},
      {"hV2TwoSub_8", "v_{2}{8}"},
   };

   TH1D *GetHist(TFile &file, const std::string &name)
   {
      TH1D *hist = nullptr;
      file.GetObject(name.c_str(), hist);
      if (hist == nullptr)
         throw std::runtime_error("Missing histogram " + name);
      return hist;
   }

   bool IsValidPoint(const TH1D &hist, int bin)
   {
      return !(hist.GetBinContent(bin) == 0.0 && hist.GetBinError(bin) == 0.0);
   }

   RatioResult Divide(double numerator, double numeratorError,
      double denominator, double denominatorError, bool numeratorValid, bool denominatorValid)
   {
      RatioResult result;
      if (!numeratorValid || !denominatorValid || denominator == 0.0)
         return result;

      result.Value = numerator / denominator;
      result.Error = std::sqrt((numeratorError / denominator) * (numeratorError / denominator) +
         (numerator * denominatorError / (denominator * denominator)) *
         (numerator * denominatorError / (denominator * denominator)));
      result.Valid = std::isfinite(result.Value) && std::isfinite(result.Error);
      return result;
   }

   RatioResult Divide(const TH1D &numerator, const TH1D &denominator, int bin)
   {
      return Divide(numerator.GetBinContent(bin), numerator.GetBinError(bin),
         denominator.GetBinContent(bin), denominator.GetBinError(bin),
         IsValidPoint(numerator, bin), IsValidPoint(denominator, bin));
   }

   double MaxY(const TH1D &data, const TH1D &mc)
   {
      double result = 0.0;
      for (int bin = 1; bin <= data.GetNbinsX(); ++bin)
      {
         if (IsValidPoint(data, bin))
            result = std::max(result, data.GetBinContent(bin) + data.GetBinError(bin));
         if (IsValidPoint(mc, bin))
            result = std::max(result, mc.GetBinContent(bin) + mc.GetBinError(bin));
      }
      return result > 0.0 ? result : 1.0;
   }

   RatioRange GetRatioRange(const TH1D &numerator, const TH1D &denominator)
   {
      RatioRange range;
      for (int bin = 1; bin <= numerator.GetNbinsX(); ++bin)
      {
         const RatioResult ratio = Divide(numerator, denominator, bin);
         if (!ratio.Valid)
            continue;

         const double low = ratio.Value - ratio.Error;
         const double high = ratio.Value + ratio.Error;
         range.Min = range.Found ? std::min(range.Min, low) : low;
         range.Max = range.Found ? std::max(range.Max, high) : high;
         range.Found = true;
      }

      if (!range.Found)
         return range;

      range.Min = std::min(range.Min, 1.0);
      range.Max = std::max(range.Max, 1.0);
      double span = range.Max - range.Min;
      if (span < 0.08)
      {
         const double center = 0.5 * (range.Max + range.Min);
         range.Min = center - 0.04;
         range.Max = center + 0.04;
         span = range.Max - range.Min;
      }
      range.Min = std::max(0.0, range.Min - 0.18 * span);
      range.Max = range.Max + 0.18 * span;
      return range;
   }

   std::unique_ptr<TGraphErrors> MakeDataGraph(const TH1D &hist)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(kBlack);
      graph->SetMarkerColor(kBlack);
      graph->SetMarkerStyle(20);
      graph->SetMarkerSize(1.10);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
      {
         if (!IsValidPoint(hist, bin))
            continue;
         graph->SetPoint(point, hist.GetBinCenter(bin), hist.GetBinContent(bin));
         graph->SetPointError(point, 0.0, hist.GetBinError(bin));
         ++point;
      }
      return graph;
   }

   std::unique_ptr<TGraphErrors> MakeRatioGraph(const TH1D &numerator, const TH1D &denominator,
      int color, int marker, double xOffset)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(color);
      graph->SetMarkerColor(color);
      graph->SetMarkerStyle(marker);
      graph->SetMarkerSize(0.95);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= numerator.GetNbinsX(); ++bin)
      {
         const RatioResult ratio = Divide(numerator, denominator, bin);
         if (!ratio.Valid)
            continue;
         graph->SetPoint(point, numerator.GetBinCenter(bin) + xOffset, ratio.Value);
         graph->SetPointError(point, 0.0, ratio.Error);
         ++point;
      }
      return graph;
   }

   std::unique_ptr<TH1D> MakeMCHistogram(const TH1D &hist, const std::string &name)
   {
      std::unique_ptr<TH1D> result(static_cast<TH1D *>(hist.Clone(name.c_str())));
      result->SetDirectory(nullptr);
      result->SetLineColor(kAzure + 2);
      result->SetLineWidth(2);
      result->SetFillColorAlpha(kAzure - 9, 0.35);
      result->SetMarkerSize(0.0);
      for (int bin = 1; bin <= result->GetNbinsX(); ++bin)
      {
         if (!IsValidPoint(hist, bin))
         {
            result->SetBinContent(bin, 0.0);
            result->SetBinError(bin, 0.0);
         }
      }
      return result;
   }

   void CopyBinLabels(TH1D &frame, const TH1D &reference)
   {
      for (int bin = 1; bin <= reference.GetNbinsX(); ++bin)
         frame.GetXaxis()->SetBinLabel(bin, reference.GetXaxis()->GetBinLabel(bin));
      frame.GetXaxis()->LabelsOption("v");
      frame.GetXaxis()->SetLabelSize(0.067);
      frame.GetYaxis()->SetLabelSize(0.067);
      frame.GetXaxis()->SetTitleSize(0.074);
      frame.GetXaxis()->SetTitleOffset(2.35);
      frame.GetYaxis()->SetTitleSize(0.074);
      frame.GetYaxis()->SetTitleOffset(0.92);
   }

   void WriteComparisonTable(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputName)
   {
      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "axis,multiplicity_bin,quantity,data,data_err,mc,mc_err,data_over_mc,data_over_mc_err,"
          << "mc_over_data,mc_over_data_err\n";
      out << std::setprecision(10);
      for (const Quantity &quantity : Quantities)
      {
         TH1D *data = GetHist(dataFile, quantity.HistName + "_" + axis);
         TH1D *mc = GetHist(mcFile, quantity.HistName + "_" + axis);
         if (data->GetNbinsX() != mc->GetNbinsX())
            throw std::runtime_error("Mismatched binning for " + quantity.HistName + "_" + axis);

         for (int bin = 1; bin <= data->GetNbinsX(); ++bin)
         {
            const double d = data->GetBinContent(bin);
            const double ed = data->GetBinError(bin);
            const double m = mc->GetBinContent(bin);
            const double em = mc->GetBinError(bin);
            const RatioResult dataOverMC = Divide(d, ed, m, em, IsValidPoint(*data, bin), IsValidPoint(*mc, bin));
            const RatioResult mcOverData = Divide(m, em, d, ed, IsValidPoint(*mc, bin), IsValidPoint(*data, bin));
            out << axis << "," << data->GetXaxis()->GetBinLabel(bin) << "," << quantity.HistName
                << "," << d << "," << ed << "," << m << "," << em
                << "," << dataOverMC.Value << "," << dataOverMC.Error
                << "," << mcOverData.Value << "," << mcOverData.Error << "\n";
         }
      }
   }

   void PlotAxis(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &dataLabel, const std::string &mcLabel,
      bool ratioMCOverData, const std::string &ratioLabel)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      TCanvas canvas(("c_two_sub_compare_" + axis).c_str(),
         ("Two-subevent data vs MC " + axis).c_str(), 1300, 900);
      canvas.Divide(2, 2, 0.001, 0.001);
      std::vector<std::unique_ptr<TH1D>> ownedFrames;
      std::vector<std::unique_ptr<TH1D>> ownedMCHists;
      std::vector<std::unique_ptr<TGraphErrors>> ownedDataGraphs;
      std::vector<std::unique_ptr<TLegend>> ownedLegends;
      std::vector<std::unique_ptr<TPad>> ownedPads;
      std::vector<std::unique_ptr<TLine>> ownedLines;

      for (std::size_t i = 0; i < Quantities.size(); ++i)
      {
         const Quantity &quantity = Quantities[i];
         TH1D *data = GetHist(dataFile, quantity.HistName + "_" + axis);
         TH1D *mc = GetHist(mcFile, quantity.HistName + "_" + axis);
         if (data->GetNbinsX() != mc->GetNbinsX())
            throw std::runtime_error("Mismatched binning for " + quantity.HistName + "_" + axis);

         canvas.cd(i + 1);
         TPad *cell = static_cast<TPad *>(gPad);
         cell->SetMargin(0.0, 0.0, 0.0, 0.0);
         cell->cd();

         auto upperPad = std::make_unique<TPad>(
            ("upper_two_sub_" + axis + "_" + quantity.HistName).c_str(), "", 0.0, 0.34, 1.0, 1.0);
         auto lowerPad = std::make_unique<TPad>(
            ("ratio_two_sub_" + axis + "_" + quantity.HistName).c_str(), "", 0.0, 0.0, 1.0, 0.34);
         upperPad->SetLeftMargin(0.15);
         upperPad->SetRightMargin(0.04);
         upperPad->SetTopMargin(0.10);
         upperPad->SetBottomMargin(0.02);
         upperPad->SetGridy(true);
         lowerPad->SetLeftMargin(0.15);
         lowerPad->SetRightMargin(0.04);
         lowerPad->SetTopMargin(0.03);
         lowerPad->SetBottomMargin(0.44);
         lowerPad->SetGridy(true);
         upperPad->Draw();
         lowerPad->Draw();
         TPad *upper = upperPad.get();
         TPad *lower = lowerPad.get();
         ownedPads.push_back(std::move(upperPad));
         ownedPads.push_back(std::move(lowerPad));

         upper->cd();
         auto frame = std::make_unique<TH1D>(("frame_two_sub_" + axis + "_" + quantity.HistName).c_str(),
            (";;two-subevent " + quantity.Label).c_str(),
            data->GetNbinsX(), data->GetXaxis()->GetXmin(), data->GetXaxis()->GetXmax());
         CopyBinLabels(*frame, *data);
         frame->SetMinimum(0.0);
         frame->SetMaximum(MaxY(*data, *mc) * 1.32);
         frame->GetXaxis()->SetLabelSize(0.0);
         frame->GetXaxis()->SetTitleSize(0.0);
         frame->Draw("AXIS");
         ownedFrames.push_back(std::move(frame));

         auto mcHist = MakeMCHistogram(*mc, "mc_two_sub_" + axis + "_" + quantity.HistName);
         TH1D *mcHistPtr = mcHist.get();
         mcHist->Draw("E2 SAME");
         mcHist->Draw("HIST SAME");
         ownedMCHists.push_back(std::move(mcHist));

         auto dataGraph = MakeDataGraph(*data);
         TGraphErrors *dataGraphPtr = dataGraph.get();
         dataGraph->Draw("P SAME");
         ownedDataGraphs.push_back(std::move(dataGraph));

         TLatex label;
         label.SetNDC();
         label.SetTextSize(0.062);
         label.DrawLatex(0.16, 0.84, (axis + " axis, two-subevent " + quantity.Label).c_str());

         if (i == 0)
         {
            auto legend = std::make_unique<TLegend>(0.56, 0.70, 0.93, 0.88);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            legend->SetTextSize(0.062);
            legend->AddEntry(mcHistPtr, mcLabel.c_str(), "lf");
            legend->AddEntry(dataGraphPtr, dataLabel.c_str(), "pe");
            legend->Draw();
            ownedLegends.push_back(std::move(legend));
            upper->Update();
         }
         upper->Modified();

         lower->cd();
         const TH1D &ratioNumerator = ratioMCOverData ? *mc : *data;
         const TH1D &ratioDenominator = ratioMCOverData ? *data : *mc;
         const RatioRange ratioRange = GetRatioRange(ratioNumerator, ratioDenominator);
         auto ratioFrame = std::make_unique<TH1D>(("ratio_frame_two_sub_" + axis + "_" + quantity.HistName).c_str(),
            (";N_{trk}^{offline};" + ratioLabel).c_str(),
            data->GetNbinsX(), data->GetXaxis()->GetXmin(), data->GetXaxis()->GetXmax());
         CopyBinLabels(*ratioFrame, *data);
         ratioFrame->SetMinimum(ratioRange.Min);
         ratioFrame->SetMaximum(ratioRange.Max);
         ratioFrame->GetXaxis()->SetLabelSize(0.140);
         ratioFrame->GetXaxis()->SetTitleSize(0.150);
         ratioFrame->GetXaxis()->SetTitleOffset(2.00);
         ratioFrame->GetYaxis()->SetLabelSize(0.120);
         ratioFrame->GetYaxis()->SetTitleSize(0.125);
         ratioFrame->GetYaxis()->SetTitleOffset(0.48);
         ratioFrame->GetYaxis()->SetNdivisions(505);
         ratioFrame->Draw("AXIS");
         ownedFrames.push_back(std::move(ratioFrame));

         auto unity = std::make_unique<TLine>(data->GetXaxis()->GetXmin(), 1.0, data->GetXaxis()->GetXmax(), 1.0);
         unity->SetLineStyle(2);
         unity->SetLineColor(kGray + 2);
         unity->SetLineWidth(2);
         unity->Draw("SAME");
         ownedLines.push_back(std::move(unity));

         auto ratioGraph = MakeRatioGraph(ratioNumerator, ratioDenominator, kBlue + 2, 20, 0.0);
         ratioGraph->Draw("P SAME");
         ownedDataGraphs.push_back(std::move(ratioGraph));
         lower->Modified();
         cell->Modified();
      }

      canvas.SaveAs((outputPrefix + "_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_" + axis + ".pdf").c_str());
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      const std::string dataName = cl.Get("DataSummary", "output/lep1_1994_data_charged_pt04_twosub_summary.root");
      const std::string mcName = cl.Get("MCSummary", "output/lep1_1994_mc_charged_pt04_twosub_summary.root");
      const std::string outputPrefix = cl.Get("OutputPrefix", "output/lep1_1994_data_mc_charged_pt04_twosub_compare");
      const std::string dataLabel = cl.Get("DataLabel", "ALEPH 1994 data");
      const std::string mcLabel = cl.Get("MCLabel", "ALEPH MC reco");
      const bool ratioMCOverData = cl.GetBool("RatioMCOverData", false);
      const std::string ratioLabel = cl.Get("RatioLabel", ratioMCOverData ? "MC / data" : "data / MC");

      TFile dataFile(dataName.c_str(), "READ");
      if (dataFile.IsZombie())
         throw std::runtime_error("Failed to open data summary " + dataName);
      TFile mcFile(mcName.c_str(), "READ");
      if (mcFile.IsZombie())
         throw std::runtime_error("Failed to open MC summary " + mcName);

      for (const std::string &axis : {std::string("beam"), std::string("thrust")})
      {
         WriteComparisonTable(dataFile, mcFile, axis, outputPrefix + "_" + axis + ".csv");
         PlotAxis(dataFile, mcFile, axis, outputPrefix, dataLabel, mcLabel, ratioMCOverData, ratioLabel);
      }

      std::cout << "Wrote " << outputPrefix << "_[beam,thrust].[csv,png,pdf]" << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}
