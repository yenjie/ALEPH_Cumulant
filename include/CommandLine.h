#ifndef ALEPH_CUMULANT_COMMANDLINE_H
#define ALEPH_CUMULANT_COMMANDLINE_H

#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class CommandLine
{
public:
   CommandLine(int argc, char *argv[])
   {
      for (int i = 1; i < argc; ++i)
      {
         std::string token(argv[i]);
         if (token.rfind("--", 0) != 0)
         {
            extra_.push_back(token);
            continue;
         }

         token.erase(0, 2);
         std::string value = "1";
         const std::size_t equal = token.find('=');
         if (equal != std::string::npos)
         {
            value = token.substr(equal + 1);
            token = token.substr(0, equal);
         }
         else if (i + 1 < argc && std::string(argv[i + 1]).rfind("-", 0) != 0)
         {
            value = argv[++i];
         }
         args_[token] = value;
      }
   }

   bool Has(const std::string &key) const
   {
      return args_.find(key) != args_.end();
   }

   std::string Get(const std::string &key, const std::string &defaultValue) const
   {
      const auto iter = args_.find(key);
      if (iter == args_.end())
         return defaultValue;
      return iter->second;
   }

   int GetInt(const std::string &key, int defaultValue) const
   {
      return std::atoi(Get(key, std::to_string(defaultValue)).c_str());
   }

   long long GetLongLong(const std::string &key, long long defaultValue) const
   {
      return std::atoll(Get(key, std::to_string(defaultValue)).c_str());
   }

   double GetDouble(const std::string &key, double defaultValue) const
   {
      return std::atof(Get(key, std::to_string(defaultValue)).c_str());
   }

   bool GetBool(const std::string &key, bool defaultValue) const
   {
      const std::string value = Get(key, defaultValue ? "1" : "0");
      if (value == "1" || value == "true" || value == "True" || value == "yes" || value == "on")
         return true;
      if (value == "0" || value == "false" || value == "False" || value == "no" || value == "off")
         return false;
      throw std::runtime_error("Cannot parse boolean argument --" + key + "=" + value);
   }

   std::vector<std::string> GetStringVector(const std::string &key,
      const std::string &defaultValue, char delimiter = ',') const
   {
      return Split(Get(key, defaultValue), delimiter);
   }

   std::vector<int> GetIntVector(const std::string &key,
      const std::string &defaultValue, char delimiter = ',') const
   {
      std::vector<int> result;
      for (const std::string &item : Split(Get(key, defaultValue), delimiter))
      {
         if (!item.empty())
            result.push_back(std::atoi(item.c_str()));
      }
      return result;
   }

private:
   static std::vector<std::string> Split(const std::string &input, char delimiter)
   {
      std::vector<std::string> result;
      std::stringstream stream(input);
      std::string item;
      while (std::getline(stream, item, delimiter))
         result.push_back(item);
      return result;
   }

   std::map<std::string, std::string> args_;
   std::vector<std::string> extra_;
};

#endif

