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
      int Marker;
   };

   const std::vector<Quantity> Quantities = {
      {"hV2_2", "v_{2}{2}", 20},
      {"hV2_4", "v_{2}{4}", 21},
      {"hV2_6", "v_{2}{6}", 22},
      {"hV2_8", "v_{2}{8}", 23},
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
      graph->SetMarkerSize(0.9);
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
      frame.GetXaxis()->SetLabelSize(0.055);
      frame.GetYaxis()->SetLabelSize(0.055);
      frame.GetXaxis()->SetTitleSize(0.060);
      frame.GetXaxis()->SetTitleOffset(2.25);
      frame.GetYaxis()->SetTitleSize(0.060);
      frame.GetYaxis()->SetTitleOffset(0.78);
   }

   void WriteComparisonTable(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputName)
   {
      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "axis,multiplicity_bin,quantity,data,data_err,mc,mc_err,data_over_mc,data_over_mc_err\n";
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
            double ratio = 0.0;
            double ratioErr = 0.0;
            if (m != 0.0 && IsValidPoint(*data, bin) && IsValidPoint(*mc, bin))
            {
               ratio = d / m;
               ratioErr = std::sqrt((ed / m) * (ed / m) + (d * em / (m * m)) * (d * em / (m * m)));
            }
            out << axis << "," << data->GetXaxis()->GetBinLabel(bin) << "," << quantity.HistName
                << "," << d << "," << ed << "," << m << "," << em << "," << ratio << "," << ratioErr << "\n";
         }
      }
   }

   void PlotAxis(TFile &dataFile, TFile &mcFile, const std::string &axis,
      const std::string &outputPrefix, const std::string &dataLabel, const std::string &mcLabel)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      TCanvas canvas(("c_compare_" + axis).c_str(), ("Data vs MC " + axis).c_str(), 1300, 900);
      canvas.Divide(2, 2, 0.001, 0.001);
      std::vector<std::unique_ptr<TH1D>> ownedFrames;
      std::vector<std::unique_ptr<TGraphErrors>> ownedGraphs;
      std::vector<std::unique_ptr<TLegend>> ownedLegends;

      for (std::size_t i = 0; i < Quantities.size(); ++i)
      {
         const Quantity &quantity = Quantities[i];
         TH1D *data = GetHist(dataFile, quantity.HistName + "_" + axis);
         TH1D *mc = GetHist(mcFile, quantity.HistName + "_" + axis);
         if (data->GetNbinsX() != mc->GetNbinsX())
            throw std::runtime_error("Mismatched binning for " + quantity.HistName + "_" + axis);

         canvas.cd(i + 1);
         TPad *pad = static_cast<TPad *>(gPad);
         pad->SetLeftMargin(0.12);
         pad->SetRightMargin(0.04);
         pad->SetTopMargin(0.10);
         pad->SetBottomMargin(0.30);
         pad->SetGridy(true);

         auto frame = std::make_unique<TH1D>(("frame_" + axis + "_" + quantity.HistName).c_str(),
            (";N_{trk}^{offline};" + quantity.Label).c_str(),
            data->GetNbinsX(), 0.5, data->GetNbinsX() + 0.5);
         CopyBinLabels(*frame, *data);
         frame->SetMinimum(0.0);
         frame->SetMaximum(MaxY(*data, *mc) * 1.28);
         frame->Draw("AXIS");
         ownedFrames.push_back(std::move(frame));

         auto dataGraph = MakeGraph(*data, kBlack, quantity.Marker, -0.10);
         auto mcGraph = MakeGraph(*mc, kRed + 1, quantity.Marker + 4, 0.10);
         dataGraph->Draw("P SAME");
         mcGraph->Draw("P SAME");
         TGraphErrors *dataGraphPtr = dataGraph.get();
         TGraphErrors *mcGraphPtr = mcGraph.get();
         ownedGraphs.push_back(std::move(dataGraph));
         ownedGraphs.push_back(std::move(mcGraph));

         TLatex label;
         label.SetNDC();
         label.SetTextSize(0.055);
         label.DrawLatex(0.16, 0.84, (axis + " axis, " + quantity.Label).c_str());

         if (i == 0)
         {
            auto legend = std::make_unique<TLegend>(0.58, 0.70, 0.93, 0.88);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            legend->AddEntry(dataGraphPtr, dataLabel.c_str(), "pe");
            legend->AddEntry(mcGraphPtr, mcLabel.c_str(), "pe");
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
      const std::string dataName = cl.Get("DataSummary", "output/lep1_1994_data_charged_pt04_summary.root");
      const std::string mcName = cl.Get("MCSummary", "output/lep1_1994_mc_charged_pt04_summary.root");
      const std::string outputPrefix = cl.Get("OutputPrefix", "output/lep1_1994_data_mc_charged_pt04_compare");
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
