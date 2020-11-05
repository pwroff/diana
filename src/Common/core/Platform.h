#pragma once
#include "Utils.h"
#include "d3d/d3dUtil.h"
#include "d3d/GameTimer.h"

namespace diana
{
	class Platform : public Singletone<Platform>
	{
        //WindowClass* m_Window;
        // Hardware GPU device
        //DeviceClass* m_Device;
        HINSTANCE m_Instance = nullptr; // application instance handle
        HWND m_Window = nullptr; // main window handle
        int m_ClientWidth = 1024;
        int m_ClientHeight = 768;
        bool mAppPaused = false;  // is the application paused?
        bool mMinimized = false;  // is the application minimized?
        bool mMaximized = false;  // is the application maximized?
        bool mResizing = false;   // are the resize bars being dragged?
        bool mFullscreenState = false;// fullscreen enabled
		bool mShotDown = false;

        // Set true to use 4X MSAA (§4.1.8).  The default is false.
        bool m4xMsaaState = false;    // 4X MSAA enabled
        UINT m4xMsaaQuality = 0;      // quality level of 4X MSAA

        // Used to keep track of the “delta-time” and game time (§4.4).
        class GameTimer mTimer;
        
        Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
        Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        UINT64 mCurrentFence = 0;
        
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

        static const int SwapChainBufferCount = 2;
        int mCurrBackBuffer = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

        D3D12_VIEWPORT mScreenViewport; 
        D3D12_RECT mScissorRect;

        UINT mRtvDescriptorSize = 0;
        UINT mDsvDescriptorSize = 0;
        UINT mCbvSrvUavDescriptorSize = 0;

        // Derived class should set these in derived constructor to customize starting values.
        D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
        DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    private:
        void CreateSystemWindow(const char* window_name, int width, int height);
        void InitDirect3D();
        void OnResize();
        void LogAdapters() const;
        void LogAdapterOutputs(IDXGIAdapter* adapter) const;
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const;
    public:
		void Initialize();
        void Run();
		// Move to window class
		LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		~Platform();
	};
}
