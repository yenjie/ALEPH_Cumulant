#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "TClass.h"
#include "TFile.h"
#include "TH1.h"
#include "TKey.h"
#include "TObject.h"

int main(int argc, char *argv[])
{
   if (argc < 3)
   {
      std::cerr << "Usage: " << argv[0] << " <output.root> <input1.root> [input2.root ...]" << std::endl;
      return 1;
   }

   const std::string outputFileName = argv[1];
   std::map<std::string, std::unique_ptr<TObject>> mergedObjects;

   for (int iFile = 2; iFile < argc; ++iFile)
   {
      TFile inputFile(argv[iFile], "READ");
      if (inputFile.IsZombie())
      {
         std::cerr << "Failed to open input file: " << argv[iFile] << std::endl;
         return 1;
      }

      TIter nextKey(inputFile.GetListOfKeys());
      while (TKey *key = static_cast<TKey *>(nextKey()))
      {
         std::unique_ptr<TObject> object(key->ReadObj());
         if (!object)
            continue;

         const std::string name = object->GetName();
         if (mergedObjects.count(name) == 0)
         {
            TObject *clone = object->Clone();
            if (clone == nullptr)
               continue;
            if (clone->InheritsFrom(TH1::Class()))
               static_cast<TH1 *>(clone)->SetDirectory(nullptr);
            mergedObjects[name].reset(clone);
         }
         else if (object->InheritsFrom(TH1::Class()) &&
            mergedObjects[name]->InheritsFrom(TH1::Class()))
         {
            static_cast<TH1 *>(mergedObjects[name].get())->Add(static_cast<TH1 *>(object.get()));
         }
      }
   }

   TFile outputFile(outputFileName.c_str(), "RECREATE");
   if (outputFile.IsZombie())
   {
      std::cerr << "Failed to create output file: " << outputFileName << std::endl;
      return 1;
   }

   for (auto &item : mergedObjects)
      item.second->Write();

   outputFile.Close();
   return 0;
}

