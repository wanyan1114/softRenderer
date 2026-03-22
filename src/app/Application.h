#pragma once

#include "app/layer/LayerContext.h"
#include "app/layer/LayerStack.h"

#include "platform/Window.h"

#include <string>

namespace sr {

class Application {
public:
    Application(std::string title, int width, int height);

    int Run();

private:
    void BuildLayers();
    bool CreateWindow();
    int RunMainLoop(LayerContext& context);
    void PrintStartupMessage(const StartupState& startupState) const;
    void Shutdown(LayerContext& context);

    std::string m_Title;
    int m_Width;
    int m_Height;
    platform::Window m_Window;
    LayerStack m_LayerStack;
};

} // namespace sr

