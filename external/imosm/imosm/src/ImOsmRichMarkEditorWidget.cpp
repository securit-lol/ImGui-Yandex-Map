#include "ImOsmRichMarkEditorWidget.h"
#include "ImOsmRichMapPlot.h"
#include "ImOsmRichMarkItem.h"
#include "ImOsmRichMarkItemWidget.h"
#include <misc/cpp/imgui_stdlib.h>
#include <iostream>
namespace ImOsm {
namespace Rich {
const float kInf = 1e9;
struct MarkEditorWidget::Ui {
  inline static const char latlonFormat[]{"%.6f"};
  std::array<float, 2> latLonInput{0.f, 0.f};
  std::string markNameInputText{};
  bool isMousePick{false};
  bool isMarkAdd{false};
};

MarkEditorWidget::MarkEditorWidget(std::shared_ptr<RichMapPlot> plot,
                                   std::shared_ptr<MarkStorage> storage)
    : _plot{plot}, _storage{storage}, _ui{std::make_unique<Ui>()} {
  _ui->markNameInputText.reserve(32);
}

MarkEditorWidget::~MarkEditorWidget() = default;

void MarkEditorWidget::AddMarkCustom(std::array<float, 2> latLonInput, std::string markNameInputText, const void* ptr) {
  _storage->addMark(latLonInput, markNameInputText, ptr);
  _plot->addItem(_storage->_markItems.back().ptr);
}

inline float ImLength(const ImVec2& a, const ImVec2& b) {
    return hypotf(a.x - b.x, a.y - b.y);
}

const void* MarkEditorWidget::paint() {
   //ImGui::TextUnformatted("Mark Editor");
  // paint_latLonInput();
  // ImGui::SameLine();
  // paint_mousePickBtn();
  // paint_markNameInput();
  // ImGui::SameLine();
  // paint_addMarkBtn();
  // paint_markTable();


  
    _ui->latLonInput = {_plot->mouseLat(), _plot->mouseLon()};
    
    float min_dist = kInf;
    const void* min_dist_circle_name = nullptr;
    for(auto st : _storage->markItems()) {
      float dist = ImLength({_plot->mouseLat(), _plot->mouseLon()}, {st.ptr.get()->geoCoords().lat, st.ptr.get()->geoCoords().lon});
      
      if (dist < min_dist) {
        min_dist = dist;
        min_dist_circle_name = st.ptr.get()->_ptr;
      }
    }
    //std::cout << min_dist_circle_name << " " <<  std::endl;
  if (min_dist < 0.5)
    return min_dist_circle_name;
  else 
    return nullptr;
  // if (_ui->isMarkAdd) {
  //   _ui->isMarkAdd = false;
  //   _storage->addMark(_ui->latLonInput, _ui->markNameInputText);
  //   _plot->addItem(_storage->_markItems.back().ptr);
  // }

  // if (_storage->handleLoadState()) {
  //   const auto &markItems{_storage->markItems()};
  //   std::for_each(markItems.begin(), markItems.end(),
  //                 [this](auto &item) { _plot->addItem(item.ptr); });
  // }

  // if (_storage->handlePickState()) {
  //   _ui->latLonInput = _storage->_pickCoords;
  // }
}

void MarkEditorWidget::paint_latLonInput() {
  ImGui::PushItemWidth(200 + ImGui::GetStyle().ItemSpacing.x / 2);
  ImGui::InputFloat2("Lat/Lon", _ui->latLonInput.data(), _ui->latlonFormat);
  ImGui::PopItemWidth();
}

void MarkEditorWidget::paint_mousePickBtn() {
  if (!_ui->isMousePick) {
    if (ImGui::Button("Mouse Pick")) {
      _ui->isMousePick = !_ui->isMousePick;
    }
  } else {
    const auto color{ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered)};
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    if (ImGui::Button("Mouse Pick")) {
      _ui->isMousePick = !_ui->isMousePick;
    }
    ImGui::PopStyleColor();
  }
}

void MarkEditorWidget::paint_markNameInput() {
  ImGui::PushItemWidth(100);
  ImGui::InputText("Mark Name", &_ui->markNameInputText);
  ImGui::PopItemWidth();
}

void MarkEditorWidget::paint_addMarkBtn() {
  if (ImGui::Button("Add Mark")) {
    _ui->isMarkAdd = true;
  }
}

void MarkEditorWidget::paint_markTable() {
  static const int tableCols{5};
  static const ImGuiTableColumnFlags colFlags{ImGuiTableColumnFlags_WidthFixed};

  if (ImGui::BeginTable("MarkTabe", tableCols)) {
    ImGui::TableSetupColumn("Name", colFlags, 200);
    ImGui::TableSetupColumn("Lat", colFlags, 100);
    ImGui::TableSetupColumn("Lon", colFlags, 100);
    ImGui::TableSetupColumn("Setup", colFlags, 50);
    ImGui::TableSetupColumn("Delete", colFlags, 50);
    ImGui::TableHeadersRow();

    _storage->rmMarks();

    const auto &markItems{_storage->markItems()};
    std::for_each(markItems.begin(), markItems.end(), [this](auto &item) {
      ImGui::TableNextRow();
      ImGui::PushID(item.ptr.get());
      paint_markTableRow(item);
      ImGui::PopID();
    });

    ImGui::EndTable();
  }
}

void MarkEditorWidget::paint_markTableRow(const MarkStorage::ItemNode &item) {
  // Name
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(item.ptr->text().c_str());

  // Lat
  ImGui::TableNextColumn();
  ImGui::Text(_ui->latlonFormat, item.ptr->geoCoords().lat);

  // Lon
  ImGui::TableNextColumn();
  ImGui::Text(_ui->latlonFormat, item.ptr->geoCoords().lon);

  // Setup
  ImGui::TableNextColumn();
  if (ImGui::Button("Setup")) {
    ImGui::OpenPopup("Setup Item");
    _itemWidget =
        std::make_unique<MarkItemWidget>(item.ptr, GeoCoords{_ui->latLonInput});
  }

  if (ImGui::BeginPopupModal("Setup Item")) {
    // Draw popup contents.
    _itemWidget->paint();
    ImGui::Separator();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
      _itemWidget.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
      _itemWidget->apply();
      ImGui::CloseCurrentPopup();
      _itemWidget.reset();
    }
    ImGui::EndPopup();
  }

  // Delete
  ImGui::TableNextColumn();
  if (ImGui::Button("Delete")) {
    item.rmFlag = true;
  }
}
} // namespace Rich
} // namespace ImOsm
