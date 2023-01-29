#include "Walnut/Application.h"
#include "Walnut/Image.h"

#include "MetarManager.h"

#include <fstream>

template <typename R> bool is_ready(std::shared_future<R> const &f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

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
        for (auto &airport : airacData)
        {
            const std::shared_future<std::string> &future = metarManager.metars.at(airport.icao);

            ImGui::Begin(airport.icao.c_str());
            ImGui::Text("METAR: %s\nRunway in use:", is_ready(future) ? future.get().c_str() : "Updating...");
            for (auto &runway : airport.runways)
            {
                if (ImGui::RadioButton(runway.name.c_str(), runwaysInUse[airport.icao] == runway.qfu))
                    runwaysInUse[airport.icao] = runway.qfu;
            }
            ImGui::End();
        }

        // ImGui::Separator();

        ImGui::ShowDemoWindow();
    }

  private:
    MetarManager metarManager;
    quicktype::Airac airacData;
    std::unordered_map<std::string, int> runwaysInUse;
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
