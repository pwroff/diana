#include "Platform.h"

namespace
{
    LRESULT CALLBACK
    MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Forward hwnd on because we can get messages (e.g., WM_CREATE)
        // before CreateWindow returns, and thus before mhMainWnd is valid.
        return diana::Platform::GetInstance()->MsgProc(hwnd, msg, wParam, lParam);
    }
}
using Microsoft::WRL::ComPtr;
namespace diana
{
	constexpr char MainWindowName[] = "MainWnd";
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
        wc.lpszClassName = MainWindowName;
        if( !RegisterClass(&wc) )
        {
            MessageBox(0, "RegisterClass Failed.", 0, 0);
			ThrowException("Couldn't register a window");
        }
        // Compute window rectangle dimensions based on requested client area dimensions.
        RECT R = { 0, 0, window_width, window_height };
        AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
        int width  = R.right - R.left;
        int height = R.bottom - R.top;
        m_Window = CreateWindow(MainWindowName, window_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_Instance, 0);
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
		CreateWindowDependentResources();
        OnResize();
    }

    void Platform::Run()
    {
		mTimer.Reset();
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
				mTimer.Tick();

				if (!mAppPaused)
				{
					CalculateFrameStats();
					Update(mTimer);
					Draw(mTimer);
				}
				else
				{
					Sleep(100);
				}
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
		if (md3dDevice != nullptr)
			FlushCommandQueue();
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
		CreateCommandListAndAllocator();
    }

    void Platform::OnResize()
    {
		assert(md3dDevice);
		assert(mSwapChain);
		assert(mDirectCmdListAlloc);

		// Flush before changing any resources.
		FlushCommandQueue();

		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < SwapChainBufferCount; ++i)
			mSwapChainBuffer[i].Reset();
		mDepthStencilBuffer.Reset();

		// Resize the swap chain.
		ThrowIfFailed(mSwapChain->ResizeBuffers(
			SwapChainBufferCount,
			m_ClientWidth, m_ClientHeight,
			mBackBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		mCurrBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
			md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, mRtvDescriptorSize);
		}

		// Create the depth/stencil buffer and view.
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = m_ClientWidth;
		depthStencilDesc.Height = m_ClientHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = mDepthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = mDepthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

		// Transition the resource from its initial state to be used as a depth buffer.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		// Execute the resize commands.
		ThrowIfFailed(mCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until resize is complete.
		FlushCommandQueue();

		// Update the viewport transform to cover the client area.
		mScreenViewport.TopLeftX = 0;
		mScreenViewport.TopLeftY = 0;
		mScreenViewport.Width = static_cast<float>(m_ClientWidth);
		mScreenViewport.Height = static_cast<float>(m_ClientHeight);
		mScreenViewport.MinDepth = 0.0f;
		mScreenViewport.MaxDepth = 1.0f;

		mScissorRect = { 0, 0, m_ClientWidth, m_ClientHeight };
    }

	void Platform::CreateWindowDependentResources()
	{
		// Descriptor heaps

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
			&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
			&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

		CreateSwapChain();
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

	// Those are used during rendering (mainly for seting up pipeline and clearing render targets)
	void Platform::CreateCommandListAndAllocator()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

		ThrowIfFailed(md3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

		ThrowIfFailed(md3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mDirectCmdListAlloc.Get(), // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(mCommandList.GetAddressOf())));

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		mCommandList->Close();
		LOG_S("Created command list and command allocator");
	}
	void Platform::FlushCommandQueue()
	{
		// Advance the fence value to mark commands up to this fence point.
		mCurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

		// Wait until the GPU has completed commands up to this fence point.
		if (mFence->GetCompletedValue() < mCurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
	void Platform::CreateSwapChain()
	{
		// Release the previous swapchain we will be recreating.
		mSwapChain.Reset();

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = m_ClientWidth;
		sd.BufferDesc.Height = m_ClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = mBackBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
		sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = SwapChainBufferCount;
		sd.OutputWindow = m_Window;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// Note: Swap chain uses queue to perform flush.
		ThrowIfFailed(mdxgiFactory->CreateSwapChain(
			mCommandQueue.Get(),
			&sd,
			mSwapChain.GetAddressOf()));

		LOG_S("Created swap chain");
	}
	ID3D12Resource* Platform::CurrentBackBuffer() const
	{
		return mSwapChainBuffer[mCurrBackBuffer].Get();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE Platform::CurrentBackBufferView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
			mCurrBackBuffer,
			mRtvDescriptorSize);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE Platform::DepthStencilView() const
	{
		return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	}
	void Platform::CalculateFrameStats()
	{
		// Code computes the average frames per second, and also the 
		// average time it takes to render one frame.  These stats 
		// are appended to the window caption bar.

		static int frameCnt = 0;
		static float timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
		{
			float fps = (float)frameCnt; // fps = frameCnt / 1
			float mspf = 1000.0f / fps;

			std::string fpsStr = std::to_string(fps);
			std::string mspfStr = std::to_string(mspf);

			std::string windowText = "MainWindow  fps:" + fpsStr + "   mspf: " + mspfStr;

			SetWindowText(m_Window, windowText.c_str());

			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}
	void Platform::Update(const GameTimer& timer)
	{
	}
	void Platform::Draw(const GameTimer& timer)
	{
		FlushCommandQueue();

		static FLOAT Color[] = { 0.0f, 0.0f, 0.0f, 1.0f };

		for (auto i = 0; i < 3; i++)
		{
			Color[i] = Color[i] + timer.DeltaTime() * (i / 3.0f);
			if (Color[i] > 1.0f)
				Color[i] = 0.0f;
		}
		{ // Begin Frame
			// Reuse the memory associated with command recording.
			// We can only reset when the associated command lists have finished execution on the GPU.
			ThrowIfFailed(mDirectCmdListAlloc->Reset());
			// Reset command list with no/dummy initial pipeline state
			mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);
			mCommandList->RSSetViewports(1, &mScreenViewport);
			mCommandList->RSSetScissorRects(1, &mScissorRect);

			// Indicate a state transition on the resource usage.
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Clear the back buffer and depth buffer.
			mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Color, 0, nullptr);
			mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
		}

		{ // Draw scene
		
		}

		{ // End frame
			// Indicate a state transition on the resource usage.
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

			// Done recording commands.
			ThrowIfFailed(mCommandList->Close());

			// Add the command list to the queue for execution.
			ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
			mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

			// Swap the back and front buffers
			ThrowIfFailed(mSwapChain->Present(0, 0));
			mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;


			// Add an instruction to the command queue to set a new fence point. 
			// Because we are on the GPU timeline, the new fence point won't be 
			// set until the GPU finishes processing all the commands prior to this Signal().
			mCommandQueue->Signal(mFence.Get(), mCurrentFence);
		}
	}
}
