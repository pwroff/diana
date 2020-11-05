#include "Platform.h"

namespace
{
    LRESULT CALLBACK
    MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Forward hwnd on because we can get messages (e.g., WM_CREATE)
        // before CreateWindow returns, and thus before mhMainWnd is valid.
        //return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
        return diana::Platform::GetInstance()->MsgProc(hwnd, msg, wParam, lParam);
    }
}
using Microsoft::WRL::ComPtr;
namespace diana
{
    void Platform::CreateSystemWindow(const char* window_name, int window_width, int window_height)
    {
        m_Instance = GetModuleHandle(NULL);
        WNDCLASS wc;
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = MainWndProc; 
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = m_Instance;
        wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor       = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszMenuName  = 0;
        wc.lpszClassName = "MainWnd";
        if( !RegisterClass(&wc) )
        {
            MessageBox(0, "RegisterClass Failed.", 0, 0);
            throw std::exception("Couldn't register a window");
        }
        // Compute window rectangle dimensions based on requested client area dimensions.
        RECT R = { 0, 0, window_width, window_height };
        AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
        int width  = R.right - R.left;
        int height = R.bottom - R.top;
        m_Window = CreateWindow("MainWnd", window_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_Instance, 0); 
        if( !m_Window )
        {
            MessageBox(0, "CreateWindow Failed.", 0, 0);
            throw std::exception("Couldn't create a window");
        }
        ShowWindow(m_Window, SW_SHOW);
        UpdateWindow(m_Window);
    }
    void Platform::Initialize()
    {
        CreateSystemWindow("My window", 800, 600);
        InitDirect3D();
        //OnResize();
    }

    void Platform::Run()
    {
        MSG msg = {0};

        while(msg.message != WM_QUIT && !mShotDown)
        {
            // If there are Window messages then process them.
            if(PeekMessage( &msg, m_Window, 0, 0, PM_REMOVE ))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
            // Otherwise, do animation/game stuff.
            else
            {	
                
            }
        }
		if (m_Window)
			DestroyWindow(m_Window);
    }

	LRESULT Platform::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			// WM_ACTIVATE is sent when the window is activated or deactivated.  
			// We pause the game when the window is deactivated and unpause it 
			// when it becomes active.  
		case WM_ACTIVATE:
			if (false)
			{
				if (LOWORD(wParam) == WA_INACTIVE)
				{
					mAppPaused = true;
					mTimer.Stop();
				}
				else
				{
					mAppPaused = false;
					mTimer.Start();
				}
			}
			return 0;

