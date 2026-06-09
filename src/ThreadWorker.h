#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace ImOsm {
  class MapPlot;
}

namespace api {
  class Lement;
}
struct ImVec2;
class WorkerThread {
  public:
  explicit WorkerThread(const std::shared_ptr<ImOsm::MapPlot>& mapPlotPtr);
  ~WorkerThread();

  WorkerThread(const WorkerThread&) = delete;
  WorkerThread& operator=(const WorkerThread&) = delete;

  WorkerThread(WorkerThread&&) = delete;
  WorkerThread& operator=(WorkerThread&&) = delete;

  void execute(std::function<void()> task);

  void CheckNearCity(const ImVec2&);
  void UpdateNearCity(const api::Lement*);

  private:
  void threadFunction(std::shared_ptr<ImOsm::MapPlot> mapPlot);

  std::thread worker;
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<std::function<void()>> tasks;
  bool stop = false;
  bool hasWork = false;
};