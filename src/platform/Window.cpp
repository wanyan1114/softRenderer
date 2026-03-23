#include "platform/Window.h"

#include "render/Framebuffer.h"

#include <windows.h>

#include <string>
#include <utility>

namespace sr::platform {

namespace {

std::wstring ToWide(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}

constexpr wchar_t kWindowClassName[] = L"SoftRendererSkeletonWindow";
constexpr DWORD kWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

Window* GetWindowFromHandle(HWND handle)
{
    return reinterpret_cast<Window*>(GetWindowLongPtrW(handle, GWLP_USERDATA));
}

bool TryMapKey(WPARAM virtualKey, Key& key)
{
    switch (virtualKey) {
    case 'W':
        key = Key::W;
        return true;
    case 'A':
        key = Key::A;
        return true;
    case 'S':
        key = Key::S;
        return true;
    case 'D':
        key = Key::D;
        return true;
    case 'Q':
        key = Key::Q;
        return true;
    case 'E':
        key = Key::E;
        return true;
    case 'F':
        key = Key::F;
        return true;
    case 'G':
        key = Key::G;
        return true;
    case 'R':
        key = Key::R;
        return true;
    case 'T':
        key = Key::T;
        return true;
    case VK_SPACE:
        key = Key::Space;
        return true;
    case VK_LSHIFT:
    case VK_SHIFT:
        key = Key::LeftShift;
        return true;
    default:
        return false;
    }
}

LRESULT CALLBACK WindowProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        Window* window = static_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    Window* window = GetWindowFromHandle(handle);
    if (window != nullptr) {
        long long result = 0;
        if (window->OnMessage(message,
                static_cast<unsigned long long>(wParam),
                static_cast<long long>(lParam),
                result)) {
            return static_cast<LRESULT>(result);
        }
    }

    return DefWindowProcW(handle, message, wParam, lParam);
}

bool RegisterWindowClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) {
        return true;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));

    if (RegisterClassW(&windowClass) == 0) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    registered = true;
    return true;
}

} // namespace

Window::Window(std::string title, int width, int height)
    : m_Title(std::move(title)),
      m_Width(width),
      m_Height(height),
      m_Handle(nullptr),
      m_Closed(false),
      m_InputState{}
{
}

Window::~Window()
{
    if (m_Handle == nullptr) {
        return;
    }

    HWND handle = static_cast<HWND>(m_Handle);
    if (IsWindow(handle)) {
        DestroyWindow(handle);
    }

    m_Handle = nullptr;
}

bool Window::Create()
{
    SetProcessDPIAware();

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!RegisterWindowClass(instance)) {
        return false;
    }

    RECT clientRect{ 0, 0, m_Width, m_Height };
    AdjustWindowRect(&clientRect, kWindowStyle, FALSE);

    const std::wstring title = ToWide(m_Title);
    HWND handle = CreateWindowExW(
        0,
        kWindowClassName,
        title.c_str(),
        kWindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (handle == nullptr) {
        return false;
    }

    m_Handle = handle;
    m_Closed = false;
    ResetInput();
    return true;
}

void Window::Show()
{
    if (m_Handle == nullptr) {
        return;
    }

    HWND handle = static_cast<HWND>(m_Handle);
    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);
}

bool Window::ProcessEvents()
{
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            m_Closed = true;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (!m_Closed && m_Handle != nullptr && !IsWindow(static_cast<HWND>(m_Handle))) {
        m_Closed = true;
    }

    return !m_Closed;
}

bool Window::Present(const render::Framebuffer& framebuffer) const
{
    if (m_Handle == nullptr || m_Closed) {
        return false;
    }

    HWND handle = static_cast<HWND>(m_Handle);
    if (!IsWindow(handle)) {
        return false;
    }

    RECT clientRect{};
    if (!GetClientRect(handle, &clientRect)) {
        return false;
    }

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0) {
        return true;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = framebuffer.Width();
    bitmapInfo.bmiHeader.biHeight = -framebuffer.Height();
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC dc = GetDC(handle);
    if (dc == nullptr) {
        return false;
    }

    const int result = StretchDIBits(
        dc,
        0,
        0,
        clientWidth,
        clientHeight,
        0,
        0,
        framebuffer.Width(),
        framebuffer.Height(),
        framebuffer.Data(),
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);

    ReleaseDC(handle, dc);
    return result != GDI_ERROR;
}

bool Window::OnMessage(unsigned int message, unsigned long long wParam, long long, long long& result)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(static_cast<HWND>(m_Handle));
        result = 0;
        return true;
    case WM_DESTROY:
        m_Closed = true;
        ResetInput();
        PostQuitMessage(0);
        result = 0;
        return true;
    case WM_KILLFOCUS:
        ResetInput();
        result = 0;
        return true;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        Key key = Key::W;
        if (TryMapKey(static_cast<WPARAM>(wParam), key)) {
            SetKeyState(key, true);
            result = 0;
            return true;
        }
        break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        Key key = Key::W;
        if (TryMapKey(static_cast<WPARAM>(wParam), key)) {
            SetKeyState(key, false);
            result = 0;
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

void Window::SetKeyState(Key key, bool isDown)
{
    m_InputState.keyStates[static_cast<std::size_t>(key)] = isDown;
}

void Window::ResetInput()
{
    m_InputState.keyStates.fill(false);
}

} // namespace sr::platform
