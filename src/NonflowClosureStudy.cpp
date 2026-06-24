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
#include "TLine.h"
#include "TPad.h"
#include "TStyle.h"

#include "CommandLine.h"

namespace
{
   struct Method
   {
      std::string Group;
      std::string Key;
      std::string Label;
      std::string FileName;
      std::string V2Base;
      std::string C2Base;
      int Color;
      int Marker;
   };

   const std::vector<Method> Methods = {
      {"gap", "inclusive", "inclusive", "output/lep1_1994_mc_merged_charged_pt04_summary.root", "hV2_2", "hC2_2", kBlack, 20},
      {"gap", "etagap1p0", "|#Delta#eta|>1.0", "output/nonflow_mc_pt04_etagap1p0_summary.root", "hV2_2EtaGap", "hCorr2EtaGap", kBlue + 1, 21},
      {"gap", "etagap1p6", "|#Delta#eta|>1.6", "output/lep1_1994_mc_merged_charged_pt04_etagap1p6_summary.root", "hV2_2EtaGap", "hCorr2EtaGap", kAzure + 2, 22},
      {"gap", "etagap2p0", "|#Delta#eta|>2.0", "output/lep1_1994_mc_merged_charged_pt04_etagap_summary.root", "hV2_2EtaGap", "hCorr2EtaGap", kCyan + 2, 23},
      {"gap", "etagap2p5", "|#Delta#eta|>2.5", "output/nonflow_mc_pt04_etagap2p5_summary.root", "hV2_2EtaGap", "hCorr2EtaGap", kTeal + 2, 29},
      {"gap", "etagap3p0", "|#Delta#eta|>3.0", "output/nonflow_mc_pt04_etagap3p0_summary.root", "hV2_2EtaGap", "hCorr2EtaGap", kGreen + 2, 33},
      {"gap", "twosub0", "two-sub gap 0", "output/lep1_1994_mc_merged_charged_pt04_twosub_summary.root", "hV2TwoSub_2", "hC2TwoSub_2", kRed + 1, 24},
      {"gap", "twosub1p0", "two-sub gap 1.0", "output/nonflow_mc_pt04_twosub_gap1p0_summary.root", "hV2TwoSub_2", "hC2TwoSub_2", kOrange + 7, 25},
      {"gap", "twosub1p6", "two-sub gap 1.6", "output/nonflow_mc_pt04_twosub_gap1p6_summary.root", "hV2TwoSub_2", "hC2TwoSub_2", kMagenta + 1, 26},
      {"gap", "twosub2p0", "two-sub gap 2.0", "output/nonflow_mc_pt04_twosub_gap2p0_summary.root", "hV2TwoSub_2", "hC2TwoSub_2", kViolet + 1, 32},

      {"selection", "inclusive", "inclusive", "output/lep1_1994_mc_merged_charged_pt04_summary.root", "hV2_2", "hC2_2", kBlack, 20},
      {"selection", "pt04to1", "0.4<p_{T}<1", "output/nonflow_mc_pt04to1_summary.root", "hV2_2", "hC2_2", kBlue + 1, 21},
      {"selection", "pt04to2", "0.4<p_{T}<2", "output/nonflow_mc_pt04to2_summary.root", "hV2_2", "hC2_2", kAzure + 2, 22},
      {"selection", "pt1to3", "1<p_{T}<3", "output/lep1_1994_mc_merged_charged_pt1to3_summary.root", "hV2_2", "hC2_2", kCyan + 2, 23},
      {"selection", "positive", "positive tracks", "output/nonflow_mc_positive_pt04_summary.root", "hV2_2", "hC2_2", kRed + 1, 24},
      {"selection", "negative", "negative tracks", "output/nonflow_mc_negative_pt04_summary.root", "hV2_2", "hC2_2", kOrange + 7, 25},
      {"selection", "thrustlt0p90", "Thrust<0.90", "output/nonflow_mc_thrustlt0p90_pt04_summary.root", "hV2_2", "hC2_2", kGreen + 2, 26},
      {"selection", "thrustlt0p85", "Thrust<0.85", "output/nonflow_mc_thrustlt0p85_pt04_summary.root", "hV2_2", "hC2_2", kTeal + 2, 32},
      {"selection", "sphgt0p12", "Sphericity>0.12", "output/nonflow_mc_sphericitygt0p12_pt04_summary.root", "hV2_2", "hC2_2", kMagenta + 1, 29},
      {"selection", "sphgt0p22", "Sphericity>0.22", "output/nonflow_mc_sphericitygt0p22_pt04_summary.root", "hV2_2", "hC2_2", kViolet + 1, 33},
   };

