#pragma once

#include <regex>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct RunwayData
{
    std::string RNAVSID;
    std::string CONVSID;

    std::string RNAVSTAR;
    std::string CONVSTAR;

    int number;
};

struct TMAInfo
{
    RunwayData LFBD;
    RunwayData LFBH;
    RunwayData LFBE;
};

std::vector<std::string> splitString(const std::string &str, char sep);
void parseMetarArray(const std::vector<std::string> &metarArray);
