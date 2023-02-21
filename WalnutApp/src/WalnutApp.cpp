#include "Walnut/Application.h"
#include "Walnut/Image.h"

#include "MetarManager.h"

#include <fstream>

#include "ParseMetar.h"

class ExampleLayer : public Walnut::Layer
{
  public:
    int getAirportIndex(const char *icao)
    {
        auto it = std::find_if(airacData.begin(), airacData.end(),
                               [&](const quicktype::Airport &airport) { return airport.icao == icao; });

        return it - airacData.begin();
    }

    virtual void OnAttach() override
    {
        if (!std::filesystem::exists("./airac.json"))
            throw std::runtime_error("No airac data found");

        std::ifstream airacFile("./airac.json");
        airacData = nlohmann::json::parse(airacFile);

        metarManager.addAirports(airacData);

        autoGenBuffer = new char[102];
        lfbdIndex = getAirportIndex("LFBD");
        lfbeIndex = getAirportIndex("LFBE");
        lfbhIndex = getAirportIndex("LFBH");
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
            ImGui::Text("Procedures:");
            ImGui::Text("SID RNAV: %s\nSID CONV: %s",
                        airport.runways[airport.selectedRunwayIndex].sidrnav.value_or("N/A").c_str(),
                        airport.runways[airport.selectedRunwayIndex].sidconv.value_or("N/A").c_str());
            ImGui::Text("\nSTAR RNAV: %s\nSTAR CONV: %s", airport.runways[airport.selectedRunwayIndex].starrnav.c_str(),
                        airport.runways[airport.selectedRunwayIndex].starconv.value_or("N/A").c_str());
            ImGui::End();
        }

        ImGui::Begin("AutoGen");
        genAtis();
        ImGui::TextUnformatted(autoGenBuffer);
        if (ImGui::Button("Copy"))
            ImGui::SetClipboardText(autoGenBuffer);
        ImGui::End();
    }

    virtual void OnDetach() override
    {
        delete[] autoGenBuffer;
    }

    void genAtis()
    {
        if (airacData[lfbdIndex].selectedRunwayIndex == -1 || airacData[lfbeIndex].selectedRunwayIndex == -1 ||
            airacData[lfbhIndex].selectedRunwayIndex == -1)
        {
            std::strcpy(autoGenBuffer, "N/A");
            return;
        }

        const quicktype::Runway &lfbdRunway = airacData[lfbdIndex].runways[airacData[lfbdIndex].selectedRunwayIndex];
        const quicktype::Runway &lfbeRunway = airacData[lfbeIndex].runways[airacData[lfbeIndex].selectedRunwayIndex];
        const quicktype::Runway &lfbhRunway = airacData[lfbhIndex].runways[airacData[lfbhIndex].selectedRunwayIndex];

        std::snprintf(
            autoGenBuffer, 102,
            "LFBD DEP-%s ARR-%s EXPECT %s%s // LFBE DEP-%s ARR-%s EXPECT %s%s // LFBH DEP-%s ARR-%s EXPECT %s%s",
            lfbdRunway.sidrnav.value_or(lfbdRunway.sidconv.value()).c_str(), lfbdRunway.starrnav.c_str(),
            lfbdRunway.app.c_str(), lfbdRunway.name.c_str(), //
            lfbeRunway.sidrnav->c_str(), lfbeRunway.starrnav.c_str(), lfbeRunway.app.c_str(), lfbeRunway.name.c_str(),
            lfbhRunway.sidconv->c_str(), lfbhRunway.starrnav.c_str(), lfbhRunway.app.c_str(), lfbhRunway.name.c_str());
    }

  private:
    MetarManager metarManager;
    quicktype::Airac airacData;
    int lfbdIndex, lfbeIndex, lfbhIndex;
    char *autoGenBuffer;
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
