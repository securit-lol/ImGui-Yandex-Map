#include "ImOsmDemoApp.h"

#include <imgui_internal.h>
// #include "ImOsmMarkPanel.h"
#include "YandexApi/YandexData.h"

using namespace ImOsm;
using namespace ImOsm::Rich;

ImOsmDemoApp::ImOsmDemoApp()
    : ImApp::MainWindow("ImGui Yandex Map")
    , _mapPlot { std::make_shared<ImOsm::MapPlot>() }
{
  worker = std::make_unique<ThreadWorker>(_mapPlot);
}

ImOsmDemoApp::~ImOsmDemoApp() { }

void ImOsmDemoApp::beforeLoop() { }

void ImOsmDemoApp::firstPaint()
{
  const ImGuiID centralNode {
    ImGui::DockBuilderGetCentralNode(dockSpaceID())->ID
  };

  ImGuiID leftNode, rightNode;
  ImGui::DockBuilderSplitNode(centralNode, ImGuiDir_Left, 0.3f, &leftNode,
      &rightNode);

  ImGuiID topLeftNode, bottomLeftNode;

  ImGui::DockBuilderSplitNode(leftNode, ImGuiDir_Up, 0.5f, &topLeftNode,
      &bottomLeftNode);

  ImGui::DockBuilderDockWindow("WayInfo", topLeftNode);
  ImGui::DockBuilderDockWindow("Data", bottomLeftNode);
  ImGui::DockBuilderDockWindow("MapWidget", rightNode);
}

#include <limits>

static std::vector<const ya_data::Lement*> Search(ThreadWorker& api, const std::string& prefix)
{
  std::vector<const ya_data::Lement*> result;

  if (prefix.empty()) {
    return result;
  }

  for (const auto& __country : api.all_country_data) {
    for (const auto& __region : __country.regions) {
      for (const auto& __lement : __region.settlements) {
        if (__lement.title.length() < prefix.length()) {
          continue;
        }

        bool matches = true;

        for (size_t i = 0; i < prefix.length(); ++i) {
          if (__lement.title[i] != prefix[i]) {
            matches = false;
            break;
          }
        }

        if (matches) {
          result.push_back(&__lement);
        }
      }
    }
  }

  return result;
}

