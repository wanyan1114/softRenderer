#include "platform/Window.h"

#include <windows.h>

#include <string>

namespace sr::platform {

namespace {

std::wstring ToWide(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}

constexpr wchar_t kWindowClassName[] = L"SoftRendererSkeletonWindow";

} // namespace

Window::Window(std::string title, int width, int height)
    : m_Title(std::move(title)), m_Width(width), m_Height(height), m_Handle(nullptr)
{
}

Window::~Window()
{
    if (m_Handle != nullptr) {
        DestroyWindow(static_cast<HWND>(m_Handle));
    }
}

bool Window::Create()
{
    const HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;

    RegisterClassW(&windowClass);

    const std::wstring title = ToWide(m_Title);
    HWND handle = CreateWindowExW(
        0,
        kWindowClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_Width,
        m_Height,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (handle == nullptr) {
        return false;
    }

    m_Handle = handle;
    ShowWindow(handle, SW_HIDE);
    UpdateWindow(handle);
    return true;
}

} // namespace sr::platform
