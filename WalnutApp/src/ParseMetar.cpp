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

void LFBDWindLogic(const glm::vec2 &wind)
{
    glm::vec2 NEProjection = {wind.y * std::cos(glm::radians(wind.x)), wind.y * std::sin(glm::radians(wind.x))};
    // RWY23-05 projection
    constexpr float rwy23angle = glm::radians(45.0f);
    glm::mat2 rotationMatrix = {{std::cos(rwy23angle), -std::sin(rwy23angle)},
                                {std::sin(rwy23angle), std::cos(rwy23angle)}};
    glm::vec2 rotatedVector = rotationMatrix * NEProjection;
    rotatedVector.x *= -1;

    if (rotatedVector.x < -5.0f)
    {
        if (rotatedVector.y < 15.0f)
        {
            // RWY11
        }
        else if (inRange(-15.0f, 15.0f, rotatedVector.y))
        {
            // RWY05
        }
        else
        {
            // RWY29
        }
    }
    else
    {
        if (rotatedVector.y < 15.0f)
        {
            // RWY11
        }
        else if (inRange(-15.0f, 15.0f, rotatedVector.y))
        {
        }
    }
}

void LFBHBEWindLogic(const glm::vec2 &wind)
{
}

void parseMetarArray(const std::vector<std::string> &metarArray)
{
    std::regex windRegex("^(\\d\\d\\d|VRB)P?(\\d+)(?:G(\\d+))?(KT|MPS|KPH)");
    std::smatch match;
    for (auto &metarPart : metarArray)
    {
        if (std::regex_match(metarPart, match, windRegex))
        {
            glm::vec2 wind = {match[1] == "VRB" ? 180 : std::stoi(match[1]), std::stoi(match[2])};

            if (metarArray[0] == "LFBD")
                LFBDWindLogic(wind);
            else if (metarArray[0] == "LFBH" || metarArray[0] == "LFBE")
                LFBHBEWindLogic(wind);
        }
    }
}
