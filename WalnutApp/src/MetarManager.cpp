#include "MetarManager.h"

httplib::Client MetarManager::httpClient(FBW_BASE_URL);

void MetarManager::addAirports(const quicktype::Airac &airac)
{
    for (int i = 0; i < airac.size(); i++)
    {
        metars[airac[i].icao] = std::async(std::launch::async, MetarManager::fetchMetar, airac[i].icao, i);
    }
}

void MetarManager::removeAirport(const std::string &airport)
{
    metars.erase(airport);
}

void MetarManager::updateMetar(const std::string &airport)
{
    metars[airport] = std::async(std::launch::async, MetarManager::fetchMetar, airport, 0);
}

void MetarManager::updateMetars()
{
    int i = 0;
    for (auto it = metars.begin(); it != metars.end(); it++, i++)
    {
        metars[it->first] = std::async(std::launch::async, MetarManager::fetchMetar, it->first, i);
    }
}

std::string MetarManager::fetchMetar(const std::string &airport, int timeout)
{
    std::this_thread::sleep_for(std::chrono::seconds(timeout));
    httplib::Headers headers = {{"accept", "application/json"}};
    httplib::Result res = httpClient.Get("/metar/" + airport, headers);
    if (res->status != 200)
        return {};

    quicktype::ApiRes json = nlohmann::json::parse(res->body);

    return json.metar;
}
