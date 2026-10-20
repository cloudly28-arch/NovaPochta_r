#include "ui/StartScreen.h"

#include <imgui.h>
#include <algorithm>

namespace {
const ImVec4 kAccent(0.25f, 0.84f, 0.76f, 1.0f);
const ImVec4 kMuted(0.53f, 0.61f, 0.71f, 1.0f);

void hint(const char* text) {
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void slider(const char* label, const char* id, int& value, int low, int high,
            const char* format, const char* description) {
  ImGui::TextUnformatted(label);
  hint(description);
  ImGui::SetNextItemWidth(-1.0f);
  ImGui::SliderInt(id, &value, low, high, format, ImGuiSliderFlags_AlwaysClamp);
  ImGui::Spacing();
}

void section(const char* number, const char* title) {
  ImGui::TextColored(kAccent, "%s", number);
  ImGui::SameLine();
  ImGui::TextUnformatted(title);
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

void warehouseSketch(float width) {
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImDrawList* draw = ImGui::GetWindowDrawList();
  const float y = origin.y + 45.0f;
  const float x = origin.x + width * 0.5f;
  const ImU32 line = IM_COL32(61, 106, 123, 170);
  const ImU32 accent = IM_COL32(64, 214, 194, 220);
  draw->AddLine(ImVec2(origin.x + 12, y + 30),
                ImVec2(origin.x + width - 12, y + 30), line);
  draw->AddRectFilled(ImVec2(x - 55, y - 18), ImVec2(x + 55, y + 30),
                      IM_COL32(24, 46, 60, 255), 4);
  draw->AddTriangleFilled(ImVec2(x - 65, y - 18), ImVec2(x, y - 43),
                          ImVec2(x + 65, y - 18), IM_COL32(35, 66, 82, 255));
  draw->AddRect(ImVec2(x - 55, y - 18), ImVec2(x + 55, y + 30), line, 4);
  for (int i = 0; i < 3; ++i) {
    const float left = x - 39 + i * 28;
    draw->AddRectFilled(ImVec2(left, y), ImVec2(left + 21, y + 30),
                        IM_COL32(12, 25, 36, 255), 2);
    draw->AddLine(ImVec2(left, y), ImVec2(left + 21, y), accent, 2);
  }
  for (int side : {-1, 1}) {
    const float node = x + side * (width * 0.38f);
    draw->AddLine(ImVec2(node, y + 12), ImVec2(x + side * 62, y + 12), line);
    draw->AddCircleFilled(ImVec2(node, y + 12), 5, accent);
    draw->AddCircle(ImVec2(node, y + 12), 10, line);
  }
  ImGui::Dummy(ImVec2(width, 84));
}
}

bool StartScreen::draw(SimulationSettings& settings) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImDrawList* background = ImGui::GetBackgroundDrawList();
  const ImVec2 pos = viewport->WorkPos;
  const ImVec2 extent = viewport->WorkSize;
  background->AddRectFilled(pos, ImVec2(pos.x + extent.x, pos.y + extent.y),
                            IM_COL32(9, 15, 25, 255));
  for (float x = pos.x; x < pos.x + extent.x; x += 40.0f) {
    background->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + extent.y),
                        IM_COL32(30, 44, 62, 65));
  }
  for (float y = pos.y; y < pos.y + extent.y; y += 40.0f) {
    background->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + extent.x, y),
                        IM_COL32(30, 44, 62, 65));
  }
  const ImVec2 size(std::min(1080.0f, extent.x - 32.0f),
                    std::min(650.0f, extent.y - 32.0f));
  ImGui::SetNextWindowPos(ImVec2(pos.x + (extent.x - size.x) * 0.5f,
                                 pos.y + (extent.y - size.y) * 0.5f),
                          ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 22));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 7));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.08f, 0.12f, 1));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.115f, 0.16f, 1));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.94f, 0.98f, 1));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.24f, 0.31f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.17f, 0.23f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.23f, 0.3f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.28f, 0.34f, 1));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, kAccent);
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                        ImVec4(0.4f, 0.95f, 0.85f, 1));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, kAccent);
  ImGui::Begin("Настройка сценария###SimulationSettings", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);

  ImGui::TextColored(kAccent, "NOVA / WAREHOUSE");
  ImGui::SetWindowFontScale(1.55f);
  ImGui::TextUnformatted("Новый эксперимент");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::TextColored(kMuted, "Склад. Спрос. Поставки. Финансовый результат.");
  ImGui::Spacing();

  ImGui::BeginChild("ScenarioBody", ImVec2(0, -78), ImGuiChildFlags_None);
  const bool wide = ImGui::GetContentRegionAvail().x >= 760.0f;
  if (ImGui::BeginTable("ScenarioColumns", wide ? 2 : 1,
                        ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    ImGui::BeginChild(
        "ScenarioCard", ImVec2(0, wide ? 0 : 410),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    section("01", "Сценарий");
    slider("Продолжительность", "##days", settings.days, 10, 100, "%d дней",
           "Количество дней моделирования.");
    slider("Магазины", "##stores", settings.stores, 3, 9, "%d",
           "У каждого магазина свой заданный ассортимент.");
    slider("Виды товаров", "##products", settings.products, 11, 20, "%d",
           "Количество видов товаров, участвующих в эксперименте.");
    ImGui::TextUnformatted("Подтверждение поставок");
    const float half = (ImGui::GetContentRegionAvail().x - 10.0f) * 0.5f;
    for (int automatic = 0; automatic < 2; ++automatic) {
      if (automatic) ImGui::SameLine();
      const bool selected =
          settings.automaticSupplierApproval == (automatic != 0);
      ImGui::PushStyleColor(ImGuiCol_Button,
                            selected ? ImVec4(0.13f, 0.36f, 0.35f, 1)
                                     : ImVec4(0.12f, 0.17f, 0.23f, 1));
      if (ImGui::Button(automatic ? "Автоматически" : "Вручную",
                        ImVec2(half, 34))) {
        settings.automaticSupplierApproval = automatic != 0;
      }
      ImGui::PopStyleColor();
      hint(automatic ? "Программа подтверждает заявки поставщикам."
                     : "Подтверждайте заявки в панели поставщика.");
    }
    if (ImGui::GetContentRegionAvail().y > 88) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                           ImGui::GetContentRegionAvail().y - 86);
      warehouseSketch(ImGui::GetContentRegionAvail().x);
    }
    ImGui::EndChild();
    ImGui::TableNextColumn();
    ImGui::BeginChild(
        "EconomyCard", ImVec2(0, wide ? 0 : 440),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    section("02", "Склад и экономика");
    ExperimentSettings& model = settings.experiment;
    slider(
        "Спрос", "##demand", model.demandPercent, 50, 150, "%d%%",
        "100% — обычные продажи. 150% — в среднем в полтора раза больше, если товар есть.");
    slider("Закупочная цена", "##purchase", model.purchaseCostPercent, 1, 100,
           "%d%% от цены продажи",
           "При 70% товар с обычной ценой 100 руб. закупается за 70 руб.");
    ImGui::Separator();
    bool manualFill = model.initialFillPercent >= 0;
    if (ImGui::Checkbox("Задать заполненность склада", &manualFill)) {
      model.initialFillPercent = manualFill ? 50 : -1;
    }
    hint(
        "Количество товара на старте. Без галочки запас рассчитывается по спросу.");
    if (manualFill) {
      ImGui::SetNextItemWidth(-1);
      ImGui::SliderInt("##fill", &model.initialFillPercent, 0, 100,
                       "%d%% вместимости", ImGuiSliderFlags_AlwaysClamp);
      hint(
          "0% — пусто, 100% — максимум каждого товара. Округление до целых упаковок.");
    } else {
      ImGui::TextColored(kMuted, "Начальный запас: автоматически");
    }
    ImGui::Spacing();
    bool manualCover = model.stockCoverDays > 0;
    if (ImGui::Checkbox("Задать запас в днях", &manualCover)) {
      model.stockCoverDays = manualCover ? 5 : 0;
    }
    hint(
        "На сколько дней продаж склад формирует резерв. Влияет на пополнение.");
    if (manualCover) {
      ImGui::SetNextItemWidth(-1);
      ImGui::SliderInt("##cover", &model.stockCoverDays, 1, 14, "%d дней",
                       ImGuiSliderFlags_AlwaysClamp);
      hint(
          "Резерв ограничен сроком годности и вместимостью; заказы магазинов также учитываются.");
    } else {
      ImGui::TextColored(kMuted, "Резерв: по сроку годности");
    }
    ImGui::EndChild();
    ImGui::EndTable();
  }
  ImGui::EndChild();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextColored(kMuted, "%d дней / %d магазинов / %d товаров",
                     settings.days, settings.stores, settings.products);
  ImGui::TextColored(kMuted, "Параметры можно изучить по подсказкам.");
  ImGui::SameLine();
  const float buttonWidth = std::min(280.0f, ImGui::GetWindowWidth() * 0.4f);
  ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 24 - buttonWidth,
                             ImGui::GetWindowHeight() - 22 - 48));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.78f, 0.67f, 1));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.29f, 0.89f, 0.77f, 1));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.65f, 0.56f, 1));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.03f, 0.11f, 0.12f, 1));
  const bool start =
      ImGui::Button("Запустить симуляцию", ImVec2(buttonWidth, 48));
  ImGui::PopStyleColor(4);
  ImGui::End();
  ImGui::PopStyleColor(10);
  ImGui::PopStyleVar(6);
  return start;
}
