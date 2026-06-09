#include "ThreadWorker.h"
#include "ImOsmDemoApp.h"
#include "YandexApi/YandexData.h"
#include <imgui_internal.h>

ThreadWorker::ThreadWorker(const std::shared_ptr<ImOsm::MapPlot>& mapPlotPtr)
    : cache(1000)
{
  worker = std::thread(&ThreadWorker::threadFunction, this, mapPlotPtr);
}

ThreadWorker::~ThreadWorker()
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    stop = true;
  }
  cv.notify_all();
  if (worker.joinable()) {
    worker.join();
  }
}

void ThreadWorker::execute(std::function<void()> task)
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(std::move(task));
    hasWork = true;
  }
  cv.notify_one();
}

static inline float ImLength(const ImVec2& a, const ImVec2& b)
{
  return hypotf(a.x - b.x, a.y - b.y);
}

std::string ThreadWorker::GetCurrentDate()
{
  auto now = std::chrono::system_clock::now();
  auto today = std::chrono::floor<std::chrono::days>(now);
  return std::format("{:%Y-%m-%d}", today);
}

void ThreadWorker::GetAllStations()
{
  cpr::Parameters parameters = {
    { "apikey", kApiKey },
    { "format", "json" },
    { "lang", "ru_RU" }
  };

  session.SetUrl(cpr::Url { "https://api.rasp.yandex-net.ru/v3.0/stations_list/" });
  session.SetParameters(parameters);
  session.SetBody(cpr::Body { "" });

  cpr::Response response = session.Get();
  if (response.status_code != 200) {
    std::cout << "Error getting all stations" << std::endl;
    return;
  }

  nlohmann::json json_data = nlohmann::json::parse(response.text);

  if (!json_data.contains("countries") || !json_data["countries"].is_array())
    return;

  for (const auto& country : json_data["countries"]) {
    ya_data::Country _country = ya_data::Country::from_json(country);
    if (_country.title.empty() || !country.contains("regions") || !country["regions"].is_array())
      continue;
    for (const auto& region : country["regions"]) {
      ya_data::Region _region = ya_data::Region::from_json(region);
      if (_region.title.empty() || !region.contains("settlements") || !region["settlements"].is_array())
        continue;
      for (const auto& settlement : region["settlements"]) {
        ya_data::Lement _lement = ya_data::Lement::from_json(settlement);
        if (_lement.title.empty() || !settlement.contains("stations") || !settlement["stations"].is_array())
          continue;
        for (const auto& station : settlement["stations"]) {
          if (std::string st_type = station.value("station_type", "");
              st_type == "station" || st_type == "train_station" || st_type == "airport" || st_type == "bus_station" || st_type == "bus_stop") {
            ya_data::Station _station = ya_data::Station::from_json(station);

            _lement.stations.push_back(_station);
            _lement.stations_count += 1;
            _lement.latitude += _station.latitude;
            _lement.longitude += _station.longitude;
          }
        }
        if (_lement.stations_count) {
          _lement.latitude /= _lement.stations_count;
          _lement.longitude /= _lement.stations_count;
        }
        _region.settlements.push_back(_lement);
      }
      _country.regions.push_back(_region);
    }
    all_country_data.push_back(_country);
  }
}

std::unique_ptr<const ya_data::City> ThreadWorker::GetNearestCity(const ya_data::Lement& lement, float distance)
{
  cpr::Parameters parameters = {
    { "apikey", kApiKey },
    { "format", "json" },
    { "lang", "ru_RU" },
    { "lat", std::to_string(lement.latitude) },
    { "lng", std::to_string(lement.longitude) },
    { "distance", std::to_string(distance) }
  };

  session.SetUrl(cpr::Url { "https://api.rasp.yandex-net.ru/v3.0/nearest_settlement/" });
  session.SetParameters(parameters);
  session.SetBody(cpr::Body { "" });

  cpr::Response response = session.Get();

  if (response.status_code == 404) {
    std::cout << "Error getting nearest city" << std::endl;
    return nullptr;
  }

  nlohmann::json json_data = nlohmann::json::parse(response.text);

  std::unique_ptr<ya_data::City> result = std::make_unique<ya_data::City>(ya_data::City::from_json(json_data));
  result->owner_title = lement.title;
  return result;
}

