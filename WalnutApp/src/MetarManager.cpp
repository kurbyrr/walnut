#include "MetarManager.h"

httplib::Client MetarManager::httpClient(FBW_BASE_URL);

template <typename R> bool is_ready(std::future<R> const &f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void MetarManager::procFutures()
{
    for (auto it = metarsFutures.begin(); it != metarsFutures.end(); it++)
    {
        if (!it->second.valid())
            continue;
        if (is_ready(it->second))
        {
            readyMetars[it->first].first = true;
            readyMetars[it->first].second = it->second.get();
        }
        else
        {
            readyMetars[it->first].first = false;
        }
    }
}

void MetarManager::addAirports(const quicktype::Airac &airac)
{
    for (int i = 0; i < airac.size(); i++)
    {
        metarsFutures[airac[i].icao] = std::async(std::launch::async, MetarManager::fetchMetar, airac[i].icao, i);
    }
}

void MetarManager::removeAirport(const std::string &airport)
{
    metarsFutures.erase(airport);
}

void MetarManager::updateMetar(const std::string &airport)
{
    metarsFutures[airport] = std::async(std::launch::async, MetarManager::fetchMetar, airport, 0);
}

void MetarManager::updateMetars()
{
    int i = 0;
    for (auto it = metarsFutures.begin(); it != metarsFutures.end(); it++, i++)
    {
        metarsFutures[it->first] = std::async(std::launch::async, MetarManager::fetchMetar, it->first, i);
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
