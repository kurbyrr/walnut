#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

class ExampleLayer : public Walnut::Layer {
public:
  virtual void OnUIRender() override {
    ImGui::Begin("Hello");
    ImGui::Button("Button");
    ImGui::End();

    ImGui::ShowDemoWindow();
  }
};

Walnut::Application *Walnut::CreateApplication(int argc, char **argv) {
  Walnut::ApplicationSpecification spec;
  spec.Name = "Walnut Example";
  spec.Width = 1280;
  spec.Height = 720;

  Walnut::Application *app = new Walnut::Application(spec);
  std::shared_ptr<ExampleLayer> exampleLayer = std::make_shared<ExampleLayer>();
  app->PushLayer(exampleLayer);
  app->SetMenubarCallback([app, exampleLayer]() {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        app->Close();
      }
      ImGui::EndMenu();
    }
  });
  return app;
}
