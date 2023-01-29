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
    // Save and load state from disk
    virtual void OnAttach() override
    {
        if (!std::filesystem::exists("./AtisConf.txt"))
            return;

        std::ifstream iFile("./AtisConf.txt");
        quicktype::ConfigData configData = nlohmann::json::parse(iFile);
        metarManager.addAirports(configData.airports);
    }

    virtual void OnDetach() override
    {
        std::ofstream oFile("./AtisConf.txt");
        quicktype::ConfigData configData;
        configData.airports.reserve(metarManager.metars.size());

        for (auto it = metarManager.metars.begin(); it != metarManager.metars.end(); it++)
        {
            configData.airports.push_back(it->first);
        }

        nlohmann::json json = configData;
        oFile << json;
    }

    virtual void OnUIRender() override
    {
        ImGui::Begin("AtisQuitaine");

        for (auto it = metarManager.metars.begin(); it != metarManager.metars.end(); it++)
        {
            ImGui::Text("%s", it->first.c_str());
            ImGui::Text("METAR: %s", is_ready(it->second) ? it->second.get().c_str() : "Updating");
        }

        ImGui::InputText("Add Airport", newAiportBuf.data(), newAiportBuf.size(), ImGuiInputTextFlags_CharsUppercase);
        if (ImGui::SameLine(), ImGui::Button("+"))
        {
            metarManager.updateMetar(newAiportBuf.data());
            newAiportBuf.fill(0);
        }

        if (ImGui::Button("Update All"))
            metarManager.updateMetars();

        ImGui::Separator();

        ImGui::End();

        ImGui::ShowDemoWindow();
    }

  private:
    MetarManager metarManager;
    std::array<char, 5> newAiportBuf;
};

Walnut::Application *Walnut::CreateApplication(int argc, char **argv)
{
    Walnut::ApplicationSpecification spec;
    spec.Name = "AtisQuitaine";

    Walnut::Application *app = new Walnut::Application(spec);
    app->PushLayer<ExampleLayer>();
    app->SetMenubarCallback([app]() {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                app->Close();
            }
            ImGui::EndMenu();
        }
    });
    return app;
}
