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
#include "TLatex.h"
#include "TLegend.h"
#include "TPad.h"
#include "TStyle.h"

#include "CommandLine.h"

namespace
{
   struct Quantity
   {
      std::string HistName;
      std::string Label;
   };

   const std::vector<Quantity> Quantities = {
      {"hV2_2", "v_{2}{2}"},
      {"hV2_4", "v_{2}{4}"},
      {"hV2_6", "v_{2}{6}"},
      {"hV2_8", "v_{2}{8}"},
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

   std::unique_ptr<TGraphErrors> MakeGraph(const TH1D &hist, int color, int marker, double xOffset)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(color);
      graph->SetMarkerColor(color);
      graph->SetMarkerStyle(marker);
      graph->SetMarkerSize(1.30);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
      {
         if (!IsValidPoint(hist, bin))
            continue;
         graph->SetPoint(point, bin + xOffset, hist.GetBinContent(bin));
         graph->SetPointError(point, 0.0, hist.GetBinError(bin));
         ++point;
      }
      return graph;
   }

   double MaxY(const TH1D &a, const TH1D &b)
   {
      double result = 0.0;
      for (int bin = 1; bin <= a.GetNbinsX(); ++bin)
      {
         if (IsValidPoint(a, bin))
            result = std::max(result, a.GetBinContent(bin) + a.GetBinError(bin));
         if (IsValidPoint(b, bin))
            result = std::max(result, b.GetBinContent(bin) + b.GetBinError(bin));
      }
      return result > 0.0 ? result : 1.0;
   }

   void CopyBinLabels(TH1D &frame, const TH1D &reference)
   {
      for (int bin = 1; bin <= reference.GetNbinsX(); ++bin)
         frame.GetXaxis()->SetBinLabel(bin, reference.GetXaxis()->GetBinLabel(bin));
      frame.GetXaxis()->LabelsOption("v");
      frame.GetXaxis()->SetLabelSize(0.067);
      frame.GetYaxis()->SetLabelSize(0.067);
      frame.GetXaxis()->SetTitleSize(0.074);
      frame.GetXaxis()->SetTitleOffset(2.50);
      frame.GetYaxis()->SetTitleSize(0.074);
      frame.GetYaxis()->SetTitleOffset(0.92);
   }

   void WriteComparisonTable(TFile &referenceFile, TFile &comparisonFile, const std::string &axis,
      const std::string &outputName)
   {
      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "axis,multiplicity_bin,quantity,lep1,lep1_err,lep2,lep2_err,lep2_over_lep1,lep2_over_lep1_err\n";
      out << std::setprecision(10);
      for (const Quantity &quantity : Quantities)
      {
         TH1D *reference = GetHist(referenceFile, quantity.HistName + "_" + axis);
         TH1D *comparison = GetHist(comparisonFile, quantity.HistName + "_" + axis);
         if (reference->GetNbinsX() != comparison->GetNbinsX())
            throw std::runtime_error("Mismatched binning for " + quantity.HistName + "_" + axis);

         for (int bin = 1; bin <= reference->GetNbinsX(); ++bin)
         {
            const double r = reference->GetBinContent(bin);
            const double er = reference->GetBinError(bin);
            const double c = comparison->GetBinContent(bin);
            const double ec = comparison->GetBinError(bin);
            double ratio = 0.0;
            double ratioErr = 0.0;
            if (r != 0.0 && IsValidPoint(*reference, bin) && IsValidPoint(*comparison, bin))
            {
               ratio = c / r;
               ratioErr = std::sqrt((ec / r) * (ec / r) + (c * er / (r * r)) * (c * er / (r * r)));
            }

            out << axis << "," << reference->GetXaxis()->GetBinLabel(bin) << "," << quantity.HistName
                << "," << r << "," << er << "," << c << "," << ec << "," << ratio << "," << ratioErr << "\n";
         }
      }
   }

   void PlotAxis(TFile &referenceFile, TFile &comparisonFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &referenceLabel,
      const std::string &comparisonLabel, const std::string &selectionLabel)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      TCanvas canvas(("c_compare_data_" + axis).c_str(), ("LEP1 vs LEP2 " + axis).c_str(), 1300, 900);
      canvas.Divide(2, 2, 0.001, 0.001);
      std::vector<std::unique_ptr<TH1D>> ownedFrames;
      std::vector<std::unique_ptr<TGraphErrors>> ownedGraphs;
      std::vector<std::unique_ptr<TLegend>> ownedLegends;