ya_data::ScheduleResponse ThreadWorker::GetWay(const std::string& fromCity, const std::string& toCity, const std::string& date)
{
  std::string map_key = fromCity + "|" + toCity;

  ya_data::ScheduleResponse result;

  std::string day_now = GetCurrentDate();
  if (cache.find(map_key, result) && result.search.date == day_now) {
    return result;
  }

  cpr::Parameters parameters = {
    { "apikey", kApiKey },
    { "format", "json" },
    { "lang", "ru_RU" },
    { "from", fromCity },
    { "to", toCity },
    { "transfers", "true" },
    { "date", date }
  };

  session.SetUrl(cpr::Url { "https://api.rasp.yandex-net.ru/v3.0/search/" });
  session.SetParameters(parameters);
  session.SetBody(cpr::Body { "" });

  cpr::Response response = session.Get();

  if (response.status_code != 200) {
    std::cout << "Error getting way" << std::endl;
    return { };
  }

  nlohmann::json json_data = nlohmann::json::parse(response.text);

  if (json_data.contains("segments") && json_data["segments"].is_array()) {
    for (const auto& item : json_data["segments"]) {
      ya_data::ScheduleResponse::Segment segment;

      if (item.contains("departure_from") && item["departure_from"].is_object()) {
        auto& departure_from = item["departure_from"];

        ya_data::TryGetValue(departure_from, "code", segment.departure_from.code);
        ya_data::TryGetValue(departure_from, "title", segment.departure_from.title);
        ya_data::TryGetValue(departure_from, "transport_type", segment.departure_from.transport_type);
        ya_data::TryGetValue(departure_from, "type", segment.departure_from.type);
      }

      if (item.contains("arrival_to") && item["arrival_to"].is_object()) {
        auto& arrival_to = item["arrival_to"];

        ya_data::TryGetValue(arrival_to, "code", segment.arrival_to.code);
        ya_data::TryGetValue(arrival_to, "title", segment.arrival_to.title);
        ya_data::TryGetValue(arrival_to, "transport_type", segment.arrival_to.transport_type);
        ya_data::TryGetValue(arrival_to, "type", segment.arrival_to.type);
      }

      ya_data::TryGetValue(item, "departure", segment.departure);
      ya_data::TryGetValue(item, "arrival", segment.arrival);
      ya_data::TryGetValue(item, "has_transfers", segment.has_transfers);

      if (item.contains("details") && item["details"].is_array()) {
        for (const auto& detail_item : item["details"]) {
          ya_data::ScheduleResponse::Detail detail;

          ya_data::TryGetValue(detail_item, "is_transfer", detail.is_transfer);
          ya_data::TryGetValue(detail_item, "duration", detail.duration);
          ya_data::TryGetValue(detail_item, "departure", detail.departure);
          ya_data::TryGetValue(detail_item, "arrival", detail.arrival);

          if (detail_item.contains("thread") && detail_item["thread"].is_object()) {
            const auto& thread = detail_item["thread"];
            ya_data::TryGetValue(thread, "title", detail.thread.title);
            ya_data::TryGetValue(thread, "transport_type", detail.thread.transport_type);
          }

          if (detail_item.contains("from") && detail_item["from"].is_object()) {
            auto& from = detail_item["from"];

            ya_data::TryGetValue(from, "type", detail.from.type);
            ya_data::TryGetValue(from, "title", detail.from.title);
            ya_data::TryGetValue(from, "code", detail.from.code);
          }

          if (detail_item.contains("to") && detail_item["to"].is_object()) {
            auto& to = detail_item["to"];
            ya_data::TryGetValue(to, "type", detail.to.type);
            ya_data::TryGetValue(to, "title", detail.to.title);
            ya_data::TryGetValue(to, "code", detail.to.code);
          }

          if (detail_item.contains("transfer_point") && detail_item["transfer_point"].is_object()) {
            auto& point = detail_item["transfer_point"];

            ya_data::TryGetValue(point, "type", detail.transfer_point.type);
            ya_data::TryGetValue(point, "title", detail.transfer_point.title);
            ya_data::TryGetValue(point, "code", detail.transfer_point.code);
          }

          if (detail_item.contains("transfer_from") && detail_item["transfer_from"].is_object()) {
            auto& from = detail_item["transfer_from"];

            ya_data::TryGetValue(from, "type", detail.transfer_from.type);
            ya_data::TryGetValue(from, "title", detail.transfer_from.title);
            ya_data::TryGetValue(from, "code", detail.transfer_from.code);
            ya_data::TryGetValue(from, "transport_type", detail.transfer_from.transport_type);
          }

          if (detail_item.contains("transfer_to") && detail_item["transfer_to"].is_object()) {
            auto& to = detail_item["transfer_to"];

            ya_data::TryGetValue(to, "type", detail.transfer_to.type);
            ya_data::TryGetValue(to, "title", detail.transfer_to.title);
            ya_data::TryGetValue(to, "code", detail.transfer_to.code);
            ya_data::TryGetValue(to, "transport_type", detail.transfer_to.transport_type);
          }

          segment.details.push_back(detail);
        }
      } else {
        continue;
      }
      result.segments.push_back(segment);
    }
  }

  if (json_data.contains("search") && json_data["search"].is_object()) {
    auto& search = json_data["search"];
    if (search.contains("date") && search["date"].is_string())
      result.search.date = search["date"].get<std::string>();

    if (search.contains("from") && search["from"].is_object()) {
      auto& from = search["from"];

      ya_data::TryGetValue(from, "code", result.search.from.code);
      ya_data::TryGetValue(from, "type", result.search.from.type);
      ya_data::TryGetValue(from, "title", result.search.from.title);
    }

    if (search.contains("to") && search["to"].is_object()) {
      auto& to = search["to"];

      ya_data::TryGetValue(to, "code", result.search.to.code);
      ya_data::TryGetValue(to, "type", result.search.to.type);
      ya_data::TryGetValue(to, "title", result.search.to.title);
    }
  }
  cache.insert(map_key, result);
  return result;
}

