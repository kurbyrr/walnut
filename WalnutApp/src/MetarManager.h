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
    void addAirports(const std::vector<std::string> &airports);
    void removeAirport(const std::string &airport);
    std::map<std::string, std::shared_future<std::string>> metars;

  private:
    static std::string fetchMetar(const std::string &airport, int timeout = 0);
    static httplib::Client httpClient;
};
