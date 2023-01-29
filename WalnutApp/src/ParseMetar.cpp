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

void parseMetarArray(const std::vector<std::string> &metarArray, std::unordered_map<std::string, int> &runwaysInUse)
{
    std::regex windRegex("^(\\d\\d\\d|VRB)P?(\\d+)(?:G(\\d+))?(KT|MPS|KPH)");
    std::smatch match;
    for (auto &metarPart : metarArray)
    {
        if (std::regex_match(metarPart, match, windRegex))
        {
            glm::vec2 wind = {match[1] == "VRB" ? 180 : std::stoi(match[1]), std::stoi(match[2])};
        }
    }
}
