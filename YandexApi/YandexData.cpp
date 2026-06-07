#include "YandexData.h"
#include <iostream>
#include <format>
#include <string>
#include <chrono>


namespace api {
    void TryGetYandexCode(const nlohmann::json& j, std::string& target) {
        if (j.contains("codes") && j["codes"].is_object()) {
            const auto& codes = j["codes"];
            if (codes.contains("yandex_code") && codes["yandex_code"].is_string()) {
                target = codes.value("yandex_code", "");
            }
        }
    }

    std::string GetCurrentDate() {
        auto now = std::chrono::system_clock::now();
        auto today = std::chrono::floor<std::chrono::days>(now);
        return std::format("{:%Y-%m-%d}", today);
    }
    
    DeadthMap<std::string, ScheduleResponse> cache = DeadthMap<std::string, ScheduleResponse>(1000);

    Station Station::from_json(const nlohmann::json& j) {
        Station s;

        TryGetValue(j, "title", s.title);
        TryGetValue(j, "station_type", s.station_type);
        TryGetValue(j, "transport_type", s.transport_type);
        TryGetValue(j, "latitude", s.latitude);
        TryGetValue(j, "longitude", s.longitude);
        TryGetYandexCode(j, s.yandex_code);

        return s;
    }

    Lement Lement::from_json(const nlohmann::json& j) {
        Lement s;

        TryGetValue(j, "title", s.title);
        TryGetYandexCode(j, s.yandex_code);

        return s;
    }

    Region Region::from_json(const nlohmann::json& j) {
        Region s;

        TryGetValue(j, "title", s.title);
        TryGetYandexCode(j, s.yandex_code);

        return s;
    }

    Country Country::from_json(const nlohmann::json& j) {
        Country s;

        TryGetValue(j, "title", s.title);
        TryGetYandexCode(j, s.yandex_code);

        return s;
    }

    City City::from_json(const nlohmann::json& j) {
        City result;
        TryGetValue(j, "title", result.title);
        TryGetValue(j, "distance", result.distance);
        TryGetValue(j, "lat", result.latitude);
        TryGetValue(j, "lng", result.longitude);
        TryGetValue(j, "code", result.yandex_code);
        return result;
    }


    namespace data {
        std::vector<Country> all_country_data = {};
        ScheduleResponse way_data = {};
        std::unique_ptr<const api::City> near_city_1 = nullptr;
        std::unique_ptr<const api::City> near_city_2 = nullptr;
        std::mutex way_mtx = {};
        std::mutex city_mtx = {};
        bool switcher = false;
    }

