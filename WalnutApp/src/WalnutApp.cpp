#include "Walnut/Application.h"
#include "Walnut/Image.h"

#include "MetarManager.h"

#include <fstream>

#include "ParseMetar.h"

class ExampleLayer : public Walnut::Layer
{
  public:
    virtual void OnAttach() override
    {
        if (!std::filesystem::exists("./airac.json"))
            throw std::runtime_error("No airac data found");

        std::ifstream airacFile("./airac.json");
        airacData = nlohmann::json::parse(airacFile);

        // Prepare names for combos
        for (auto &airport : airacData)
        {
            airport.runwayNames.reserve(airport.runways.size());
            for (auto &runway : airport.runways)
            {
                airport.runwayNames.emplace_back(runway.name.c_str());
            }
        }

        metarManager.addAirports(airacData);
    }

    virtual void OnUIRender() override
    {
        metarManager.procFutures();

        for (auto &airport : airacData)
        {
            ImGui::Begin(airport.icao.c_str());
            if (!metarManager.readyMetars[airport.icao].first)
            {
                ImGui::Text("Updating...");
                ImGui::End();
                continue;
            }
            ImGui::Text("METAR: %s\nRunway in use:", metarManager.readyMetars[airport.icao].second.c_str());
            parseMetar(airport, metarManager.readyMetars[airport.icao].second);
            ImGui::Combo("Runway in use", &airport.selectedRunwayIndex, airport.runwayNames.data(),
                         airport.runwayNames.size());
            ImGui::End();
        }

        // ImGui::Separator();

        ImGui::ShowDemoWindow();
    }

  private:
    MetarManager metarManager;
    quicktype::Airac airacData;
};

Walnut::Application *Walnut::CreateApplication(int argc, char **argv)
{
    Walnut::ApplicationSpecification spec;
    spec.Name = "AtisQuitaine";

    Walnut::Application *app = new Walnut::Application(spec);
    app->PushLayer<ExampleLayer>();
    // app->SetMenubarCallback([app]() {
    //     if (ImGui::BeginMenu("File"))
    //     {
    //         if (ImGui::MenuItem("Exit"))
    //         {
    //             app->Close();
    //         }
    //         ImGui::EndMenu();
    //     }
    // });
    return app;
}
