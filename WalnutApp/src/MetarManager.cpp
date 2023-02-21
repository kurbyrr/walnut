#include "MetarManager.h"

httplib::Client MetarManager::httpClient(FBW_BASE_URL);

template <typename R> bool is_ready(std::future<R> const &f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void MetarManager::procFutures()
{
    if (metarsFutures.empty())
        return;

    for (auto it = metarsFutures.begin(); it != metarsFutures.end();)
    {
        if (is_ready(it->second))
        {
            auto &readyMetar = readyMetars[it->first];
            readyMetar.first = true;
            readyMetar.second = it->second.get();
            metarsFutures.erase(it++);
        }
        else
        {
            readyMetars[it->first].first = false;
            it++;
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
    int retries = 5;
    while (retries)
    {
        httplib::Result res = httpClient.Get("/metar/" + airport, headers);
        if (!res || res->status != 200) // Short circuiting
        {
            retries--;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        quicktype::ApiRes json = nlohmann::json::parse(res->body);

        return json.metar;
    }

    return "An error has occured!";
}