    void GetAllStations() {
        std::string url = "https://api.rasp.yandex-net.ru/v3.0/stations_list/";
        cpr::Parameters parameters = {
            {"apikey", kApiKey},
            {"format", "json"},
            {"lang", "ru_RU"}
        };

        cpr::Response response = cpr::Get(cpr::Url{ url }, parameters);
        if (response.status_code != 200) {
            std::cout << "Error getting all stations" << std::endl;
            return;
        }

        nlohmann::json json_data = nlohmann::json::parse(response.text);

        if (!json_data.contains("countries") || !json_data["countries"].is_array()) return;

        for (const auto& country : json_data["countries"]) {
            Country _country = Country::from_json(country);
            if (_country.title.empty() || !country.contains("regions") || !country["regions"].is_array()) continue;
            for (const auto& region : country["regions"]) {
                Region _region = Region::from_json(region);
                if (_region.title.empty() || !region.contains("settlements") || !region["settlements"].is_array()) continue;
                for (const auto& settlement : region["settlements"]) {
                    Lement _lement = Lement::from_json(settlement);
                    if (_lement.title.empty() || !settlement.contains("stations") || !settlement["stations"].is_array()) continue;
                    for (const auto& station : settlement["stations"]) {
                        if (std::string st_type = station.value("station_type", "");
                            st_type == "station" || st_type == "train_station" || st_type == "airport" || st_type == "bus_station" || st_type == "bus_stop") {
                            Station _station = Station::from_json(station);

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
            data::all_country_data.push_back(_country);
        }
    }

    std::unique_ptr<const City> GetNearestCity(const Lement& lement, float distance) {
        std::string url = "https://api.rasp.yandex-net.ru/v3.0/nearest_settlement/";
        cpr::Parameters parameters = {
            {"apikey", kApiKey},
            {"format", "json"},
            {"lang", "ru_RU"},
            {"lat", std::to_string(lement.latitude)},
            {"lng", std::to_string(lement.longitude)},
            {"distance", std::to_string(distance)}
        };

        cpr::Response response = cpr::Get(cpr::Url{ url }, parameters);
        if (response.status_code == 404) {
            std::cout << "Error getting nearest city" << std::endl;
            return nullptr;
        }

        nlohmann::json json_data = nlohmann::json::parse(response.text);

        std::unique_ptr<City> result = std::make_unique<City>(City::from_json(json_data));
        result->owner_title = lement.title;
        return result;
    }


    ScheduleResponse GetWay(const std::string& fromCity, const std::string& toCity, const std::string& date) {
        std::string url = "https://api.rasp.yandex-net.ru/v3.0/search/";
        cpr::Parameters parameters = {
            {"apikey", kApiKey},
            {"format", "json"},
            {"lang", "ru_RU"},
            {"from", fromCity},
            {"to", toCity},
            {"transfers", "true"},
            { "date", date }
        };
        
        std::string map_key = fromCity + "|" + toCity;

        ScheduleResponse result;
        
        //auto it = cache::data.find(map_key, result);
        std::string day_now = GetCurrentDate();
        if (cache.find(map_key, result) && result.search.date == day_now) {
            return result;
        }

        cpr::Response response = cpr::Get(cpr::Url{ url }, parameters);


        if (response.status_code != 200) {
            std::cout << "Error getting way" << std::endl;
            return {};
        }

        nlohmann::json json_data = nlohmann::json::parse(response.text);

        if (json_data.contains("segments") && json_data["segments"].is_array()) {
            for (const auto& item : json_data["segments"]) {
                ScheduleResponse::Segment segment;

                if (item.contains("departure_from") && item["departure_from"].is_object()) {
                    auto& departure_from = item["departure_from"];

                    TryGetValue(departure_from, "code", segment.departure_from.code);
                    TryGetValue(departure_from, "title", segment.departure_from.title);
                    TryGetValue(departure_from, "transport_type", segment.departure_from.transport_type);
                    TryGetValue(departure_from, "type", segment.departure_from.type);
                }

                if (item.contains("arrival_to") && item["arrival_to"].is_object()) {
                    auto& arrival_to = item["arrival_to"];

                    TryGetValue(arrival_to, "code", segment.arrival_to.code);
                    TryGetValue(arrival_to, "title", segment.arrival_to.title);
                    TryGetValue(arrival_to, "transport_type", segment.arrival_to.transport_type);
                    TryGetValue(arrival_to, "type", segment.arrival_to.type);
                }

                TryGetValue(item, "departure", segment.departure);
                TryGetValue(item, "arrival", segment.arrival);
                TryGetValue(item, "has_transfers", segment.has_transfers);

                if (item.contains("details") && item["details"].is_array()) {
                    for (const auto& detail_item : item["details"]) {
                        ScheduleResponse::Detail detail;

                        TryGetValue(detail_item, "is_transfer", detail.is_transfer);
                        TryGetValue(detail_item, "duration", detail.duration);
                        TryGetValue(detail_item, "departure", detail.departure);
                        TryGetValue(detail_item, "arrival", detail.arrival);

                        if (detail_item.contains("thread") && detail_item["thread"].is_object()) {
                            const auto& thread = detail_item["thread"];
                            TryGetValue(thread, "title", detail.thread.title);
                            TryGetValue(thread, "transport_type", detail.thread.transport_type);
                        }

                        if (detail_item.contains("from") && detail_item["from"].is_object()) {
                            auto& from = detail_item["from"];

                            TryGetValue(from, "type", detail.from.type);
                            TryGetValue(from, "title", detail.from.title);
                            TryGetValue(from, "code", detail.from.code);
                        }

                        if (detail_item.contains("to") && detail_item["to"].is_object()) {
                            auto& to = detail_item["to"];
                            TryGetValue(to, "type", detail.to.type);
                            TryGetValue(to, "title", detail.to.title);
                            TryGetValue(to, "code", detail.to.code);
                        }

                        if (detail_item.contains("transfer_point") && detail_item["transfer_point"].is_object()) {
                            auto& point = detail_item["transfer_point"];

                            TryGetValue(point, "type", detail.transfer_point.type);
                            TryGetValue(point, "title", detail.transfer_point.title);
                            TryGetValue(point, "code", detail.transfer_point.code);
                        }

                        if (detail_item.contains("transfer_from") && detail_item["transfer_from"].is_object()) {
                            auto& from = detail_item["transfer_from"];

                            TryGetValue(from, "type", detail.transfer_from.type);
                            TryGetValue(from, "title", detail.transfer_from.title);
                            TryGetValue(from, "code", detail.transfer_from.code);
                            TryGetValue(from, "transport_type", detail.transfer_from.transport_type);
                        }

                        if (detail_item.contains("transfer_to") && detail_item["transfer_to"].is_object()) {
                            auto& to = detail_item["transfer_to"];

                            TryGetValue(to, "type", detail.transfer_to.type);
                            TryGetValue(to, "title", detail.transfer_to.title);
                            TryGetValue(to, "code", detail.transfer_to.code);
                            TryGetValue(to, "transport_type", detail.transfer_to.transport_type);
                        }

                        segment.details.push_back(detail);
                    }
                }
                else {
                    continue;
                }
                result.segments.push_back(segment);
            }
        }

        if (json_data.contains("search") && json_data["search"].is_object()) {
            auto& search = json_data["search"];
            if (search.contains("date") && search["date"].is_string()) result.search.date = search["date"].get<std::string>();

            if (search.contains("from") && search["from"].is_object()) {
                auto& from = search["from"];

                TryGetValue(from, "code", result.search.from.code);
                TryGetValue(from, "type", result.search.from.type);
                TryGetValue(from, "title", result.search.from.title);
            }

            if (search.contains("to") && search["to"].is_object()) {
                auto& to = search["to"];

                TryGetValue(to, "code", result.search.to.code);
                TryGetValue(to, "type", result.search.to.type);
                TryGetValue(to, "title", result.search.to.title);
            }
        }
        cache.insert(map_key, result);
        return result;
    }
} // namespace api

