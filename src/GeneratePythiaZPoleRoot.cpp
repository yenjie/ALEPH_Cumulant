#include "CommandLine.h"

#include "Pythia8/Pythia.h"

#include "TFile.h"
#include "TParameter.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
   constexpr double kPi = 3.14159265358979323846;

   bool IsNeutrino(int id)
   {
      const int absId = std::abs(id);
      return absId == 12 || absId == 14 || absId == 16;
   }

   short StudyMultPwflag(const Pythia8::Particle &particle)
   {
      const int absId = std::abs(particle.id());
      if (absId == 11)
         return 1;
      if (absId == 13)
         return 2;
      if (absId == 22)
         return 4;
      if (particle.charge() == 0)
         return 5;
      return 0;
   }

   bool PassAlephLikeAcceptance(const Pythia8::Particle &particle)
   {
      const double p = particle.pAbs();
      if (p <= 0)
         return false;

      const double cosTheta = particle.pz() / p;
      const double theta = std::acos(std::max(-1.0, std::min(1.0, cosTheta)));
      const bool isCharged = particle.charge() != 0;

      if (theta > kPi - 0.2 || theta < 0.2)
         return false;
      if (isCharged && theta > 160.0 * kPi / 180.0)
         return false;
      if (isCharged && theta < 20.0 * kPi / 180.0)
         return false;
      if (isCharged && particle.pT() < 0.2)
         return false;
      if (!isCharged && particle.e() < 0.8)
         return false;
      if (!isCharged && std::abs(cosTheta) > 0.98)
         return false;
      return true;
   }

   void Read(Pythia8::Pythia &pythia, const std::string &setting)
   {
      if (!pythia.readString(setting))
         throw std::runtime_error("Pythia rejected setting: " + setting);
   }

   void Usage()
   {
      std::cout
         << "Usage: bin/generate_pythia_zpole_root [options]\n"
         << "  --Output output/pythia_shoving_zpole_1M.root\n"
         << "  --Events 1000000\n"
         << "  --Seed 12345\n"
         << "  --PythiaData /path/to/share/Pythia8/xmldoc\n"
         << "  --EnableShoving 1\n"
         << "  --ShovingRepulsionFactor 0.25\n"
         << "  --ApplyAlephLikeAcceptance 0\n";
   }
}

