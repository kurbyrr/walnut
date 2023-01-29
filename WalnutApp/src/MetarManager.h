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
    void updateMetar(const std::string &airport);
    void updateMetars();
    void addAirports(const quicktype::Airac &airac);
    void removeAirport(const std::string &airport);
    void procFutures();

    std::map<std::string, std::future<std::string>> metarsFutures;
    std::unordered_map<std::string, std::pair<bool, std::string>> readyMetars;

  private:
    static std::string fetchMetar(const std::string &airport, int timeout = 0);
    static httplib::Client httpClient;
};
