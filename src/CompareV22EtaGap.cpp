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
#include "TColor.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TPad.h"
#include "TStyle.h"
#include "TLatex.h"

#include "CommandLine.h"

namespace
{
   struct RatioResult
   {
      double Value = 0.0;
      double Error = 0.0;
      bool Valid = false;
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

   std::unique_ptr<TGraphErrors> MakeGraph(const TH1D &hist, int color, int marker, double xOffset)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(color);
      graph->SetMarkerColor(color);
      graph->SetMarkerStyle(marker);
      graph->SetMarkerSize(1.0);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
      {
         if (!IsValidPoint(hist, bin))
            continue;
         graph->SetPoint(point, hist.GetBinCenter(bin) + xOffset, hist.GetBinContent(bin));
         graph->SetPointError(point, 0.0, hist.GetBinError(bin));
         ++point;
      }
      return graph;
   }

   std::unique_ptr<TGraphErrors> MakeRatioGraph(const TH1D &gap, const TH1D &inclusive,
      int color, int marker)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(color);
      graph->SetMarkerColor(color);
      graph->SetMarkerStyle(marker);
      graph->SetMarkerSize(1.0);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= gap.GetNbinsX(); ++bin)
      {
         const RatioResult ratio = Divide(gap, inclusive, bin);
         if (!ratio.Valid)
            continue;
         graph->SetPoint(point, gap.GetBinCenter(bin), ratio.Value);
         graph->SetPointError(point, 0.0, ratio.Error);
         ++point;
      }
      return graph;
   }

   std::unique_ptr<TH1D> MakeBand(const TH1D &source, const std::string &name, int color)
   {
      auto hist = std::unique_ptr<TH1D>(static_cast<TH1D *>(source.Clone(name.c_str())));
      hist->SetDirectory(nullptr);
      hist->SetLineColor(color);
      hist->SetLineWidth(2);
      hist->SetFillColorAlpha(color, 0.25);
      hist->SetMarkerSize(0.0);
      return hist;
   }

   std::unique_ptr<TH1D> MakeRatioBand(const TH1D &gap, const TH1D &inclusive,
      const std::string &name, int color)
   {
      auto hist = std::unique_ptr<TH1D>(static_cast<TH1D *>(gap.Clone(name.c_str())));
      hist->SetDirectory(nullptr);
      hist->Reset("ICES");
      hist->SetLineColor(color);
      hist->SetLineWidth(2);
      hist->SetFillColorAlpha(color, 0.25);
      hist->SetMarkerSize(0.0);
      for (int bin = 1; bin <= gap.GetNbinsX(); ++bin)
      {
         const RatioResult ratio = Divide(gap, inclusive, bin);
         if (!ratio.Valid)
            continue;
         hist->SetBinContent(bin, ratio.Value);
         hist->SetBinError(bin, ratio.Error);
      }
      return hist;
   }

   void CopyBinLabels(TH1D &frame, const TH1D &reference)
   {
      for (int bin = 1; bin <= reference.GetNbinsX(); ++bin)
         frame.GetXaxis()->SetBinLabel(bin, reference.GetXaxis()->GetBinLabel(bin));
      frame.GetXaxis()->LabelsOption("v");
   }

   double MaxY(const std::vector<const TH1D *> &hists)
   {
      double result = 0.0;
      for (const TH1D *hist : hists)
      {
         for (int bin = 1; bin <= hist->GetNbinsX(); ++bin)
         {
            if (IsValidPoint(*hist, bin))
               result = std::max(result, hist->GetBinContent(bin) + hist->GetBinError(bin));
         }
      }
      return result > 0.0 ? result : 1.0;
   }

   double MaxRatio(const TH1D &dataGap, const TH1D &dataInclusive,
      const TH1D &mcGap, const TH1D &mcInclusive)
   {
      double result = 0.0;
      for (int bin = 1; bin <= dataGap.GetNbinsX(); ++bin)
      {
         const RatioResult dataRatio = Divide(dataGap, dataInclusive, bin);
         if (dataRatio.Valid)
            result = std::max(result, dataRatio.Value + dataRatio.Error);
         const RatioResult mcRatio = Divide(mcGap, mcInclusive, bin);
         if (mcRatio.Valid)
            result = std::max(result, mcRatio.Value + mcRatio.Error);
      }
      return result > 0.0 ? result : 1.0;
   }

   void WriteComparisonTable(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputName)
   {
      TH1D *dataInclusive = GetHist(dataFile, "hV2_2_" + axis);
      TH1D *dataGap = GetHist(dataFile, "hV2_2EtaGap_" + axis);
      TH1D *mcInclusive = GetHist(mcFile, "hV2_2_" + axis);
      TH1D *mcGap = GetHist(mcFile, "hV2_2EtaGap_" + axis);

      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "axis,multiplicity_bin,data_v22,data_v22_err,data_v22_gap,data_v22_gap_err,"
          << "data_gap_over_inclusive,data_gap_over_inclusive_err,"
          << "mc_v22,mc_v22_err,mc_v22_gap,mc_v22_gap_err,"
          << "mc_gap_over_inclusive,mc_gap_over_inclusive_err,"
          << "data_over_mc_v22,data_over_mc_v22_gap\n";
      out << std::setprecision(10);

      for (int bin = 1; bin <= dataInclusive->GetNbinsX(); ++bin)
      {
         const RatioResult dataGapRatio = Divide(*dataGap, *dataInclusive, bin);
         const RatioResult mcGapRatio = Divide(*mcGap, *mcInclusive, bin);
         const RatioResult dataOverMcInclusive = Divide(*dataInclusive, *mcInclusive, bin);
         const RatioResult dataOverMcGap = Divide(*dataGap, *mcGap, bin);

         out << axis << "," << dataInclusive->GetXaxis()->GetBinLabel(bin)
             << "," << dataInclusive->GetBinContent(bin) << "," << dataInclusive->GetBinError(bin)
             << "," << dataGap->GetBinContent(bin) << "," << dataGap->GetBinError(bin)
             << "," << dataGapRatio.Value << "," << dataGapRatio.Error
             << "," << mcInclusive->GetBinContent(bin) << "," << mcInclusive->GetBinError(bin)
             << "," << mcGap->GetBinContent(bin) << "," << mcGap->GetBinError(bin)
             << "," << mcGapRatio.Value << "," << mcGapRatio.Error
             << "," << dataOverMcInclusive.Value << "," << dataOverMcGap.Value << "\n";
      }
   }

   void PlotAxis(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &dataLabel, const std::string &mcLabel)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      TH1D *dataInclusive = GetHist(dataFile, "hV2_2_" + axis);
      TH1D *dataGap = GetHist(dataFile, "hV2_2EtaGap_" + axis);
      TH1D *mcInclusive = GetHist(mcFile, "hV2_2_" + axis);
      TH1D *mcGap = GetHist(mcFile, "hV2_2EtaGap_" + axis);
      if (dataInclusive->GetNbinsX() != dataGap->GetNbinsX() ||
         dataInclusive->GetNbinsX() != mcInclusive->GetNbinsX() ||
         dataInclusive->GetNbinsX() != mcGap->GetNbinsX())
      {
         throw std::runtime_error("Mismatched binning for eta-gap comparison, " + axis);
      }

      TCanvas canvas(("c_v22_etagap_" + axis).c_str(),
         ("v2{2} eta-gap comparison " + axis).c_str(), 1150, 900);
      TPad upper(("upper_" + axis).c_str(), "", 0.0, 0.32, 1.0, 1.0);
      TPad lower(("lower_" + axis).c_str(), "", 0.0, 0.0, 1.0, 0.32);
      upper.SetLeftMargin(0.10);
      upper.SetRightMargin(0.04);
      upper.SetTopMargin(0.08);
      upper.SetBottomMargin(0.02);
      upper.SetGridy(true);
      lower.SetLeftMargin(0.10);
      lower.SetRightMargin(0.04);
      lower.SetTopMargin(0.03);
      lower.SetBottomMargin(0.33);
      lower.SetGridy(true);
      upper.Draw();
      lower.Draw();

      const int nBins = dataInclusive->GetNbinsX();
      upper.cd();
      TH1D frame(("frame_" + axis).c_str(), ";;v_{2}{2}", nBins, 0.0, static_cast<double>(nBins));
      CopyBinLabels(frame, *dataInclusive);
      frame.SetMinimum(0.0);
      frame.SetMaximum(1.25 * MaxY({dataInclusive, dataGap, mcInclusive, mcGap}));
      frame.GetXaxis()->SetLabelSize(0.0);
      frame.GetYaxis()->SetLabelSize(0.045);
      frame.GetYaxis()->SetTitleSize(0.052);
      frame.GetYaxis()->SetTitleOffset(0.78);
      frame.Draw("AXIS");

      auto mcInclusiveBand = MakeBand(*mcInclusive, "mc_inclusive_" + axis, kRed + 1);
      auto mcGapBand = MakeBand(*mcGap, "mc_gap_" + axis, kBlue + 1);
      mcInclusiveBand->Draw("E2 SAME");
      mcInclusiveBand->Draw("HIST SAME");
      mcGapBand->Draw("E2 SAME");
      mcGapBand->Draw("HIST SAME");

      auto dataInclusiveGraph = MakeGraph(*dataInclusive, kRed + 1, 20, -0.08);
      auto dataGapGraph = MakeGraph(*dataGap, kBlue + 1, 21, 0.08);
      dataInclusiveGraph->Draw("P SAME");
      dataGapGraph->Draw("P SAME");

      TLatex label;
      label.SetNDC();
      label.SetTextSize(0.046);
      label.DrawLatex(0.13, 0.86, (axis + " axis").c_str());
      label.DrawLatex(0.13, 0.80, "charged particles, p_{T} > 0.4 GeV");

      TLegend legend(0.58, 0.62, 0.94, 0.90);
      legend.SetBorderSize(0);
      legend.SetFillStyle(0);
      legend.AddEntry(dataInclusiveGraph.get(), (dataLabel + " inclusive").c_str(), "pe");
      legend.AddEntry(dataGapGraph.get(), (dataLabel + " |#Delta#eta|>2").c_str(), "pe");
      legend.AddEntry(mcInclusiveBand.get(), (mcLabel + " inclusive").c_str(), "lf");
      legend.AddEntry(mcGapBand.get(), (mcLabel + " |#Delta#eta|>2").c_str(), "lf");
      legend.Draw();
      upper.Modified();

      lower.cd();
      TH1D ratioFrame(("ratio_frame_" + axis).c_str(),
         ";charged multiplicity bin;gap / incl.", nBins, 0.0, static_cast<double>(nBins));
      CopyBinLabels(ratioFrame, *dataInclusive);
      ratioFrame.SetMinimum(0.0);
      ratioFrame.SetMaximum(1.25 * MaxRatio(*dataGap, *dataInclusive, *mcGap, *mcInclusive));
      ratioFrame.GetXaxis()->SetLabelSize(0.095);
      ratioFrame.GetXaxis()->SetTitleSize(0.110);
      ratioFrame.GetYaxis()->SetLabelSize(0.090);
      ratioFrame.GetYaxis()->SetTitleSize(0.095);
      ratioFrame.GetYaxis()->SetTitleOffset(0.40);
      ratioFrame.GetYaxis()->SetNdivisions(505);
      ratioFrame.Draw("AXIS");

      auto mcRatioBand = MakeRatioBand(*mcGap, *mcInclusive, "mc_gap_ratio_" + axis, kBlue + 1);
      auto dataRatioGraph = MakeRatioGraph(*dataGap, *dataInclusive, kBlack, 20);
      mcRatioBand->Draw("E2 SAME");
      mcRatioBand->Draw("HIST SAME");
      dataRatioGraph->Draw("P SAME");
      lower.Modified();

      canvas.SaveAs((outputPrefix + "_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_" + axis + ".pdf").c_str());
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      const std::string dataName = cl.Get("DataSummary", "output/lep1_1994_data_charged_pt04_etagap_summary.root");
      const std::string mcName = cl.Get("MCSummary", "output/lep1_1994_mc_charged_pt04_etagap_summary.root");
      const std::string outputPrefix = cl.Get("OutputPrefix", "output/lep1_1994_data_mc_charged_pt04_etagap_compare");
      const std::string dataLabel = cl.Get("DataLabel", "ALEPH 1994 data");
      const std::string mcLabel = cl.Get("MCLabel", "ALEPH 1994 MC");

      TFile dataFile(dataName.c_str(), "READ");
      if (dataFile.IsZombie())
         throw std::runtime_error("Failed to open data summary " + dataName);
      TFile mcFile(mcName.c_str(), "READ");
      if (mcFile.IsZombie())
         throw std::runtime_error("Failed to open MC summary " + mcName);

      for (const std::string &axis : {std::string("beam"), std::string("thrust")})
      {
         WriteComparisonTable(dataFile, mcFile, axis, outputPrefix + "_" + axis + ".csv");
         PlotAxis(dataFile, mcFile, axis, outputPrefix, dataLabel, mcLabel);
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
