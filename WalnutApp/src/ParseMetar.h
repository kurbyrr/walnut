#pragma once

#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

std::vector<std::string> splitString(const std::string &str, char sep);
void parseMetarArray(const std::vector<std::string> &metarArray, std::unordered_map<std::string, int> &runwaysInUse);
