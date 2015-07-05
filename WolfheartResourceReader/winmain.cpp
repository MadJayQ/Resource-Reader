#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincodec.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <memory>
#include <algorithm>



#pragma comment (lib, "windowscodecs")
#pragma comment (lib, "d2d1")


struct HEADER
{
	int signature;
	int version;
};

struct IMAGE
{
	int width;
	int height;
	int bbp;
	unsigned int datalength;
};

struct FOOTER
{

};


template<class cClass>
inline void SafeRelease(cClass **ppClass)
{
	if (*ppClass != NULL)
	{
		(*ppClass)->Release();
		(*ppClass) = NULL;
	}
}


IWICImagingFactory* m_pWICFactory = nullptr;
ID2D1Factory* m_pFactory = nullptr;
ID2D1HwndRenderTarget* m_pRenderTarget = nullptr;
IWICBitmap* m_pWICBitmap = nullptr;
ID2D1Bitmap* m_pBitmap = nullptr;

HWND hwnd;

unsigned char *pBuffer;


//Function Prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Tick();
bool InitDirect2D();
void Update();
void Render();
void LoadResources();

int __stdcall wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = "WolfheartReader_Class";
	wcex.lpszMenuName = nullptr;
	wcex.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(nullptr, "Class Registering Failed!", "", MB_ICONERROR);
	}

	hwnd = CreateWindow("WolfheartReader_Class", "WolfheartReader", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd)
	{
		MessageBox(nullptr, "Window Creation Failed!", "", MB_ICONERROR);
	}
	else
	{
		ShowWindow(hwnd, nCmdShow);
		if (!InitDirect2D())
		{
			return 0;
		}
		LoadResources();
	}

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Tick();
		}
	}

	return (int)msg.wParam;

}

void LoadResources()
{

	HRESULT hr;
	HEADER h;
	IMAGE img;
	std::fstream fh;
	char cBuffer[64];
	fh.open("../swag.whd", std::fstream::in | std::fstream::binary);
	if (fh.is_open())
	{
		fh.read((char*)&h.signature, sizeof(h.signature));
		fh.read((char*)&h.version, sizeof(h.version));
		fh.read((char*)&img.width, sizeof(img.width));
		fh.read((char*)&img.height, sizeof(img.height));
		fh.read((char*)&img.bbp, sizeof(img.bbp));
		fh.read((char*)&img.datalength, sizeof(img.datalength));
		pBuffer = new unsigned char[img.datalength];
		fh.read((char*)pBuffer, img.datalength);
		fh.close();
		hr = m_pWICFactory->CreateBitmapFromMemory(img.width, img.height, GUID_WICPixelFormat32bppPBGRA, img.bbp * img.width, img.datalength, pBuffer, &m_pWICBitmap);
		if (FAILED(hr))
		{
			sprintf_s(cBuffer, "doh", hr);
			MessageBox(nullptr, cBuffer, "", 0);
		}
		hr = m_pRenderTarget->CreateBitmapFromWicBitmap(m_pWICBitmap, &m_pBitmap);
		if (FAILED(hr))
		{
			char cBuffer[64];
			sprintf_s(cBuffer, "0x%x", hr);
			MessageBox(nullptr, cBuffer, "", MB_ICONINFORMATION);
		}
	}
}

bool InitDirect2D()
{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
	if (FAILED(hr))
	{
		return false;
	}
	hr = m_pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(800, 600), D2D1_PRESENT_OPTIONS_NONE), &m_pRenderTarget);
	if (FAILED(hr))
	{
		return false;
	}
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory), (void**)&m_pWICFactory);
	if (FAILED(hr))
	{
		return false;
	}
	
	return true;
}

void Tick()
{
	Update();
	Render();
}

void Update()
{
	/*
	HRESULT hr = S_OK;

	unsigned char *buffer = new unsigned char[800 * 4 * 600];

	int width = 800;
	int height = 600;

	int c = 0;
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			buffer[c + 0] = (unsigned char)rand();
			buffer[c + 1] = (unsigned char)rand();
			buffer[c + 2] = (unsigned char)rand();
			buffer[c + 3] = (unsigned char)rand();

			c += 4;
		}
	}

	//hr = m_pWICFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppPBGRA, 4 * width, 800 * 4 * 600, buffer, &m_pWICBitmap);

	if (FAILED(hr))
	{
		char cBuffer[64];
		sprintf_s(cBuffer, "0x%x", hr);
		MessageBox(nullptr, cBuffer, "", MB_ICONINFORMATION);
	}
	

	hr = m_pRenderTarget->CreateBitmapFromWicBitmap(m_pWICBitmap, &m_pBitmap);
	if (FAILED(hr))
	{
		char cBuffer[64];
		sprintf_s(cBuffer, "0x%x", hr);
		MessageBox(nullptr, cBuffer, "", MB_ICONINFORMATION);
	}
	


	delete[] buffer;
	*/
}
void Render()
{
	m_pRenderTarget->BeginDraw();
	m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	m_pRenderTarget->DrawBitmap(m_pBitmap, D2D1::RectF(0, 0, 800, 600));
	m_pRenderTarget->EndDraw();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DESTROY)
	{
		delete[] pBuffer;
		SafeRelease(&m_pBitmap);
		SafeRelease(&m_pFactory);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pWICFactory);
		SafeRelease(&m_pWICBitmap);
		PostQuitMessage(0);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}