   struct LoadedMethod
   {
      const Method *Info = nullptr;
      std::unique_ptr<TFile> File;
      TH1D *V2 = nullptr;
      TH1D *C2 = nullptr;
   };

   bool IsValidPoint(const TH1D &hist, int bin)
   {
      return !(hist.GetBinContent(bin) == 0.0 && hist.GetBinError(bin) == 0.0);
   }

   TH1D *GetHist(TFile &file, const std::string &name)
   {
      TH1D *hist = nullptr;
      file.GetObject(name.c_str(), hist);
      if (hist == nullptr)
         throw std::runtime_error("Missing histogram " + name + " in " + file.GetName());
      return hist;
   }

   std::vector<LoadedMethod> LoadMethods(const std::string &group, const std::string &axis)
   {
      std::vector<LoadedMethod> result;
      for (const Method &method : Methods)
      {
         if (method.Group != group)
            continue;

         auto file = std::make_unique<TFile>(method.FileName.c_str(), "READ");
         if (file->IsZombie())
            throw std::runtime_error("Failed to open " + method.FileName);

         LoadedMethod loaded;
         loaded.Info = &method;
         loaded.V2 = GetHist(*file, method.V2Base + "_" + axis);
         loaded.C2 = GetHist(*file, method.C2Base + "_" + axis);
         loaded.File = std::move(file);
         result.push_back(std::move(loaded));
      }
      return result;
   }

   std::unique_ptr<TGraphErrors> MakeGraph(const TH1D &hist, int color, int marker)
   {
      auto graph = std::make_unique<TGraphErrors>();
      graph->SetLineColor(color);
      graph->SetMarkerColor(color);
      graph->SetMarkerStyle(marker);
      graph->SetMarkerSize(1.05);
      graph->SetLineWidth(2);

      int point = 0;
      for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
      {
         if (!IsValidPoint(hist, bin))
            continue;
         graph->SetPoint(point, bin, hist.GetBinContent(bin));
         graph->SetPointError(point, 0.0, hist.GetBinError(bin));
         ++point;
      }
      return graph;
   }

   std::pair<double, double> Range(const std::vector<LoadedMethod> &methods, bool useC2)
   {
      double minValue = useC2 ? 0.0 : 0.0;
      double maxValue = useC2 ? 0.0 : 1e-6;
      for (const LoadedMethod &method : methods)
      {
         const TH1D *hist = useC2 ? method.C2 : method.V2;
         for (int bin = 1; bin <= hist->GetNbinsX(); ++bin)
         {
            if (!IsValidPoint(*hist, bin))
               continue;
            const double value = hist->GetBinContent(bin);
            const double error = hist->GetBinError(bin);
            minValue = std::min(minValue, value - error);
            maxValue = std::max(maxValue, value + error);
         }
      }
      const double span = std::max(1e-6, maxValue - minValue);
      return {minValue - 0.15 * span, maxValue + 0.30 * span};
   }

   void CopyBinLabels(TH1D &frame, const TH1D &reference)
   {
      for (int bin = 1; bin <= reference.GetNbinsX(); ++bin)
         frame.GetXaxis()->SetBinLabel(bin, reference.GetXaxis()->GetBinLabel(bin));
      frame.GetXaxis()->LabelsOption("v");
      frame.GetXaxis()->SetLabelSize(0.050);
      frame.GetYaxis()->SetLabelSize(0.050);
      frame.GetXaxis()->SetTitleSize(0.055);
      frame.GetYaxis()->SetTitleSize(0.055);
      frame.GetXaxis()->SetTitleOffset(2.10);
      frame.GetYaxis()->SetTitleOffset(1.05);
   }

