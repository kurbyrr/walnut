#include "ParseMetar.h"

std::vector<std::string> splitString(const std::string &str, char sep)
{
    std::vector<std::string> result;
    std::string buffer;

    for (char ch : str)
    {
        if (ch == sep)
        {
            if (!buffer.empty())
            {
                result.push_back(buffer);
                buffer.clear();
            }
        }
        else
            buffer += ch;
    }
    if (!buffer.empty())
        result.push_back(buffer);

    return result;
}

void parseMetar(quicktype::Airport &airport, const std::string &metar)
{
    if (airport.runwayOverriden)
        return;

    std::vector<std::string> metarArray = splitString(metar, ' ');

    std::regex windRegex("^(\\d\\d\\d|VRB)P?(\\d+)(?:G(\\d+))?(KT|MPS|KPH)");
    std::smatch match;
    for (auto &metarPart : metarArray)
    {
        if (std::regex_match(metarPart, match, windRegex))
        {
            glm::vec2 wind = {match[1] == "VRB" ? 180 : std::stoi(match[1]), std::stoi(match[2])}; // x: deg / y: speed

            std::vector<std::pair<int, float>> runwayScores;
            for (int i = 0; i < airport.runways.size(); i++)
            {
                const quicktype::Runway &runway = airport.runways[i];

                float frontWind = wind.y * std::cos(glm::radians(wind.x - runway.qfu));
                float sideWind = wind.y * std::sin(glm::radians(wind.x - runway.qfu));

                float runwayScore = frontWind - std::abs(sideWind);

                runwayScores.emplace_back(i, runwayScore);
            }

            std::sort(runwayScores.begin(), runwayScores.end(),
                      [](std::pair<int, float> &a, std::pair<int, float> &b) { return a.second > b.second; });

            airport.selectedRunwayIndex = runwayScores[0].first;
        }
    }
}