      for (std::size_t i = 0; i < Quantities.size(); ++i)
      {
         const Quantity &quantity = Quantities[i];
         TH1D *reference = GetHist(referenceFile, quantity.HistName + "_" + axis);
         TH1D *comparison = GetHist(comparisonFile, quantity.HistName + "_" + axis);
         if (reference->GetNbinsX() != comparison->GetNbinsX())
            throw std::runtime_error("Mismatched binning for " + quantity.HistName + "_" + axis);

         canvas.cd(i + 1);
         TPad *pad = static_cast<TPad *>(gPad);
         pad->SetLeftMargin(0.15);
         pad->SetRightMargin(0.04);
         pad->SetTopMargin(0.10);
         pad->SetBottomMargin(0.35);
         pad->SetGridy(true);

         auto frame = std::make_unique<TH1D>(("frame_data_" + axis + "_" + quantity.HistName).c_str(),
            (";N_{trk}^{offline};" + quantity.Label).c_str(),
            reference->GetNbinsX(), 0.5, reference->GetNbinsX() + 0.5);
         CopyBinLabels(*frame, *reference);
         frame->SetMinimum(0.0);
         frame->SetMaximum(MaxY(*reference, *comparison) * 1.30);
         frame->Draw("AXIS");
         ownedFrames.push_back(std::move(frame));

         auto referenceGraph = MakeGraph(*reference, kBlack, 24, -0.08);
         auto comparisonGraph = MakeGraph(*comparison, kRed + 1, 20, 0.08);
         referenceGraph->Draw("P SAME");
         comparisonGraph->Draw("P SAME");
         TGraphErrors *referenceGraphPtr = referenceGraph.get();
         TGraphErrors *comparisonGraphPtr = comparisonGraph.get();
         ownedGraphs.push_back(std::move(referenceGraph));
         ownedGraphs.push_back(std::move(comparisonGraph));

         TLatex label;
         label.SetNDC();
         label.SetTextSize(0.064);
         label.DrawLatex(0.16, 0.84, (axis + " axis, " + quantity.Label).c_str());
         label.SetTextSize(0.052);
         label.DrawLatex(0.16, 0.76, selectionLabel.c_str());

         if (i == 0)
         {
            auto legend = std::make_unique<TLegend>(0.52, 0.66, 0.93, 0.88);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            legend->SetTextSize(0.058);
            legend->AddEntry(referenceGraphPtr, referenceLabel.c_str(), "pe");
            legend->AddEntry(comparisonGraphPtr, comparisonLabel.c_str(), "pe");
            legend->Draw();
            ownedLegends.push_back(std::move(legend));
            pad->Update();
         }
         pad->Modified();
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
      const std::string referenceName =
         cl.Get("ReferenceSummary", "output/lep1_1992_1995_data_charged_pt04_summary.root");
      const std::string comparisonName =
         cl.Get("ComparisonSummary", "output/lep2_1995_2000_data_charged_pt04_summary.root");
      const std::string outputPrefix =
         cl.Get("OutputPrefix", "output/lep1_1992_1995_data_vs_lep2_1995_2000_data_charged_pt04_compare");
      const std::string referenceLabel = cl.Get("ReferenceLabel", "LEP1 1992--1995 data");
      const std::string comparisonLabel = cl.Get("ComparisonLabel", "LEP2 merged data");
      const std::string selectionLabel = cl.Get("SelectionLabel", "charged particles, p_{T} > 0.4 GeV");

      TFile referenceFile(referenceName.c_str(), "READ");
      if (referenceFile.IsZombie())
         throw std::runtime_error("Failed to open reference summary " + referenceName);
      TFile comparisonFile(comparisonName.c_str(), "READ");
      if (comparisonFile.IsZombie())
         throw std::runtime_error("Failed to open comparison summary " + comparisonName);

      for (const std::string &axis : {std::string("beam"), std::string("thrust")})
      {
         WriteComparisonTable(referenceFile, comparisonFile, axis, outputPrefix + "_" + axis + ".csv");
         PlotAxis(referenceFile, comparisonFile, axis, outputPrefix, referenceLabel, comparisonLabel, selectionLabel);
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
