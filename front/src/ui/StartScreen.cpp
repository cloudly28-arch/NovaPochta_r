#include "ui/StartScreen.h"

#include <imgui.h>
#include <algorithm>

bool StartScreen::draw(SimulationSettings& settings) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 size(std::min(680.0f, viewport->WorkSize.x - 24.0f),
                    std::min(690.0f, viewport->WorkSize.y - 24.0f));
  ImGui::SetNextWindowPos(
      ImVec2(viewport->WorkPos.x + (viewport->WorkSize.x - size.x) * 0.5f,
             viewport->WorkPos.y + (viewport->WorkSize.y - size.y) * 0.5f),
      ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  ImGui::Begin("Simulation Settings", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoMove);
  ImGui::BeginChild("SettingsBody", ImVec2(0.0f, -66.0f));
  ImGui::TextUnformatted("NOVA WAREHOUSE");
  ImGui::Separator();
  ImGui::SliderInt("Дней моделирования", &settings.days, 10, 100);
  ImGui::SliderInt("Магазинов", &settings.stores, 3, 9);
  ImGui::SliderInt("Видов товаров", &settings.products, 12, 20);
  ImGui::TextWrapped(
      "Ассортимент каждого магазина задан заранее и сохраняется.");
  ImGui::Spacing();
  ImGui::Separator();

  ExperimentSettings& model = settings.experiment;
  bool automaticFill = model.initialFillPercent < 0;
  if (ImGui::Checkbox("Начальный запас по спросу", &automaticFill)) {
    model.initialFillPercent = automaticFill ? -1 : 50;
  }
  if (!automaticFill) {
    ImGui::SliderInt("Заполненность, %", &model.initialFillPercent, 0, 100,
                     "%d", ImGuiSliderFlags_AlwaysClamp);
  }
  ImGui::TextWrapped(
      "Ручная заполненность — доля вместимости каждого товара, "
      "округлённая вниз до целой упаковки. Меняет только старт.");
  ImGui::SliderInt("Интенсивность спроса, %", &model.demandPercent, 50, 150,
                   "%d", ImGuiSliderFlags_AlwaysClamp);
  ImGui::TextWrapped(
      "100%% — обычные продажи магазинов; 150%% — в среднем в 1,5 раза "
      "больше, если товар есть в наличии.");
  bool automaticCover = model.stockCoverDays == 0;
  if (ImGui::Checkbox("Запас в днях по сроку годности", &automaticCover)) {
    model.stockCoverDays = automaticCover ? 0 : 5;
  }
  if (!automaticCover) {
    ImGui::SliderInt("Целевой запас, дней", &model.stockCoverDays, 1, 14, "%d",
                     ImGuiSliderFlags_AlwaysClamp);
  }
  ImGui::TextWrapped(
      "Влияет на начальный автозапас и пополнение весь эксперимент. "
      "Ограничен вместимостью и сроком годности товара; заявки магазинов также учитываются.");
  ImGui::SliderInt("Закупочная цена, %", &model.purchaseCostPercent, 1, 100,
                   "%d", ImGuiSliderFlags_AlwaysClamp);
  ImGui::TextWrapped(
      "Процент обычной цены продажи. Например, 70%% даёт 30%% "
      "валовой маржи до уценки, списаний и других расходов.");
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextUnformatted("Подтверждение поставок");
  if (ImGui::RadioButton("Ручное", !settings.automaticSupplierApproval)) {
    settings.automaticSupplierApproval = false;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Автоматическое",
                         settings.automaticSupplierApproval)) {
    settings.automaticSupplierApproval = true;
  }
  ImGui::TextWrapped(
      "При ручном режиме заявки поставщикам нужно подтверждать. "
      "Транспорт, зарплаты и хранение в финансовый результат не входят.");
  ImGui::EndChild();

  ImGui::Separator();
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 240.0f) * 0.5f);
  const bool startPressed =
      ImGui::Button("START SIMULATION", ImVec2(240.0f, 46.0f));
  ImGui::End();
  return startPressed;
}