   void PlotGroup(const std::string &group, const std::string &axis, bool useC2,
      const std::string &outputPrefix)
   {
      auto methods = LoadMethods(group, axis);
      if (methods.empty())
         return;

      gStyle->SetOptStat(0);
      gStyle->SetEndErrorSize(3);

      const std::string observable = useC2 ? "c22" : "v22";
      const std::string yTitle = useC2 ? "c_{2}{2} or #LT2#GT_{gap}" : "v_{2}{2}";
      TCanvas canvas(("c_nonflow_" + group + "_" + observable + "_" + axis).c_str(),
         ("nonflow closure " + group + " " + observable + " " + axis).c_str(), 1200, 820);
      canvas.SetLeftMargin(0.11);
      canvas.SetRightMargin(0.04);
      canvas.SetTopMargin(0.08);
      canvas.SetBottomMargin(0.24);
      canvas.SetGridy(true);

      const TH1D *reference = useC2 ? methods.front().C2 : methods.front().V2;
      auto frame = std::make_unique<TH1D>(("frame_" + group + "_" + observable + "_" + axis).c_str(),
         (";N_{trk}^{offline};" + yTitle).c_str(), reference->GetNbinsX(), 0.5, reference->GetNbinsX() + 0.5);
      CopyBinLabels(*frame, *reference);
      const auto [minY, maxY] = Range(methods, useC2);
      frame->SetMinimum(minY);
      frame->SetMaximum(maxY);
      frame->Draw("AXIS");

      TLine zero(0.5, 0.0, reference->GetNbinsX() + 0.5, 0.0);
      zero.SetLineStyle(2);
      zero.SetLineColor(kGray + 2);
      if (useC2)
         zero.Draw("SAME");

      std::vector<std::unique_ptr<TGraphErrors>> graphs;
      auto legend = std::make_unique<TLegend>(0.14, group == "gap" ? 0.55 : 0.53, 0.95, 0.90);
      legend->SetBorderSize(0);
      legend->SetFillStyle(0);
      legend->SetNColumns(2);
      legend->SetTextSize(0.036);

      for (const LoadedMethod &method : methods)
      {
         const TH1D *hist = useC2 ? method.C2 : method.V2;
         auto graph = MakeGraph(*hist, method.Info->Color, method.Info->Marker);
         graph->Draw("PL SAME");
         legend->AddEntry(graph.get(), method.Info->Label.c_str(), "pl");
         graphs.push_back(std::move(graph));
      }
      legend->Draw();

      TLatex label;
      label.SetNDC();
      label.SetTextSize(0.045);
      label.DrawLatex(0.12, 0.93, ("LEP1 reconstructed MC, " + axis + " axis").c_str());

      canvas.SaveAs((outputPrefix + "_" + group + "_" + observable + "_" + axis + ".png").c_str());
      canvas.SaveAs((outputPrefix + "_" + group + "_" + observable + "_" + axis + ".pdf").c_str());
   }

   void WriteCsv(const std::string &outputName)
   {
      std::ofstream out(outputName);
      if (!out.is_open())
         throw std::runtime_error("Failed to create " + outputName);

      out << "group,axis,method,bin,v22,v22_err,c22_or_corr,c22_or_corr_err,c22_significance\n";
      out << std::setprecision(10);
      for (const std::string &group : {std::string("gap"), std::string("selection")})
      {
         for (const std::string &axis : {std::string("beam"), std::string("thrust")})
         {
            auto methods = LoadMethods(group, axis);
            for (const LoadedMethod &method : methods)
            {
               for (int bin = 1; bin <= method.C2->GetNbinsX(); ++bin)
               {
                  const bool validV2 = IsValidPoint(*method.V2, bin);
                  const bool validC2 = IsValidPoint(*method.C2, bin);
                  const double c2 = validC2 ? method.C2->GetBinContent(bin) : 0.0;
                  const double ec2 = validC2 ? method.C2->GetBinError(bin) : 0.0;
                  const double significance = ec2 > 0.0 ? c2 / ec2 : 0.0;
                  out << group << "," << axis << "," << method.Info->Key << ","
                      << method.C2->GetXaxis()->GetBinLabel(bin) << ","
                      << (validV2 ? method.V2->GetBinContent(bin) : 0.0) << ","
                      << (validV2 ? method.V2->GetBinError(bin) : 0.0) << ","
                      << c2 << "," << ec2 << "," << significance << "\n";
               }
            }
         }
      }
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      const std::string outputPrefix = cl.Get("OutputPrefix", "output/nonflow_mc_closure");

      for (const std::string &group : {std::string("gap"), std::string("selection")})
      {
         for (const std::string &axis : {std::string("beam"), std::string("thrust")})
         {
            PlotGroup(group, axis, true, outputPrefix);
            PlotGroup(group, axis, false, outputPrefix);
         }
      }
      WriteCsv(outputPrefix + ".csv");
      std::cout << "Wrote " << outputPrefix << " nonflow closure plots and CSV" << std::endl;
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << "Error: " << error.what() << std::endl;
      return 1;
   }
}
