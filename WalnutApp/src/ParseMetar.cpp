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

template <typename T> bool inRange(T low, T high, T x)
{
    return (low <= x && x <= high);
}

void parseMetar(quicktype::Airport &airport, const std::string &metar)
{
    std::vector<std::string> metarArray = splitString(metar, ' ');

    std::regex windRegex("^(\\d\\d\\d|VRB)P?(\\d+)(?:G(\\d+))?(KT|MPS|KPH)");
    std::smatch match;
    for (auto &metarPart : metarArray)
    {
        if (std::regex_match(metarPart, match, windRegex))
        {
            glm::vec2 wind = {match[1] == "VRB" ? 180 : std::stoi(match[1]), std::stoi(match[2])}; // x: deg / y: speed
            glm::vec2 NEProjection = {wind.y * std::cos(glm::radians(wind.x)), wind.y * std::sin(glm::radians(wind.x))};

            for (auto &runway : airport.runways)
            {
                float runwayAngle = glm::radians((float)runway.qfu);
                glm::mat2 rotationMatrix = {{std::cos(runwayAngle), -std::sin(runwayAngle)},
                                            {std::cos(runwayAngle), std::sin(runwayAngle)}};
                glm::vec2 rotatedWindVec = rotationMatrix * NEProjection;
            }
        }
    }
}
