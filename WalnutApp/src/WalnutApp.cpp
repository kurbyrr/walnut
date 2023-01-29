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
            ImGui::Text("METAR: %s\nRunway in use:", metarManager.readyMetars[airport.icao].second.c_str());
            parseMetar(airport, metarManager.readyMetars[airport.icao].second);
            for (int i = 0; i < airport.runways.size(); i++)
            {
                if (ImGui::RadioButton(airport.runways[i].name.c_str(), airport.selectedRunwayIndex == i))
                    airport.selectedRunwayIndex = i;
            }
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
