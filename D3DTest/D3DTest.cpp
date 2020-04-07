#include "stdafx.h"

#define USE_D3D9 1
#define USE_D3D9EX 0
#define USE_D3D11 0

#if (USE_D3D9 && USE_D3D9EX) || (USE_D3D9 && USE_D3D11) || (USE_D3D9EX && USE_D3D11)
#error "Only one of the USE_D3D* defines must be 1"
#endif

#if USE_D3D9 || USE_D3D9EX
// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#elif USE_D3D11
#pragma comment (lib, "d3d11.lib")
#endif

#define SAFERELEASE(x) if (x) { x->Release(); x = nullptr; }

// global declarations
HWND l_mainhWnd = nullptr;
HWND l_childhWnd[2] = {};
bool l_animateWindowSize = false;
bool l_animDirection = false;
#if USE_D3D9
LPDIRECT3D9 l_d3d[2] = {};			// the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 l_d3dDevice[2] = {};	// the pointer to the device class
#elif USE_D3D9EX
LPDIRECT3D9EX l_d3d[2] = {};			// the pointer to our Direct3D interface
LPDIRECT3DDEVICE9EX l_d3dDevice[2] = {};	// the pointer to the device class
#elif USE_D3D11
IDXGISwapChain *l_swapChain[2] = {};
ID3D11Device *l_d3dDevice[2] = {};
ID3D11DeviceContext *l_immediateContext[2] = {};
ID3D11RenderTargetView *l_renderTargetView[2] = {};
ID3D11Texture2D *l_texture[2] = {};
#endif

uint32_t l_screenWidth = 800;
uint32_t l_screenHeight = 600;

void InitD3D(size_t d3dIndex);				// sets up and initializes Direct3D
void RenderFrame(size_t d3dIndex);			// renders a single frame
void CleanD3D(size_t d3dIndex);				// closes Direct3D and releases memory
void OnSize(size_t d3dIndex);
void AllocationTest();

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#if USE_D3D9 || USE_D3D9EX
D3DPRESENT_PARAMETERS GetPresentationParameters(size_t d3dIndex)
{
	D3DCAPS9 caps;
	l_d3d[d3dIndex]->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	D3DPRESENT_PARAMETERS d3dpp{ 0 };
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = l_childhWnd[d3dIndex];
	d3dpp.BackBufferCount = 1;
	if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
	{
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}

	return d3dpp;
}
#endif

#if USE_D3D11
DXGI_SWAP_CHAIN_DESC GetSwapChainDesc(size_t d3dIndex)
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = l_screenWidth / 2;
	sd.BufferDesc.Height = l_screenHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = l_childhWnd[d3dIndex];
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	return sd;
}
#endif

