#include "ImOsmMarkRenderer.h"
#include <imosm_rich.h>
#include "YandexData.h"

using namespace ImOsm;
using namespace ImOsm::Rich;

namespace {
std::shared_ptr<MarkStorage> s_storage;
std::unique_ptr<MarkEditorWidget> s_editor;
std::shared_ptr<RichMapPlot> s_plot;
bool choose_idx = 0;
}

void ImOsm::Rich::InitMarkRenderer(std::shared_ptr<RichMapPlot> plot) {
  s_plot = plot;
  s_storage = std::make_shared<MarkStorage>();
  s_editor = std::make_unique<MarkEditorWidget>(s_plot, s_storage);
  choose_idx = 0;
}

void ImOsm::Rich::AddMarkGeo(float lat, float lon, const std::string &title) {
  // if (!s_editor) InitMarkRenderer(s_plot);
  // if (s_editor) {
  //   s_editor->AddMarkCustom({lat, lon}, title);
  // }
}

void ImOsm::Rich::AddMarksFromApi() {
  if (!s_editor) InitMarkRenderer(s_plot);
  api::GetAllStations();
  for (const auto &country : api::data::all_country_data) {
    for (const auto &region : country.regions) {
      for (const auto &lement : region.settlements) {
        if (s_editor)
          s_editor->AddMarkCustom({lement.latitude, lement.longitude},
                                  lement.title, &lement);
      }
    }
  }
}

#include <iostream>

void ImOsm::Rich::SkanMarks() {
  if (!s_editor) InitMarkRenderer(s_plot);
  if (s_plot->mouseOnPlot() && ImGui::IsMouseClicked(0)) {
    const void* ptr = s_editor->paint();
    if (ptr) {
            if (choose_idx) {
                api::data::lement_2 = static_cast<const api::Lement*> (ptr);
            }
            else {
                api::data::lement_1 = static_cast<const api::Lement*>( ptr);
            }
            choose_idx = !choose_idx;
            if (api::data::lement_1)
            std::cout <<  api::data::lement_1->title << std::endl;
            if (api::data::lement_2)
            std::cout <<  api::data::lement_2->title << std::endl;
            api::UpdateStatus();
    }
            
  }
  if (api::data::lement_1 && api::data::lement_2 && api::data::lement_1->title == api::data::near_city_1.owner_title && api::data::lement_1->title == api::data::near_city_1.owner_title) {
            if (api::data::near_city_1.yandex_code[0] == 'c' && api::data::near_city_2.yandex_code[0] == 'c' && api::data::near_city_1.yandex_code != api::data::near_city_2.yandex_code) {
                if (ImGui::Button("Построить маршрут")) {
                    api::AddTask(api::data::near_city_1.yandex_code, api::data::near_city_2.yandex_code, api::GetCurrentDate());
                }
                    //api::data::schedule_response = api::GetWay(api::data::near_city_1, api::data::near_city_2, api::GetCurrentDate());
            }

        }
         if (api::data::lement_1)
            ImGui::Text(("Point 1: " + api::data::lement_1->title + " | Ближайший город: " + api::data::near_city_1.title + " (code: " + api::data::near_city_1.yandex_code + ")").c_str());

        if (api::data::lement_2)
            ImGui::Text(("Point 2: " + api::data::lement_2->title + " | Ближайший город: " + api::data::near_city_2.title + " (code: " + api::data::near_city_2.yandex_code + ")").c_str());


        if (!api::data::schedule_response.search.from.title.empty() && !api::data::schedule_response.search.to.title.empty() &&
            api::data::schedule_response.search.from.code == api::data::near_city_1.yandex_code && api::data::schedule_response.search.to.code == api::data::near_city_2.yandex_code) {
            ImGui::Text("Откуда: %s (%s)", api::data::schedule_response.search.from.title.c_str(), api::data::schedule_response.search.from.code.c_str());
            ImGui::Text("Куда: %s (%s)", api::data::schedule_response.search.to.title.c_str(), api::data::schedule_response.search.to.code.c_str());
            ImGui::Text("Дата: %s", api::data::schedule_response.search.date.c_str());
            ImGui::Text("Кол-во путей: %d", api::data::schedule_response.segments.size());

            if (!api::data::schedule_response.segments.empty()) {
                int way_idx = 0;
                for (const auto& segment : api::data::schedule_response.segments) {
                    if (segment.details.empty()) continue;
                    ImGui::Text("\n--- Путь№ %zu ---", ++way_idx);
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
                        }
                        else {
                            way_detail = detail.from.title + " -> " + detail.to.title;
                        }
                        way_detail += " (";
                        if (!detail.is_transfer) {
                            std::string dep_time = detail.departure;
                            std::string arr_time = detail.arrival;
                            if (dep_time.length() > 16) dep_time = dep_time.substr(11, 5);
                            if (arr_time.length() > 16) arr_time = arr_time.substr(11, 5);

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

    
}

void ImOsm::Rich::ClearMarks() {
  s_editor.reset();
  s_storage.reset();
}
