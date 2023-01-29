#pragma once

#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Quicktype.hpp"

std::vector<std::string> splitString(const std::string &str, char sep);
void parseMetar(quicktype::Airport &airport, const std::string &metar);
