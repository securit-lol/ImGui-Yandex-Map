#pragma once
#include <chrono>
#include <cpr/cpr.h>
#include <format>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

const std::string kApiKey = "";

namespace ya_data {

  struct City {
    float distance = -1.0f;
    float latitude = 0.0f;
    float longitude = 0.0f;
    std::string yandex_code;
    std::string title;
    std::string owner_title = "";

    static City from_json(const nlohmann::json& j);
  };

  struct Station {
    std::string yandex_code;
    std::string station_type;
    std::string title;
    std::string transport_type;
    float longitude = 0.0f;
    float latitude = 0.0f;

    static Station from_json(const nlohmann::json& j);
  };

  struct Lement {
    int stations_count = 0;
    float latitude = 0.0f;
    float longitude = 0.0f;
    std::string title;
    std::string yandex_code;
    std::vector<Station> stations;

    static Lement from_json(const nlohmann::json& j);
  };

  struct Region {
    std::string title;
    std::string yandex_code;
    std::vector<Lement> settlements;

    static Region from_json(const nlohmann::json& j);
  };

  struct Country {
    std::string title;
    std::string yandex_code;
    std::vector<Region> regions;

    static Country from_json(const nlohmann::json& j);
  };

  struct ScheduleResponse {
    struct SearchObject {
      std::string code;
      std::string type;
      std::string title;
    };

    struct Search {
      std::string date;
      SearchObject to;
      SearchObject from;
    };

    struct Location {
      std::string type;
      std::string title;
      std::string code;
      std::string transport_type;
    };

    struct Detail {
      struct Thread {
        std::string title;
        std::string transport_type;
      };
      bool is_transfer = false;
      float duration = 0.0f;

      std::string departure;
      std::string arrival;

      Thread thread;
      SearchObject from;
      SearchObject to;

      SearchObject transfer_point;
      Location transfer_from;
      Location transfer_to;
    };

    struct Segment {
      bool has_transfers = false;
      std::string departure;
      std::string arrival;
      Location departure_from;
      Location arrival_to;
      std::vector<Detail> details;
    };

    Search search;
    std::vector<Segment> segments;
  };

  template <typename T>
  bool TryGetValue(const nlohmann::json& j, const std::string& key, T& target);
  void TryGetYandexCode(const nlohmann::json& j, std::string& target);

} // namespace ya_data

template <typename T>
bool ya_data::TryGetValue(const nlohmann::json& j, const std::string& key, T& target)
{
  if (!j.contains(key))
    return false;

  const auto& value = j[key];

  if constexpr (std::is_same_v<T, std::string>) {
    if (value.is_string()) {
      target = value.get<std::string>();
      return true;
    }
  } else if constexpr (std::is_same_v<T, bool>) {
    if (value.is_boolean()) {
      target = value.get<bool>();
      return true;
    }
  } else if constexpr (std::is_arithmetic_v<T>) {
    if (value.is_number()) {
      target = value.get<T>();
      return true;
    }
  }
  return false;
}

template <typename Key, typename Value>
class DeadthMap {
  private:
  std::map<Key, Value> data;
  std::vector<Key> insertion_order;
  size_t max_size;

  public:
  DeadthMap(size_t max)
      : max_size(max)
  {
  }

  void insert(const Key& key, const Value& value)
  {
    auto it = data.find(key);

    if (it != data.end()) {
      it->second = value;
      return;
    }

    if (data.size() >= max_size) {
      Key oldest = insertion_order.front();
      data.erase(oldest);
      insertion_order.erase(insertion_order.begin());
    }

    data[key] = value;
    insertion_order.push_back(key);
  }

  bool find(const Key& key, Value& value) const
  {
    auto it = data.find(key);
    if (it == data.end())
      return false;
    value = it->second;
    return true;
  }

  size_t size() const { return data.size(); }
};