DWORD GetBehaviorFlags()
{
	DWORD flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
	return flags;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx(&wc);

	l_mainhWnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"D3DTest",
		WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SIZEBOX|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
		0, 0,
		l_screenWidth, l_screenHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	l_childhWnd[0] = CreateWindowEx(WS_EX_CLIENTEDGE,
		L"WindowClass",
		nullptr,
		WS_CHILD | WS_VISIBLE,
		0, 0,
		l_screenWidth / 2, l_screenHeight,
		l_mainhWnd,
		nullptr,
		hInstance,
		nullptr);

	l_childhWnd[1] = CreateWindowEx(WS_EX_CLIENTEDGE,
		L"WindowClass",
		nullptr,
		WS_CHILD | WS_VISIBLE,
		l_screenWidth / 2, 0,
		l_screenWidth / 2, l_screenHeight,
		l_mainhWnd,
		nullptr,
		hInstance,
		nullptr);

	ShowWindow(l_mainhWnd, nCmdShow);

	// set up and initialize Direct3D
	InitD3D(0);
	InitD3D(1);

	// enter the main loop:
	int widthModify = 16;
	MSG msg;
	while (true)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)
			&& msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			break;
		}

		if (l_animateWindowSize)
		{
			RECT r;
			GetWindowRect(l_mainhWnd, &r);
			uint32_t width = r.right - r.left;
			uint32_t height = r.bottom - r.top;

			SetWindowPos(l_mainhWnd, nullptr,
				r.left, r.top,
				width + widthModify, height,
				SWP_NOZORDER);
			widthModify = -widthModify;
		}

		RenderFrame(0);
		RenderFrame(1);
	}

	// clean up DirectX and COM
	CleanD3D(0);
	CleanD3D(1);

	return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (hWnd == l_mainhWnd)
		{
			if (wParam == VK_SPACE)
			{
				l_animateWindowSize = !l_animateWindowSize;
			}
			if (wParam == VK_F1)
			{
				AllocationTest();
			}
			if (wParam == VK_F5)
			{
				CleanD3D(0);
				CleanD3D(1);

				InitD3D(0);
				InitD3D(1);
			}
			return 0;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		if (hWnd == l_mainhWnd)
		{
			PostQuitMessage(0);
		}
		return 0;
	case WM_PAINT:
		for (size_t d3dIndex = 0; d3dIndex < 2; ++d3dIndex)
		{
			if (l_childhWnd[d3dIndex] == hWnd)
			{
				PAINTSTRUCT paintStruct = { 0 };
				BeginPaint(hWnd, &paintStruct);

				RenderFrame(d3dIndex);

				EndPaint(hWnd, &paintStruct);
				return 0;
			}
		}
		break;
	case WM_SIZING:
		if (hWnd == l_mainhWnd)
		{
			RECT* r = reinterpret_cast<RECT*>(lParam);
			l_screenWidth = r->right - r->left;
			l_screenHeight = r->bottom - r->top;

			SetWindowPos(l_childhWnd[0], l_mainhWnd, 0, 0, l_screenWidth/2, l_screenHeight, SWP_NOZORDER);
			SetWindowPos(l_childhWnd[1], l_mainhWnd, l_screenWidth/2, 0, l_screenWidth/2, l_screenHeight, SWP_NOZORDER);
			return 0;
		}
		break;
	case WM_SIZE:
		if (hWnd == l_mainhWnd)
		{
			l_screenWidth = LOWORD(lParam);
			l_screenHeight = HIWORD(lParam);

			SetWindowPos(l_childhWnd[0], l_mainhWnd, 0, 0, l_screenWidth / 2, l_screenHeight, SWP_NOZORDER);
			SetWindowPos(l_childhWnd[1], l_mainhWnd, l_screenWidth / 2, 0, l_screenWidth / 2, l_screenHeight, SWP_NOZORDER);

			return 0;
		}
		for (int d3dIndex = 0; d3dIndex < 2; ++d3dIndex)
		{
			if (hWnd == l_childhWnd[d3dIndex])
			{
				OnSize(d3dIndex);
				return 0;
			}
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// this function initializes and prepares Direct3D for use
void InitD3D(size_t d3dIndex)
{
#if USE_D3D9
	l_d3d[d3dIndex] = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp = GetPresentationParameters(d3dIndex);

	// create a device class using this information and the info from the d3dpp struct
	l_d3d[d3dIndex]->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		l_childhWnd[d3dIndex],
		GetBehaviorFlags(),
		&d3dpp,
		&l_d3dDevice[d3dIndex]);
#elif USE_D3D9EX
	Direct3DCreate9Ex(D3D_SDK_VERSION, &l_d3d[d3dIndex]);

	D3DPRESENT_PARAMETERS d3dpp = GetPresentationParameters(d3dIndex);

	// create a device class using this information and the info from the d3dpp struct
	l_d3d[d3dIndex]->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		l_childhWnd[d3dIndex],
		GetBehaviorFlags(),
		&d3dpp,
		nullptr,
		&l_d3dDevice[d3dIndex]);

#elif USE_D3D11
	DXGI_SWAP_CHAIN_DESC sd = GetSwapChainDesc(d3dIndex);
	D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		&featureLevels,
		1,
		D3D11_SDK_VERSION,
		&sd,
		&l_swapChain[d3dIndex],
		&l_d3dDevice[d3dIndex],
		&featureLevel,
		&l_immediateContext[d3dIndex]);

	ID3D11Texture2D *backBuffer;
	hr = l_swapChain[d3dIndex]->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	hr = l_d3dDevice[d3dIndex]->CreateRenderTargetView(backBuffer, NULL, &l_renderTargetView[d3dIndex]);
	backBuffer->Release();
	l_immediateContext[d3dIndex]->OMSetRenderTargets(1, &l_renderTargetView[d3dIndex], NULL);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = 1920;
	texDesc.Height = 1200;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DYNAMIC;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
#endif
}