void ImOsmDemoApp::paint()
{
  ImGui::Begin("MapWidget", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
  ImGui::Text("MOUSE: lat %.2f, lon %.2f", _mapPlot->mouseLon(),
      _mapPlot->mouseLat());
  // ImGui::Text("VIEW: lat %.2f:%.2f, lon %.2f:%.2f ", _mapPlot->minLat(),
  // _mapPlot->maxLat(), _mapPlot->minLon(), _mapPlot->maxLon());
  ImGui::Text("ZOOM: %.2f", _mapPlot->current_zoom());
  _mapPlot->paint();

  if (ImGui::IsMouseReleased(0) && _mapPlot->mouseOnPlot() && _mapPlot->checkReady()) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDragMaxDistanceSqr[0] < 100.0f) {
      worker->CheckNearCity(ImVec2(_mapPlot->mouseLat(), _mapPlot->mouseLon()));
    }
  }

  ImGui::End();

  ImGui::Begin("WayInfo", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  if (worker->near_city_1.get())
    ImGui::Text(("Point 1: " + worker->near_city_1->title + " (code: " + worker->near_city_1->yandex_code + ")").c_str());

  if (worker->near_city_2.get())
    ImGui::Text(("Point 2: " + worker->near_city_2->title + " (code: " + worker->near_city_2->yandex_code + ")").c_str());

  ImGui::Separator();

  if (worker->near_city_1.get() && worker->near_city_2.get() && !worker->way_data.search.from.title.empty() && !worker->way_data.search.to.title.empty()) {
    std::lock_guard<std::mutex> lock(worker->way_mtx);
    ImGui::Text("Откуда: %s (%s)",
        worker->way_data.search.from.title.c_str(),
        worker->way_data.search.from.code.c_str());
    ImGui::Text("Куда: %s (%s)", worker->way_data.search.to.title.c_str(),
        worker->way_data.search.to.code.c_str());
    ImGui::Text("Дата: %s", worker->way_data.search.date.c_str());
    ImGui::Text("Кол-во путей: %d", (int)worker->way_data.segments.size());

    if (!worker->way_data.segments.empty()) {
      int way_idx = 0;
      for (const auto& segment : worker->way_data.segments) {
        if (segment.details.empty())
          continue;

        ImGui::Text("\n--- Путь %d ---", ++way_idx);
        ImGui::Text("Откуда: %s", segment.departure_from.title.c_str());
        ImGui::Text("Куда: %s", segment.arrival_to.title.c_str());
        ImGui::Text("Отправление: %s", segment.departure.c_str());
        ImGui::Text("Прибытие: %s", segment.arrival.c_str());

        ImGui::Text("Детали пути:");
        ImGui::Indent();

        for (const auto& detail : segment.details) {
          std::string way_detail;
          if (detail.is_transfer) {
            way_detail = detail.transfer_from.title + " -> " + detail.transfer_to.title;
          } else {
            way_detail = detail.from.title + " -> " + detail.to.title;
          }
          way_detail += " (";
          if (!detail.is_transfer) {
            std::string dep_time = detail.departure;
            std::string arr_time = detail.arrival;
            if (dep_time.length() > 16)
              dep_time = dep_time.substr(11, 5);
            if (arr_time.length() > 16)
              arr_time = arr_time.substr(11, 5);

            way_detail += dep_time + " -> " + arr_time + ", ";
          }

          int det_hours = static_cast<int>(detail.duration / 3600);
          int det_minutes = static_cast<int>(std::fmod(detail.duration, 3600.0) / 60);
          way_detail += std::to_string(det_hours) + "ч " + std::to_string(det_minutes) + "м)";

          if (detail.is_transfer)
            way_detail += " | transfer";
          else
            way_detail += " | " + detail.thread.transport_type;

          ImGui::Text("%s", way_detail.c_str());
        }

        ImGui::Unindent();

        ImGui::Separator();
      }
    }
  }
  ImGui::End();
  static char search_buffer[128] = "";
  static std::vector<const ya_data::Lement*> search_results;

  ImGui::Begin("Data", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  if (ImGui::InputText("Поиск", search_buffer, IM_ARRAYSIZE(search_buffer))) {
    search_results.clear();
    if (strlen(search_buffer) > 0) {
      search_results = Search(*worker, search_buffer);
    }
  }

  if (!search_results.empty()) {
    ImGui::Text("Результаты поиска: ");
    int lem_indx = 0;
    for (const auto& __lement : search_results) {
      if (ImGui::TreeNode((__lement->title + "##l" + std::to_string(++lem_indx)).c_str())) {
        if (ImGui::Button(("Выбрать эту точку##z" + std::to_string(++lem_indx)).c_str())) {
          worker->UpdateNearCity(__lement);
        }
        for (const auto& __station : __lement->stations) {
          ImGui::BulletText("Название: %s", __station.title.c_str());
          ImGui::Text("Тип: %s", __station.station_type.c_str());
          ImGui::Text("Транспорт: %s", __station.transport_type.c_str());
          ImGui::Text("Яндекс код: %s", __station.yandex_code.c_str());
          ImGui::Text("Координаты: %.6f, %.6f", __station.latitude,
              __station.longitude);
          ImGui::Separator();
        }

        ImGui::TreePop();
      }
    }
  }

  ImGui::Separator();
  ImGui::Text("Все данные:");

  for (const auto& __country : worker->all_country_data) {
    if (ImGui::CollapsingHeader(__country.title.c_str())) {
      int reg_indx = 0;
      for (const auto& __region : __country.regions) {
        if (ImGui::TreeNode(
                (__region.title + "##r" + std::to_string(++reg_indx))
                    .c_str())) {
          int lem_indx = 0;
          for (const auto& __lement : __region.settlements) {
            if (ImGui::TreeNode(
                    (__lement.title + "##l" + std::to_string(++lem_indx))
                        .c_str())) {
              if (ImGui::Button(
                      ("Выбрать эту точку##z" + std::to_string(++lem_indx))
                          .c_str())) {
                worker->UpdateNearCity(&__lement);
              }
              for (const auto& __station : __lement.stations) {
                ImGui::BulletText("Название: %s", __station.title.c_str());
                ImGui::Text("Тип: %s", __station.station_type.c_str());
                ImGui::Text("Транспорт: %s", __station.transport_type.c_str());
                ImGui::Text("Яндекс код: %s", __station.yandex_code.c_str());
                ImGui::Text("Координаты: %.6f, %.6f", __station.latitude,
                    __station.longitude);
                ImGui::Separator();
              }

              ImGui::TreePop();
            }
          }

          ImGui::TreePop();
        }
      }
    }
  }

  ImGui::End();
}