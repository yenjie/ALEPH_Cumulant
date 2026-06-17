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
#include "TLine.h"
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

   std::unique_ptr<TH1D> MakeRatioHist(const TH1D &numerator, const TH1D &denominator,
      const std::string &name)
   {
      auto hist = std::unique_ptr<TH1D>(static_cast<TH1D *>(numerator.Clone(name.c_str())));
      hist->SetDirectory(nullptr);
      hist->Reset("ICES");

      for (int bin = 1; bin <= numerator.GetNbinsX(); ++bin)
      {
         const RatioResult ratio = Divide(numerator, denominator, bin);
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

   double MinY(const std::vector<const TH1D *> &hists)
   {
      bool found = false;
      double result = 0.0;
      for (const TH1D *hist : hists)
      {
         for (int bin = 1; bin <= hist->GetNbinsX(); ++bin)
         {
            if (!IsValidPoint(*hist, bin))
               continue;

            const double value = hist->GetBinContent(bin) - hist->GetBinError(bin);
            if (!std::isfinite(value))
               continue;

            result = found ? std::min(result, value) : value;
            found = true;
         }
      }
      return found ? result : 0.8;
   }

   void WriteComparisonTable(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputName)
   {
      TH1D *dataInclusive = GetHist(dataFile, "hV2_2_" + axis);
      TH1D *dataGap = GetHist(dataFile, "hV2_2EtaGap_" + axis);
      TH1D *mcInclusive = GetHist(mcFile, "hV2_2_" + axis);
      TH1D *mcGap = GetHist(mcFile, "hV2_2EtaGap_" + axis);
      TH1D *dataGapRatio = GetHist(dataFile, "hV2_2EtaGapOverInclusive_" + axis);
      TH1D *mcGapRatio = GetHist(mcFile, "hV2_2EtaGapOverInclusive_" + axis);

      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "axis,multiplicity_bin,data_v22,data_v22_err,data_v22_gap,data_v22_gap_err,"
          << "data_gap_over_inclusive,data_gap_over_inclusive_err,"
          << "mc_v22,mc_v22_err,mc_v22_gap,mc_v22_gap_err,"
          << "mc_gap_over_inclusive,mc_gap_over_inclusive_err,"
          << "data_over_mc_v22,data_over_mc_v22_err,"
          << "data_over_mc_v22_gap,data_over_mc_v22_gap_err\n";
      out << std::setprecision(10);

      for (int bin = 1; bin <= dataInclusive->GetNbinsX(); ++bin)
      {
         const bool dataGapRatioValid = IsValidPoint(*dataGapRatio, bin);
         const bool mcGapRatioValid = IsValidPoint(*mcGapRatio, bin);
         const RatioResult dataOverMcInclusive = Divide(*dataInclusive, *mcInclusive, bin);
         const RatioResult dataOverMcGap = Divide(*dataGap, *mcGap, bin);

         out << axis << "," << dataInclusive->GetXaxis()->GetBinLabel(bin)
             << "," << dataInclusive->GetBinContent(bin) << "," << dataInclusive->GetBinError(bin)
             << "," << dataGap->GetBinContent(bin) << "," << dataGap->GetBinError(bin)
             << "," << (dataGapRatioValid ? dataGapRatio->GetBinContent(bin) : 0.0)
             << "," << (dataGapRatioValid ? dataGapRatio->GetBinError(bin) : 0.0)
             << "," << mcInclusive->GetBinContent(bin) << "," << mcInclusive->GetBinError(bin)
             << "," << mcGap->GetBinContent(bin) << "," << mcGap->GetBinError(bin)
             << "," << (mcGapRatioValid ? mcGapRatio->GetBinContent(bin) : 0.0)
             << "," << (mcGapRatioValid ? mcGapRatio->GetBinError(bin) : 0.0)
             << "," << dataOverMcInclusive.Value << "," << dataOverMcInclusive.Error
             << "," << dataOverMcGap.Value << "," << dataOverMcGap.Error << "\n";
      }
   }

   void PlotAxis(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &dataLabel,
      const std::string &mcLabel, const std::string &gapLabel)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      TH1D *dataInclusive = GetHist(dataFile, "hV2_2_" + axis);
      TH1D *dataGap = GetHist(dataFile, "hV2_2EtaGap_" + axis);
      TH1D *mcInclusive = GetHist(mcFile, "hV2_2_" + axis);
      TH1D *mcGap = GetHist(mcFile, "hV2_2EtaGap_" + axis);
      TH1D *dataGapRatio = GetHist(dataFile, "hV2_2EtaGapOverInclusive_" + axis);
      TH1D *mcGapRatio = GetHist(mcFile, "hV2_2EtaGapOverInclusive_" + axis);
      if (dataInclusive->GetNbinsX() != dataGap->GetNbinsX() ||
         dataInclusive->GetNbinsX() != mcInclusive->GetNbinsX() ||
         dataInclusive->GetNbinsX() != mcGap->GetNbinsX() ||
         dataInclusive->GetNbinsX() != dataGapRatio->GetNbinsX() ||
         dataInclusive->GetNbinsX() != mcGapRatio->GetNbinsX())
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
      legend.AddEntry(dataGapGraph.get(), (dataLabel + " " + gapLabel).c_str(), "pe");
      legend.AddEntry(mcInclusiveBand.get(), (mcLabel + " inclusive").c_str(), "lf");
      legend.AddEntry(mcGapBand.get(), (mcLabel + " " + gapLabel).c_str(), "lf");
      legend.Draw();
      upper.Modified();

      lower.cd();
      TH1D ratioFrame(("ratio_frame_" + axis).c_str(),
         ";lab selected charged multiplicity bin;gap / incl.", nBins, 0.0, static_cast<double>(nBins));
      CopyBinLabels(ratioFrame, *dataInclusive);
      ratioFrame.SetMinimum(0.0);
      ratioFrame.SetMaximum(1.25 * MaxY({dataGapRatio, mcGapRatio}));
      ratioFrame.GetXaxis()->SetLabelSize(0.095);
      ratioFrame.GetXaxis()->SetTitleSize(0.110);
      ratioFrame.GetYaxis()->SetLabelSize(0.090);
      ratioFrame.GetYaxis()->SetTitleSize(0.095);
      ratioFrame.GetYaxis()->SetTitleOffset(0.40);
      ratioFrame.GetYaxis()->SetNdivisions(505);
      ratioFrame.Draw("AXIS");

      auto mcRatioBand = MakeBand(*mcGapRatio, "mc_gap_ratio_" + axis, kBlue + 1);
      auto dataRatioGraph = MakeGraph(*dataGapRatio, kBlack, 20, 0.0);
      mcRatioBand->Draw("E2 SAME");
      mcRatioBand->Draw("HIST SAME");
      dataRatioGraph->Draw("P SAME");
      lower.Modified();

      canvas.SaveAs((outputPrefix + "_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_" + axis + ".pdf").c_str());
   }

   void PlotDataMcRatioAxis(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &gapLabel)
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
         throw std::runtime_error("Mismatched binning for eta-gap data/MC ratio, " + axis);
      }

      auto inclusiveRatio = MakeRatioHist(*dataInclusive, *mcInclusive, "data_mc_inclusive_" + axis);
      auto gapRatio = MakeRatioHist(*dataGap, *mcGap, "data_mc_gap_" + axis);

      const int nBins = dataInclusive->GetNbinsX();
      TCanvas canvas(("c_v22_etagap_datamc_" + axis).c_str(),
         ("v2{2} eta-gap data/MC " + axis).c_str(), 1150, 520);
      canvas.SetLeftMargin(0.10);
      canvas.SetRightMargin(0.04);
      canvas.SetTopMargin(0.10);
      canvas.SetBottomMargin(0.25);
      canvas.SetGridy(true);

      TH1D frame(("datamc_frame_" + axis).c_str(),
         ";lab selected charged multiplicity bin;Data / MC", nBins, 0.0, static_cast<double>(nBins));
      CopyBinLabels(frame, *dataInclusive);
      const double yMin = std::max(0.0, std::min(0.85, MinY({inclusiveRatio.get(), gapRatio.get()}) * 0.94));
      const double yMax = std::max(1.15, MaxY({inclusiveRatio.get(), gapRatio.get()}) * 1.06);
      frame.SetMinimum(yMin);
      frame.SetMaximum(yMax);
      frame.GetXaxis()->SetLabelSize(0.070);
      frame.GetXaxis()->SetTitleSize(0.075);
      frame.GetYaxis()->SetLabelSize(0.070);
      frame.GetYaxis()->SetTitleSize(0.080);
      frame.GetYaxis()->SetTitleOffset(0.55);
      frame.GetYaxis()->SetNdivisions(505);
      frame.Draw("AXIS");

      TLine unity(0.0, 1.0, static_cast<double>(nBins), 1.0);
      unity.SetLineStyle(2);
      unity.SetLineColor(kGray + 2);
      unity.SetLineWidth(2);
      unity.Draw("SAME");

      auto inclusiveGraph = MakeGraph(*inclusiveRatio, kRed + 1, 20, -0.08);
      auto gapGraph = MakeGraph(*gapRatio, kBlue + 1, 21, 0.08);
      inclusiveGraph->Draw("P SAME");
      gapGraph->Draw("P SAME");

      TLatex label;
      label.SetNDC();
      label.SetTextSize(0.060);
      label.DrawLatex(0.13, 0.83, (axis + " axis").c_str());

      TLegend legend(0.60, 0.70, 0.94, 0.88);
      legend.SetBorderSize(0);
      legend.SetFillStyle(0);
      legend.AddEntry(inclusiveGraph.get(), "inclusive", "pe");
      legend.AddEntry(gapGraph.get(), gapLabel.c_str(), "pe");
      legend.Draw();

      canvas.SaveAs((outputPrefix + "_datamc_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_datamc_" + axis + ".pdf").c_str());
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
      const std::string gapLabel = cl.Get("GapLabel", "|#Delta#eta|>2");

      TFile dataFile(dataName.c_str(), "READ");
      if (dataFile.IsZombie())
         throw std::runtime_error("Failed to open data summary " + dataName);
      TFile mcFile(mcName.c_str(), "READ");
      if (mcFile.IsZombie())
         throw std::runtime_error("Failed to open MC summary " + mcName);

      for (const std::string &axis : {std::string("beam"), std::string("thrust")})
      {
         WriteComparisonTable(dataFile, mcFile, axis, outputPrefix + "_" + axis + ".csv");
         PlotAxis(dataFile, mcFile, axis, outputPrefix, dataLabel, mcLabel, gapLabel);
         PlotDataMcRatioAxis(dataFile, mcFile, axis, outputPrefix, gapLabel);
      }

      std::cout << "Wrote " << outputPrefix
                << "_[beam,thrust].[csv,png,pdf] and _datamc_[beam,thrust].[png,pdf]"
                << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}
