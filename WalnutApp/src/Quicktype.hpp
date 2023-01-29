//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     ApiRes data = nlohmann::json::parse(jsonString);

#pragma once

#include "json.hpp"
#include <optional>

// FBW API
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

// AIRAC
#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann
{
template <typename T> struct adl_serializer<std::shared_ptr<T>>
{
    static void to_json(json &j, const std::shared_ptr<T> &opt)
    {
        if (!opt)
            j = nullptr;
        else
            j = *opt;
    }

    static std::shared_ptr<T> from_json(const json &j)
    {
        if (j.is_null())
            return std::make_shared<T>();
        else
            return std::make_shared<T>(j.get<T>());
    }
};
template <typename T> struct adl_serializer<std::optional<T>>
{
    static void to_json(json &j, const std::optional<T> &opt)
    {
        if (!opt)
            j = nullptr;
        else
            j = *opt;
    }

    static std::optional<T> from_json(const json &j)
    {
        if (j.is_null())
            return std::make_optional<T>();
        else
            return std::make_optional<T>(j.get<T>());
    }
};
} // namespace nlohmann
#endif

namespace quicktype
{
using nlohmann::json;

#ifndef NLOHMANN_OPTIONAL_quicktype_
#define NLOHMANN_OPTIONAL_quicktype_
template <typename T> inline std::shared_ptr<T> get_heap_optional(const json &j, const char *property)
{
    auto it = j.find(property);
    if (it != j.end() && !it->is_null())
    {
        return j.at(property).get<std::shared_ptr<T>>();
    }
    return std::shared_ptr<T>();
}

template <typename T> inline std::shared_ptr<T> get_heap_optional(const json &j, std::string property)
{
    return get_heap_optional<T>(j, property.data());
}
template <typename T> inline std::optional<T> get_stack_optional(const json &j, const char *property)
{
    auto it = j.find(property);
    if (it != j.end() && !it->is_null())
    {
        return j.at(property).get<std::optional<T>>();
    }
    return std::optional<T>();
}

template <typename T> inline std::optional<T> get_stack_optional(const json &j, std::string property)
{
    return get_stack_optional<T>(j, property.data());
}
#endif

struct Runway
{
    std::string name;
    int64_t qfu;
    std::optional<std::string> sidrnav;
    std::optional<std::string> sidconv;
    std::string starrnav;
    std::optional<std::string> starconv;
};

struct Airport
{
    std::string icao;
    std::vector<std::string> sidwpt;
    std::vector<std::string> starwpt;
    std::vector<Runway> runways;
};

using Airac = std::vector<Airport>;
} // namespace quicktype

namespace quicktype
{
void from_json(const json &j, Runway &x);
void to_json(json &j, const Runway &x);

void from_json(const json &j, Airport &x);
void to_json(json &j, const Airport &x);

inline void from_json(const json &j, Runway &x)
{
    x.name = j.at("Name").get<std::string>();
    x.qfu = j.at("QFU").get<int64_t>();
    x.sidrnav = get_stack_optional<std::string>(j, "SIDRNAV");
    x.sidconv = get_stack_optional<std::string>(j, "SIDCONV");
    x.starrnav = j.at("STARRNAV").get<std::string>();
    x.starconv = get_stack_optional<std::string>(j, "STARCONV");
}

inline void to_json(json &j, const Runway &x)
{
    j = json::object();
    j["Name"] = x.name;
    j["QFU"] = x.qfu;
    j["SIDRNAV"] = x.sidrnav;
    j["SIDCONV"] = x.sidconv;
    j["STARRNAV"] = x.starrnav;
    j["STARCONV"] = x.starconv;
}

inline void from_json(const json &j, Airport &x)
{
    x.icao = j.at("ICAO").get<std::string>();
    x.sidwpt = j.at("SIDWPT").get<std::vector<std::string>>();
    x.starwpt = j.at("STARWPT").get<std::vector<std::string>>();
    x.runways = j.at("Runways").get<std::vector<Runway>>();
}

inline void to_json(json &j, const Airport &x)
{
    j = json::object();
    j["ICAO"] = x.icao;
    j["SIDWPT"] = x.sidwpt;
    j["STARWPT"] = x.starwpt;
    j["Runways"] = x.runways;
}
} // namespace quicktype
