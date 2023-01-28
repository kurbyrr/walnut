#include "MetarManager.h"

httplib::Client MetarManager::httpClient(FBW_BASE_URL);

void MetarManager::removeAirport(const std::string &airport)
{
    metars.erase(airport);
}

void MetarManager::updateMetar(const std::string &airport)
{
    metars[airport] = std::async(std::launch::async, MetarManager::fetchMetar, airport);
}

void MetarManager::updateMetars()
{
    for (auto it = metars.begin(); it != metars.end(); it++)
    {
        updateMetar(it->first);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
}

std::string MetarManager::fetchMetar(const std::string &airport)
{
    httplib::Headers headers = {{"accept", "application/json"}};
    httplib::Result res = httpClient.Get("/metar/" + airport, headers);
    if (res->status != 200)
        return {};

    quicktype::ApiRes json = nlohmann::json::parse(res->body);

    return json.metar;
}
