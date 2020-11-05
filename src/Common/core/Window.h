#pragma once
#include "shared.h"

namespace diana
{
    struct Window
    {
        uint32 m_Width = 1024;
        uint32 m_Height = 768;
        char* m_WindowName = "Window";
        HWND m_Handle = nullptr;

        void Initialize(HINSTANCE instance);
    };
}