void ThreadWorker::CheckNearCity(const ImVec2& mouse_pos)
{
  execute([this, mouse_pos]() {
    const ya_data::Lement* nearest_point = nullptr;
    {
      float min_dist = std::numeric_limits<float>::infinity();
      for (const auto& country : all_country_data) {
        for (const auto& region : country.regions) {
          for (const auto& lement : region.settlements) {
            float dist = ImLength(ImVec2(mouse_pos.x, mouse_pos.y), { lement.latitude, lement.longitude });
            if (dist < min_dist) {
              min_dist = dist;
              nearest_point = &lement;
            }
          }
        }
      }
    }
    if (!nearest_point)
      return;

    std::lock_guard<std::mutex> lock(city_mtx);
    auto nearest_city = GetNearestCity(*nearest_point, 50);
    if (nearest_city) {
      if (switcher) {
        near_city_1 = std::move(nearest_city);
      } else {
        near_city_2 = std::move(nearest_city);
      }
      switcher = !switcher;
    }
    if (near_city_1 && near_city_2 && near_city_1->yandex_code[0] == 'c' && near_city_2->yandex_code[0] == 'c' && near_city_1->yandex_code != near_city_2->yandex_code) {
      std::lock_guard<std::mutex> lock2(way_mtx);
      way_data = GetWay(near_city_1->yandex_code, near_city_2->yandex_code, GetCurrentDate());
    }
  });
}

void ThreadWorker::UpdateNearCity(const ya_data::Lement* nearest_point)
{
  execute([this, nearest_point]() {
    std::lock_guard<std::mutex> lock(city_mtx);
    auto nearest_city = GetNearestCity(*nearest_point, 50);
    if (nearest_city) {
      if (switcher) {
        near_city_1 = std::move(nearest_city);
      } else {
        near_city_2 = std::move(nearest_city);
      }
      switcher = !switcher;
    }
    if (near_city_1 && near_city_2 && near_city_1->yandex_code[0] == 'c' && near_city_2->yandex_code[0] == 'c' && near_city_1->yandex_code != near_city_2->yandex_code) {
      std::lock_guard<std::mutex> lock2(way_mtx);
      way_data = GetWay(near_city_1->yandex_code, near_city_2->yandex_code, GetCurrentDate());
    }
  });
}

void ThreadWorker::threadFunction(std::shared_ptr<ImOsm::MapPlot> mapPlot)
{
  if (kApiKey.size() == 0) {
    std::cout << "Wrong api key. Edit (kApiKey) in YandexApi->YandexData.h" << std::endl;
    return;
  }

  session.SetHeader(cpr::Header { { "Yandex-Api", "app" } });
  session.SetTimeout(cpr::Timeout { 10000 });

  GetAllStations();
  for (const auto& country : all_country_data) {
    for (const auto& region : country.regions) {
      for (const auto& lement : region.settlements) {
        mapPlot->addPoint(lement.title, lement.longitude, lement.latitude);
      }
    }
  }
  mapPlot->getReadyToDraw();

  while (!stop) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() {
      return hasWork || stop;
    });

    if (stop)
      break;

    while (!tasks.empty()) {
      auto task = std::move(tasks.front());
      tasks.pop();
      lock.unlock();
      task();
      lock.lock();
    }
    hasWork = false;
  }
}