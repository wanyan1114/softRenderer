#pragma once

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
};

} // namespace sr
