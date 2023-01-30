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
            parseMetar(airport, metarManager.readyMetars[airport.icao].second);
            ImGui::Text("METAR: %s\nRunway in use: %s", metarManager.readyMetars[airport.icao].second.c_str(),
                        airport.runways[airport.selectedRunwayIndex].name.c_str());
            for (int i = 0; i < airport.runways.size(); i++)
            {
                if (ImGui::RadioButton(airport.runways[i].name.c_str(), airport.selectedRunwayIndex == i))
                {
                    airport.selectedRunwayIndex = i;
                    airport.runwayOverriden = true;
                }
            }
            if (airport.runwayOverriden)
                if (ImGui::Button("Auto Runway"))
                    airport.runwayOverriden = false;
            ImGui::End();
        }
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
    //         if (ImGui::MenuItem("Open Airac"))
    //         {

    //         }
    //         ImGui::EndMenu();
    //     }
    // });
    return app;
}
