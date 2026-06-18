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
#include "TStyle.h"

#include "CommandLine.h"

namespace
{
   struct Quantity
   {
      std::string HistName;
      std::string Label;
      int Color;
      int Marker;
   };

   std::vector<Quantity> MakeQuantities(const std::string &histPrefix)
   {
      return {
         {histPrefix + "2", "v_{2}{2}", kBlack, 20},
         {histPrefix + "4", "v_{2}{4}", kRed + 1, 21},
         {histPrefix + "6", "v_{2}{6}", kBlue + 1, 22},
         {histPrefix + "8", "v_{2}{8}", kGreen + 2, 23},
      };
   }

   TH1D *GetHist(TFile &file, const std::string &name)
   {
      TH1D *hist = nullptr;
      file.GetObject(name.c_str(), hist);
      if (hist == nullptr)
         throw std::runtime_error("Missing histogram " + name);
      return hist;
   }

   std::unique_ptr<TGraphErrors> MakeGraph(const TH1D &hist, const Quantity &quantity)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetName((std::string(hist.GetName()) + "Graph").c_str());
      graph->SetTitle(quantity.Label.c_str());
      graph->SetLineColor(quantity.Color);
      graph->SetMarkerColor(quantity.Color);
      graph->SetMarkerStyle(quantity.Marker);
      graph->SetMarkerSize(1.15);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
      {
         const double y = hist.GetBinContent(bin);
         const double ey = hist.GetBinError(bin);
         if (y == 0.0 && ey == 0.0)
            continue;
         graph->SetPoint(point, bin, y);
         graph->SetPointError(point, 0.0, ey);
         point += 1;
      }
      return graph;
   }

   void WriteTable(TFile &file, const std::string &axis, const std::string &outputName,
      const std::vector<Quantity> &quantities)
   {
      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      std::vector<TH1D *> hists;
      for (const Quantity &quantity : quantities)
         hists.push_back(GetHist(file, quantity.HistName + "_" + axis));

      out << "axis,multiplicity_bin";
      for (const Quantity &quantity : quantities)
         out << "," << quantity.HistName << "," << quantity.HistName << "_err";
      out << "\n";

      out << std::setprecision(10);
      for (int bin = 1; bin <= hists.front()->GetNbinsX(); ++bin)
      {
         out << axis << "," << hists.front()->GetXaxis()->GetBinLabel(bin);
         for (TH1D *hist : hists)
            out << "," << hist->GetBinContent(bin) << "," << hist->GetBinError(bin);
         out << "\n";
      }
   }

   void PlotAxis(TFile &file, const std::string &axis, const std::string &outputPrefix,
      const std::vector<Quantity> &quantities, const std::string &yTitle,
      const std::string &plotTitlePrefix)
   {
      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(4);

      TH1D *reference = GetHist(file, quantities.front().HistName + "_" + axis);
      std::vector<std::unique_ptr<TGraphErrors>> graphs;
      double yMax = 0.0;
      for (const Quantity &quantity : quantities)
      {
         TH1D *hist = GetHist(file, quantity.HistName + "_" + axis);
         for (int bin = 1; bin <= hist->GetNbinsX(); ++bin)
            yMax = std::max(yMax, hist->GetBinContent(bin) + hist->GetBinError(bin));
         graphs.push_back(MakeGraph(*hist, quantity));
      }
      if (yMax <= 0.0)
         yMax = 1.0;

      TCanvas canvas(("c_" + axis).c_str(), ("v2 vs multiplicity " + axis).c_str(), 1000, 720);
      canvas.SetLeftMargin(0.13);
      canvas.SetRightMargin(0.03);
      canvas.SetBottomMargin(0.32);

      TH1D frame("frame", (plotTitlePrefix + ", " + axis + " axis;N_{trk}^{offline};" + yTitle).c_str(),
         reference->GetNbinsX(), 0.5, reference->GetNbinsX() + 0.5);
      frame.SetMinimum(0.0);
      frame.SetMaximum(yMax * 1.25);
      for (int bin = 1; bin <= reference->GetNbinsX(); ++bin)
         frame.GetXaxis()->SetBinLabel(bin, reference->GetXaxis()->GetBinLabel(bin));
      frame.GetXaxis()->LabelsOption("v");
      frame.GetXaxis()->SetLabelSize(0.060);
      frame.GetYaxis()->SetLabelSize(0.060);
      frame.GetXaxis()->SetTitleSize(0.070);
      frame.GetXaxis()->SetTitleOffset(2.35);
      frame.GetYaxis()->SetTitleSize(0.070);
      frame.GetYaxis()->SetTitleOffset(0.88);
      frame.Draw("AXIS");

      TLegend legend(0.66, 0.68, 0.94, 0.90);
      legend.SetBorderSize(0);
      legend.SetFillStyle(0);
      legend.SetTextSize(0.055);
      for (std::size_t i = 0; i < graphs.size(); ++i)
      {
         graphs[i]->Draw("P SAME");
         legend.AddEntry(graphs[i].get(), quantities[i].Label.c_str(), "pe");
      }
      legend.Draw();

      canvas.SaveAs((outputPrefix + "_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_" + axis + ".pdf").c_str());
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      const std::string inputName = cl.Get("Input", "output/studymult_charged_pt04_summary.root");
      const std::string outputPrefix = cl.Get("OutputPrefix", "output/studymult_charged_pt04_v2");
      const std::string histPrefix = cl.Get("HistPrefix", "hV2_");
      const std::string yTitle = cl.Get("YTitle", "v_{2}{2k}");
      const std::string plotTitlePrefix = cl.Get("PlotTitlePrefix", "ALEPH charged particles");
      const std::vector<Quantity> quantities = MakeQuantities(histPrefix);

      TFile file(inputName.c_str(), "READ");
      if (file.IsZombie())
      {
         std::cerr << "Failed to open " << inputName << std::endl;
         return 1;
      }

      for (const std::string &axis : {std::string("beam"), std::string("thrust")})
      {
         WriteTable(file, axis, outputPrefix + "_" + axis + ".csv", quantities);
         PlotAxis(file, axis, outputPrefix, quantities, yTitle, plotTitlePrefix);
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
