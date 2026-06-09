#pragma once

#include "YandexApi/YandexData.h"
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace ImOsm {
  class MapPlot;
}

struct ImVec2;

namespace ya_data {
  class Lement;
  struct City;
  struct Country;
  struct ScheduleResponse;
} // namespace ya_data

class ThreadWorker {
  public:
  explicit ThreadWorker(const std::shared_ptr<ImOsm::MapPlot>& mapPlotPtr);
  ~ThreadWorker();

  ThreadWorker(const ThreadWorker&) = delete;
  ThreadWorker& operator=(const ThreadWorker&) = delete;
  ThreadWorker(ThreadWorker&&) = delete;
  ThreadWorker& operator=(ThreadWorker&&) = delete;

  void GetAllStations();
  std::unique_ptr<const ya_data::City> GetNearestCity(const ya_data::Lement& lement, float distance);
  ya_data::ScheduleResponse GetWay(const std::string& fromCity, const std::string& toCity, const std::string& date);

  std::vector<ya_data::Country> all_country_data = { };
  ya_data::ScheduleResponse way_data = { };
  std::mutex way_mtx = { };
  std::mutex city_mtx = { };
  std::unique_ptr<const ya_data::City> near_city_1 = nullptr;
  std::unique_ptr<const ya_data::City> near_city_2 = nullptr;

  void CheckNearCity(const ImVec2&);
  void UpdateNearCity(const ya_data::Lement*);
  void execute(std::function<void()> task);

  private:
  bool switcher = true;
  void threadFunction(std::shared_ptr<ImOsm::MapPlot> mapPlot);
  std::string GetCurrentDate();

  std::thread worker;
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<std::function<void()>> tasks;
  bool stop = false;
  bool hasWork = false;

  cpr::Session session;
  DeadthMap<std::string, ya_data::ScheduleResponse> cache;
};