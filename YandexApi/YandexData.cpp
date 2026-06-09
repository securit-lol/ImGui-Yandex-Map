#include "YandexData.h"
#include <chrono>
#include <format>
#include <iostream>
#include <string>

void ya_data::TryGetYandexCode(const nlohmann::json& j, std::string& target)
{
  if (j.contains("codes") && j["codes"].is_object()) {
    const auto& codes = j["codes"];
    if (codes.contains("yandex_code") && codes["yandex_code"].is_string()) {
      target = codes.value("yandex_code", "");
    }
  }
}

ya_data::Station ya_data::Station::from_json(const nlohmann::json& j)
{
  Station s;

  TryGetValue(j, "title", s.title);
  TryGetValue(j, "station_type", s.station_type);
  TryGetValue(j, "transport_type", s.transport_type);
  TryGetValue(j, "latitude", s.latitude);
  TryGetValue(j, "longitude", s.longitude);
  TryGetYandexCode(j, s.yandex_code);

  return s;
}

ya_data::Lement ya_data::Lement::from_json(const nlohmann::json& j)
{
  Lement s;

  TryGetValue(j, "title", s.title);
  TryGetYandexCode(j, s.yandex_code);

  return s;
}

ya_data::Region ya_data::Region::from_json(const nlohmann::json& j)
{
  Region s;

  TryGetValue(j, "title", s.title);
  TryGetYandexCode(j, s.yandex_code);

  return s;
}

ya_data::Country ya_data::Country::from_json(const nlohmann::json& j)
{
  Country s;

  TryGetValue(j, "title", s.title);
  TryGetYandexCode(j, s.yandex_code);

  return s;
}

ya_data::City ya_data::City::from_json(const nlohmann::json& j)
{
  City result;
  TryGetValue(j, "title", result.title);
  TryGetValue(j, "distance", result.distance);
  TryGetValue(j, "lat", result.latitude);
  TryGetValue(j, "lng", result.longitude);
  TryGetValue(j, "code", result.yandex_code);
  return result;
}