			// WM_SIZE is sent when the user resizes the window.  
		case WM_SIZE:
			// Save the new client area dimensions.
			m_ClientWidth = LOWORD(lParam);
			m_ClientHeight = HIWORD(lParam);
			if (md3dDevice)
			{
				if (wParam == SIZE_MINIMIZED)
				{
					mAppPaused = true;
					mMinimized = true;
					mMaximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					mAppPaused = false;
					mMinimized = false;
					mMaximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED)
				{

					// Restoring from minimized state?
					if (mMinimized)
					{
						mAppPaused = false;
						mMinimized = false;
						OnResize();
					}

					// Restoring from maximized state?
					else if (mMaximized)
					{
						mAppPaused = false;
						mMaximized = false;
						OnResize();
					}
					else if (mResizing)
					{
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					{
						OnResize();
					}
				}
			}
			return 0;

			// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		case WM_ENTERSIZEMOVE:
			mAppPaused = true;
			mResizing = true;
			mTimer.Stop();
			return 0;

			// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
			// Here we reset everything based on the new window dimensions.
		case WM_EXITSIZEMOVE:
			mAppPaused = false;
			mResizing = false;
			mTimer.Start();
			OnResize();
			return 0;

			// WM_DESTROY is sent when the window is being destroyed.
		case WM_DESTROY:
			PostQuitMessage(0);
			mShotDown = true;
			return 0;

			// The WM_MENUCHAR message is sent when a menu is active and the user presses 
			// a key that does not correspond to any mnemonic or accelerator key. 
		case WM_MENUCHAR:
			// Don't beep when we alt-enter.
			return MAKELRESULT(0, MNC_CLOSE);

			// Catch this message so to prevent the window from becoming too small.
		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;
			/*
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
			*/
		case WM_KEYUP:
			if (wParam == VK_ESCAPE)
			{
				LOG_S("Shotting down");
				mShotDown = true;
				PostQuitMessage(0);
			}
			//else if ((int)wParam == VK_F2)
				//Set4xMsaaState(!m4xMsaaState);

			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	Platform::~Platform()
	{
		//if (md3dDevice != nullptr)
			//FlushCommandQueue();
	}

    void Platform::InitDirect3D()
    {
        #if defined(DEBUG) || defined(_DEBUG) 
        // Enable the D3D12 debug layer.
        {
            ComPtr<ID3D12Debug> debugController;
            ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            debugController->EnableDebugLayer();
        }
        #endif

        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

        // Try to create hardware device.
        HRESULT hardwareResult = D3D12CreateDevice(
            nullptr,             // default adapter
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice));

        // Fallback to WARP device.
        if(FAILED(hardwareResult))
        {
            ComPtr<IDXGIAdapter> pWarpAdapter;
            ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

            ThrowIfFailed(D3D12CreateDevice(
                pWarpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&md3dDevice)));
        }

        ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&mFence)));

        mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Check 4X MSAA quality support for our back buffer format.
        // All Direct3D 11 capable devices support 4X MSAA for all render 
        // target formats, so we only need to check quality support.

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
        msQualityLevels.Format = mBackBufferFormat;
        msQualityLevels.SampleCount = 4;
        msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        msQualityLevels.NumQualityLevels = 0;
        ThrowIfFailed(md3dDevice->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msQualityLevels,
            sizeof(msQualityLevels)));

        m4xMsaaQuality = msQualityLevels.NumQualityLevels;
        assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
        
    #ifdef _DEBUG
        LogAdapters();
    #endif

        //CreateCommandObjects();
        //CreateSwapChain();
        //CreateRtvAndDsvDescriptorHeaps();

    }

    void Platform::OnResize()
    {

    }

    void Platform::LogAdapters() const
    {
        UINT i = 0;
        IDXGIAdapter* adapter = nullptr;
        std::vector<IDXGIAdapter*> adapterList;
        while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);
            LOG_S("***Adapter: %ls", desc.Description);
            adapterList.push_back(adapter);
            
            ++i;
        }

        for(size_t i = 0; i < adapterList.size(); ++i)
        {
            LogAdapterOutputs(adapterList[i]);
            ReleaseCom(adapterList[i]);
        }
    }

    void Platform::LogAdapterOutputs(IDXGIAdapter* adapter) const
    {
        UINT i = 0;
        IDXGIOutput* output = nullptr;
        while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_OUTPUT_DESC desc;
            output->GetDesc(&desc);
            LOG_S("***Output: %ls", desc.DeviceName);

            LogOutputDisplayModes(output, mBackBufferFormat);

            ReleaseCom(output);

            ++i;
        }
    }

    void Platform::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const
    {
        UINT count = 0;
        UINT flags = 0;

        // Call with nullptr to get list count.
        output->GetDisplayModeList(format, flags, &count, nullptr);

        std::vector<DXGI_MODE_DESC> modeList(count);
        output->GetDisplayModeList(format, flags, &count, &modeList[0]);

        for(auto& x : modeList)
        {
            UINT n = x.RefreshRate.Numerator;
            UINT d = x.RefreshRate.Denominator;
            LOG_S("Width: %i; Height: %i; Refresh: %i / %i", x.Width, x.Height, n, d);
        }
    }
}