// this is the function used to render a single frame
void RenderFrame(size_t d3dIndex)
{
#if USE_D3D9 || USE_D3D9EX
	if (l_d3dDevice[d3dIndex])
	{
		auto color = d3dIndex == 0 ? D3DCOLOR_XRGB(0, 128, 0) : D3DCOLOR_XRGB(0, 0, 128);
		l_d3dDevice[d3dIndex]->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);

		l_d3dDevice[d3dIndex]->BeginScene();

		l_d3dDevice[d3dIndex]->EndScene();

#if USE_D3D9
		l_d3dDevice[d3dIndex]->Present(nullptr, nullptr, nullptr, nullptr);
#elif USE_D3D9EX
		l_d3dDevice[d3dIndex]->PresentEx(nullptr, nullptr, nullptr, nullptr, D3DPRESENT_LINEAR_CONTENT);
#endif
	}
#elif USE_D3D11
	if (l_immediateContext[d3dIndex]
		&& l_renderTargetView[d3dIndex]
		&& l_swapChain[d3dIndex])
	{
		if (d3dIndex == 0)
		{
			float color[4] = {0.0f, 0.5f, 0.0f, 1.0f};
			l_immediateContext[d3dIndex]->ClearRenderTargetView(l_renderTargetView[d3dIndex], color);
		}
		else
		{
			float color[4] = { 0.0f, 0.0f, 0.5f, 1.0f };
			l_immediateContext[d3dIndex]->ClearRenderTargetView(l_renderTargetView[d3dIndex], color);
		}

		l_swapChain[d3dIndex]->Present(0, 0);
	}
#endif
}

// this is the function that cleans up Direct3D and COM
void CleanD3D(size_t d3dIndex)
{
#if USE_D3D9 || USE_D3D9EX
	SAFERELEASE(l_d3dDevice[d3dIndex])
	SAFERELEASE(l_d3d[d3dIndex]);
#elif USE_D3D11
	SAFERELEASE(l_swapChain[d3dIndex]);
	SAFERELEASE(l_renderTargetView[d3dIndex]);
	SAFERELEASE(l_d3dDevice[d3dIndex]);
	SAFERELEASE(l_immediateContext[d3dIndex]);
	SAFERELEASE(l_texture[d3dIndex]);
#endif
}

void OnSize(size_t d3dIndex)
{
#if USE_D3D9 || USE_D3D9EX
	auto newPresentParams = GetPresentationParameters(d3dIndex);
	l_d3dDevice[d3dIndex]->Reset(&newPresentParams);
#elif USE_D3D11
	l_immediateContext[d3dIndex]->OMSetRenderTargets(0, 0, 0);
	l_renderTargetView[d3dIndex]->Release();

	HRESULT hr;
	hr = l_swapChain[d3dIndex]->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* backBuffer;
	hr = l_swapChain[d3dIndex]->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

	hr = l_d3dDevice[d3dIndex]->CreateRenderTargetView(backBuffer, NULL,
		&l_renderTargetView[d3dIndex]);
	// Perform error handling here!
	backBuffer->Release();

	l_immediateContext[d3dIndex]->OMSetRenderTargets(1, &l_renderTargetView[d3dIndex], NULL);

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<float>(l_screenWidth/2);
	vp.Height = static_cast<float>(l_screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	l_immediateContext[d3dIndex]->RSSetViewports(1, &vp);
#endif
}

void AllocationTest()
{
	constexpr uint32_t TexWidth = 2*1920;
	constexpr uint32_t TexHeight = 2*1080;
	//char* memoryLeak = new char[1 * 1024 * 1024];

#if USE_D3D9 || USE_D3D9EX
	for (size_t d3dIndex = 0; d3dIndex < 2; ++d3dIndex)
	{
		IDirect3DTexture9 *texture = nullptr;
		HRESULT hr = l_d3dDevice[d3dIndex]->CreateTexture(TexWidth, TexHeight, 0, D3DUSAGE_DYNAMIC,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);

		std::wstringstream ss;
		if (FAILED(hr))
		{
			ss << L"Failed to create texture on window " << d3dIndex << L", hr = " << hr << std::endl;
		}
		else
		{
			ss << L"Texture created for window " << d3dIndex << L" without errors." << std::endl;
		}
		OutputDebugString(ss.str().c_str());
		SAFERELEASE(texture);
	}
#elif USE_D3D11

#endif
}