#include "window_impl_win32.hpp"
#include "assert.hpp"
#include "log.hpp"

namespace ash::window
{
std::wstring string_to_wstring(std::string_view str)
{
    wchar_t buffer[128] = {};
    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        buffer,
        static_cast<int>(sizeof(buffer)));

    return buffer;
}

std::string wstring_to_string(std::wstring_view str)
{
    char buffer[128] = {};
    WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(wcslen(str.data())),
        buffer,
        static_cast<int>(sizeof(buffer)),
        nullptr,
        nullptr);

    return buffer;
}

void mouse_win32::clip_cursor(bool clip)
{
    if (clip)
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        MapWindowRect(m_hwnd, nullptr, &rect);
        ClipCursor(&rect);
    }
    else
    {
        ClipCursor(nullptr);
    }
}

void mouse_win32::show_cursor(bool show)
{
    ShowCursor(show);
}

bool window_impl_win32::initialize(uint32_t width, uint32_t height, std::string_view title)
{
    m_instance = GetModuleHandle(0);

    WNDCLASSEX wndClassEx;
    memset(&wndClassEx, 0, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClassEx.hIcon = LoadIcon(m_instance, IDI_APPLICATION);
    wndClassEx.hInstance = m_instance;
    wndClassEx.lpfnWndProc = wnd_create_proc;
    wndClassEx.lpszClassName = m_class_name;
    wndClassEx.lpszMenuName = NULL;
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClassEx(&wndClassEx);

    RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    int posX = (GetSystemMetrics(SM_CXSCREEN) - (windowRect.right - windowRect.left)) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - (windowRect.bottom - windowRect.top)) / 2;

    SetProcessDPIAware();

    ASH_ASSERT(title.size() < 128, "The title is too long");
    std::wstring wtitle = string_to_wstring(title);

    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        posX,
        posY,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_instance,
        this);

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x1;
    rid.usUsage = 0x2;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = m_hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        log::error("RegisterRawInputDevices failed");
    }

    m_mouse.set_window_handle(m_hwnd);

    show();

    return true;
}

void window_impl_win32::tick()
{
    m_mouse.reset_relative_cursor();

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_impl_win32::show()
{
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

const void* window_impl_win32::get_handle() const
{
    return &m_hwnd;
}

window_rect window_impl_win32::get_rect() const
{
    RECT rect = {};
    GetClientRect(m_hwnd, &rect);

    window_rect result = {};
    result.x = rect.left;
    result.y = rect.top;
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;

    return result;
}

void window_impl_win32::set_title(std::string_view title)
{
    SetWindowText(m_hwnd, string_to_wstring(title).c_str());
}

LRESULT CALLBACK
window_impl_win32::wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
        window_impl_win32* impl =
            reinterpret_cast<window_impl_win32*>(create_struct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
        SetWindowLongPtr(
            hwnd,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&window_impl_win32::wnd_proc));
        return impl->handle_message(hwnd, message, wparam, lparam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

LRESULT CALLBACK window_impl_win32::wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    window_impl_win32* const impl =
        reinterpret_cast<window_impl_win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    return impl->handle_message(hwnd, message, wparam, lparam);
}

LRESULT window_impl_win32::handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_MOUSEMOVE: {
        if (m_mouse.get_mode() == mouse_mode::CURSOR_ABSOLUTE)
            m_mouse.set_cursor(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        break;
    }
    case WM_INPUT: {
        if (m_mouse.get_mode() == mouse_mode::CURSOR_RELATIVE)
        {
            RAWINPUT raw;
            UINT rawSize = sizeof(raw);
            GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lparam),
                RID_INPUT,
                &raw,
                &rawSize,
                sizeof(RAWINPUTHEADER));
            if (raw.header.dwType == RIM_TYPEMOUSE &&
                !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            {
                m_mouse.set_cursor(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
            }
        }
        break;
    }
    case WM_LBUTTONDOWN:
        m_mouse.key_down(mouse_key::LEFT_BUTTON);
        break;
    case WM_LBUTTONUP:
        m_mouse.key_up(mouse_key::LEFT_BUTTON);
        break;
    case WM_RBUTTONDOWN:
        m_mouse.key_down(mouse_key::RIGHT_BUTTON);
        break;
    case WM_RBUTTONUP:
        m_mouse.key_up(mouse_key::RIGHT_BUTTON);
        break;
    case WM_MBUTTONDOWN:
        m_mouse.key_down(mouse_key::MIDDLE_BUTTON);
        break;
    case WM_MBUTTONUP:
        m_mouse.key_up(mouse_key::MIDDLE_BUTTON);
        break;
    case WM_KEYDOWN:
        m_keyboard.key_down(static_cast<keyboard_key>(wparam));
        break;
    case WM_KEYUP:
        m_keyboard.key_up(static_cast<keyboard_key>(wparam));
        break;
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}
} // namespace ash::window