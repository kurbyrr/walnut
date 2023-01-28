#pragma once

#include <future>
#include <string>
#include <vector>

#include "Quicktype.hpp"
#include "json.hpp"
#include <httplib.h>

#define FBW_BASE_URL "https://api.flybywiresim.com"

class MetarManager
{
  public:
    void updateMetars();
    void updateMetar(const std::string &airport);
    void removeAirport(const std::string &airport);
    std::map<std::string, std::future<std::string>> metars;

  private:
    static std::string fetchMetar(const std::string &airport);
    static httplib::Client httpClient;
};
