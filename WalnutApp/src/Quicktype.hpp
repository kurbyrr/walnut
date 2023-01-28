//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     ApiRes data = nlohmann::json::parse(jsonString);

#pragma once

#include "json.hpp"

namespace quicktype
{
using nlohmann::json;

#ifndef NLOHMANN_UNTYPED_quicktype_HELPERHELPER
#define NLOHMANN_UNTYPED_quicktype_HELPERHELPER
inline json get_untyped(const json &j, const char *property)
{
    if (j.find(property) != j.end())
    {
        return j.at(property).get<json>();
    }
    return json();
}

inline json get_untyped(const json &j, std::string property)
{
    return get_untyped(j, property.data());
}
#endif

struct ApiRes
{
    std::string icao;
    std::string metar;
    std::string source;
};
} // namespace quicktype

namespace quicktype
{
void from_json(const json &j, ApiRes &x);
void to_json(json &j, const ApiRes &x);

inline void from_json(const json &j, ApiRes &x)
{
    x.icao = j.at("icao").get<std::string>();
    x.metar = j.at("metar").get<std::string>();
    x.source = j.at("source").get<std::string>();
}

inline void to_json(json &j, const ApiRes &x)
{
    j = json::object();
    j["icao"] = x.icao;
    j["metar"] = x.metar;
    j["source"] = x.source;
}
} // namespace quicktype

namespace quicktype
{
using nlohmann::json;

struct ConfigData
{
    std::vector<std::string> airports;
};
} // namespace quicktype

namespace quicktype
{
void from_json(const json &j, ConfigData &x);
void to_json(json &j, const ConfigData &x);

inline void from_json(const json &j, ConfigData &x)
{
    x.airports = j.at("airports").get<std::vector<std::string>>();
}

inline void to_json(json &j, const ConfigData &x)
{
    j = json::object();
    j["airports"] = x.airports;
}
} // namespace quicktype