int main(int argc, char *argv[])
{
   try
   {
      CommandLine cl(argc, argv);
      if (cl.GetBool("Help", false))
      {
         Usage();
         return 0;
      }

      const std::string outputPath = cl.Get("Output", "output/pythia_shoving_zpole_1M.root");
      const long long requestedEvents = cl.GetLongLong("Events", 1000000);
      const int seed = cl.GetInt("Seed", 12345);
      const double eCM = cl.GetDouble("ECM", 91.1876);
      const bool enableShoving = cl.GetBool("EnableShoving", true);
      const double shovingRepulsionFactor = cl.GetDouble("ShovingRepulsionFactor", 0.25);
      const bool applyAcceptance = cl.GetBool("ApplyAlephLikeAcceptance", false);
      const bool keepNeutrinos = cl.GetBool("KeepNeutrinos", false);
      const long long reportEvery = cl.GetLongLong("ReportEvery", 50000);
      const long long maxFailures = cl.GetLongLong("MaxFailures", 100000);
      const std::string pythiaData = cl.Get("PythiaData", "");

      if (requestedEvents <= 0)
         throw std::runtime_error("--Events must be positive");

      std::unique_ptr<Pythia8::Pythia> pythiaPtr;
      if (pythiaData.empty())
         pythiaPtr = std::make_unique<Pythia8::Pythia>();
      else
         pythiaPtr = std::make_unique<Pythia8::Pythia>(pythiaData);
      Pythia8::Pythia &pythia = *pythiaPtr;

      Read(pythia, "Beams:idA = 11");
      Read(pythia, "Beams:idB = -11");
      Read(pythia, "Beams:eCM = " + std::to_string(eCM));
      Read(pythia, "WeakSingleBoson:ffbar2gmZ = on");
      Read(pythia, "PDF:lepton = off");
      Read(pythia, "SpaceShower:QEDshowerByL = off");
      Read(pythia, "23:onMode = off");
      Read(pythia, "23:onIfAny = 1 2 3 4 5");
      Read(pythia, "Next:numberShowInfo = 1");
      Read(pythia, "Next:numberShowProcess = 0");
      Read(pythia, "Next:numberShowEvent = 0");
      if (reportEvery > 0)
         Read(pythia, "Next:numberCount = " + std::to_string(reportEvery));
      Read(pythia, "Random:setSeed = on");
      Read(pythia, "Random:seed = " + std::to_string(seed));

      if (enableShoving)
      {
         Read(pythia, "StringInteractions:model = 3");
         Read(pythia, "Gleipnir:shoving = on");
         Read(pythia, "Gleipnir:StringR = 1.0");
         Read(pythia, "Gleipnir:tauString = 1.0");
         Read(pythia, "Gleipnir:tauHad = 2.0");
         Read(pythia, "Gleipnir:nPushMaxCalc = 0");
         Read(pythia, "Gleipnir:pushPT = 0.02");
         Read(pythia, "Gleipnir:extendGluonRegions = on");
         Read(pythia, "Gleipnir:repulsionFactor = " + std::to_string(shovingRepulsionFactor));
         Read(pythia, "Fragmentation:setVertices = on");
         Read(pythia, "PartonVertex:setVertex = on");
         Read(pythia, "PartonVertex:modeRadiation = 2");
      }

      if (!pythia.init())
         throw std::runtime_error("Pythia initialization failed");

      TFile output(outputPath.c_str(), "RECREATE");
      if (output.IsZombie())
         throw std::runtime_error("Could not create output ROOT file: " + outputPath);

      TTree tree("t", "Pythia8 Z-pole visible final particles");
      int EventNo = 0;
      int source = 0;
      float Energy = 0;
      bool passEventSelection = true;
      std::vector<float> px;
      std::vector<float> py;
      std::vector<float> pz;
      std::vector<float> mass;
      std::vector<float> energy;
      std::vector<float> vx;
      std::vector<float> vy;
      std::vector<float> vz;
      std::vector<int> pid;
      std::vector<short> pwflag;
      std::vector<short> charge;

      tree.Branch("EventNo", &EventNo);
      tree.Branch("source", &source);
      tree.Branch("Energy", &Energy);
      tree.Branch("passEventSelection", &passEventSelection);
      tree.Branch("px", &px);
      tree.Branch("py", &py);
      tree.Branch("pz", &pz);
      tree.Branch("mass", &mass);
      tree.Branch("energy", &energy);
      tree.Branch("vx", &vx);
      tree.Branch("vy", &vy);
      tree.Branch("vz", &vz);
      tree.Branch("pid", &pid);
      tree.Branch("pwflag", &pwflag);
      tree.Branch("charge", &charge);

      TTree config("pythia_config", "Pythia generation configuration");
      char key[256] = {0};
      char value[2048] = {0};
      config.Branch("key", key, "key/C");
      config.Branch("value", value, "value/C");
      auto fillConfig = [&](const char *configKey, const std::string &configValue) {
         std::snprintf(key, sizeof(key), "%s", configKey);
         std::snprintf(value, sizeof(value), "%s", configValue.c_str());
         config.Fill();
      };
      fillConfig("EventsRequested", std::to_string(requestedEvents));
      fillConfig("MaxFailures", std::to_string(maxFailures));
      fillConfig("Seed", std::to_string(seed));
      fillConfig("ECM", std::to_string(eCM));
      fillConfig("EnableShoving", enableShoving ? "1" : "0");
      fillConfig("ShovingRepulsionFactor", std::to_string(shovingRepulsionFactor));
      fillConfig("ApplyAlephLikeAcceptance", applyAcceptance ? "1" : "0");
      fillConfig("KeepNeutrinos", keepNeutrinos ? "1" : "0");
      fillConfig("PythiaData", pythiaData);
      fillConfig("Process", "e+e- -> gamma*/Z -> q qbar, 23:onIfAny = 1 2 3 4 5");
      fillConfig("ShovingSettings", enableShoving
         ? "StringInteractions:model=3; Gleipnir:shoving=on; StringR=1.0; tauString=1.0; tauHad=2.0; pushPT=0.02; extendGluonRegions=on; repulsionFactor=" + std::to_string(shovingRepulsionFactor) + "; PartonVertex:setVertex=on; PartonVertex:modeRadiation=2"
         : "off");

      long long acceptedEvents = 0;
      long long attemptedEvents = 0;
      long long failedEvents = 0;
      long long visibleParticles = 0;
      long long chargedParticles = 0;

      while (acceptedEvents < requestedEvents)
      {
         ++attemptedEvents;
         if (!pythia.next())
         {
            ++failedEvents;
            if (failedEvents > maxFailures)
               throw std::runtime_error("Exceeded --MaxFailures while generating requested accepted events");
            continue;
         }

         px.clear();
         py.clear();
         pz.clear();
         mass.clear();
         energy.clear();
         vx.clear();
         vy.clear();
         vz.clear();
         pid.clear();
         pwflag.clear();
         charge.clear();

         for (int iPart = 0; iPart < pythia.event.size(); ++iPart)
         {
            const Pythia8::Particle &particle = pythia.event[iPart];
            if (!particle.isFinal())
               continue;
            if (!keepNeutrinos && IsNeutrino(particle.id()))
               continue;
            if (applyAcceptance && !PassAlephLikeAcceptance(particle))
               continue;

            px.push_back(static_cast<float>(particle.px()));
            py.push_back(static_cast<float>(particle.py()));
            pz.push_back(static_cast<float>(particle.pz()));
            mass.push_back(static_cast<float>(particle.m()));
            energy.push_back(static_cast<float>(particle.e()));
            vx.push_back(static_cast<float>(particle.xProd()));
            vy.push_back(static_cast<float>(particle.yProd()));
            vz.push_back(static_cast<float>(particle.zProd()));
            pid.push_back(particle.id());
            pwflag.push_back(StudyMultPwflag(particle));
            charge.push_back(static_cast<short>(std::lround(particle.charge())));
         }

         EventNo = static_cast<int>(acceptedEvents);
         source = pythia.info.code();
         Energy = static_cast<float>(pythia.info.eCM());
         passEventSelection = true;
         tree.Fill();

         ++acceptedEvents;
         visibleParticles += static_cast<long long>(px.size());
         for (const short flag : pwflag)
            if (flag >= 0 && flag <= 2)
               ++chargedParticles;

         if (reportEvery > 0 && acceptedEvents % reportEvery == 0)
            std::cout << "Generated " << acceptedEvents << " accepted events / "
                      << requestedEvents << " requested" << std::endl;
      }

      output.cd();
      tree.Write();
      config.Write();
      TParameter<long long>("EventsRequested", requestedEvents).Write();
      TParameter<long long>("EventsWritten", acceptedEvents).Write();
      TParameter<long long>("EventsAttempted", attemptedEvents).Write();
      TParameter<long long>("EventsFailed", failedEvents).Write();
      TParameter<long long>("VisibleParticles", visibleParticles).Write();
      TParameter<long long>("ChargedParticles", chargedParticles).Write();
      TParameter<int>("Seed", seed).Write();
      TParameter<double>("ECM", eCM).Write();
      output.Close();

      pythia.stat();
      std::cout << "Wrote " << acceptedEvents << " events to " << outputPath
                << " with " << visibleParticles << " visible particles and "
                << chargedParticles << " StudyMult-selected charged particles."
                << std::endl;
      std::cout << "Pythia attempts: " << attemptedEvents << std::endl;
      if (failedEvents > 0)
         std::cout << "Pythia failures skipped: " << failedEvents << std::endl;
   }
   catch (const std::exception &e)
   {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
