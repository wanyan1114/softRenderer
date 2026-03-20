#pragma once

#include "app/CameraLayer.h"

#include <string>

namespace sr {

class Application {
public:
    Application(std::string title, int width, int height);

    int Run();

private:
    std::string m_Title;
    int m_Width;
    int m_Height;
    CameraLayer m_CameraLayer;
};

} // namespace sr
