#include "ThreadWorker.h"
#include "ImOsmDemoApp.h"
#include "YandexApi/YandexData.h"
#include <imgui_internal.h>

WorkerThread::WorkerThread(const std::shared_ptr<ImOsm::MapPlot>& mapPlotPtr)
{
  worker = std::thread(&WorkerThread::threadFunction, this, mapPlotPtr);
}

WorkerThread::~WorkerThread()
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

void WorkerThread::execute(std::function<void()> task)
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(std::move(task));
    hasWork = true;
  }
  cv.notify_one();
}

inline float ImLength(const ImVec2& a, const ImVec2& b)
{
  return hypotf(a.x - b.x, a.y - b.y);
}

const api::Lement* getNearLement(const ImVec2& mouse_pos)
{
  float min_dist = std::numeric_limits<float>::infinity();
  const api::Lement* _this = nullptr;
  for (const auto& country : api::data::all_country_data) {
    for (const auto& region : country.regions) {
      for (const auto& lement : region.settlements) {
        float dist = ImLength(ImVec2(mouse_pos.x, mouse_pos.y), { lement.latitude, lement.longitude });
        if (dist < min_dist) {
          min_dist = dist;
          _this = &lement;
        }
      }
    }
  }
  // std::cout << _this->title << std::endl;
  return _this;
}

void UpdateWay()
{
  if (api::data::near_city_1 && api::data::near_city_2 && api::data::near_city_1->yandex_code[0] == 'c' && api::data::near_city_2->yandex_code[0] == 'c' && api::data::near_city_1->yandex_code != api::data::near_city_2->yandex_code) {
    std::lock_guard<std::mutex> lock(api::data::way_mtx);
    api::data::way_data = api::GetWay(api::data::near_city_1->yandex_code, api::data::near_city_2->yandex_code, api::GetCurrentDate());
  }
}

void UpdateCity(const api::Lement* nearest_point)
{
  std::lock_guard<std::mutex> lock(api::data::city_mtx);
  std::unique_ptr<const api::City> nearest_city = std::move(api::GetNearestCity(*nearest_point, 50));
  if (nearest_city.get()) {
    if (api::data::switcher) {
      api::data::near_city_1 = std::move(nearest_city);
    } else {
      api::data::near_city_2 = std::move(nearest_city);
    }
    api::data::switcher = !api::data::switcher;
  }
  UpdateWay();
}

void CityNamer(const ImVec2& mouse_pos)
{
  const api::Lement* nearest_point = getNearLement(mouse_pos);
  UpdateCity(nearest_point);
}

void WorkerThread::CheckNearCity(const ImVec2& mouse_pos)
{
  execute([=]() {
    CityNamer(mouse_pos);
  });
}

void WorkerThread::UpdateNearCity(const api::Lement* nearest_point)
{
  execute([=]() {
    UpdateCity(nearest_point);
  });
}

void WorkerThread::threadFunction(std::shared_ptr<ImOsm::MapPlot> mapPlot)
{
  if (kApiKey.size() == 0) {
    std::cout << "Wrong api key. Edit (kApiKey) in YandexApi->YandexData.h" << std::endl;
    return;
  }
  api::SessionInit();
  api::GetAllStations();
  for (const auto& country : api::data::all_country_data) {
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