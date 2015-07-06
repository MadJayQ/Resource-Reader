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
#include <png.h>


#pragma comment (lib, "libpng")
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
ID2D1Bitmap* m_pPaddle = nullptr;

HWND hwnd;

unsigned char *pBuffer;


//Function Prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Tick();
bool InitDirect2D();
void Update();
void Render();
void LoadResources();
int ValidatePNG(std::fstream&);
void __cdecl ReadData(png_structp pngPtr, png_bytep data, png_size_t length);

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

void __cdecl ReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
	png_voidp a = png_get_io_ptr(pngPtr);

	((std::fstream*)a)->read((char*)data, length);
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

	fh.open("../paddle.png", std::ios::in | std::ios::binary);
	if (fh.is_open())
	{
		if (ValidatePNG(fh))
		{
			MessageBox(nullptr, "NOT A PNG", "", 0);
		}
	}
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pngPtr)
	{
		MessageBox(nullptr, "ERROR: Couldn't initialize png read struct", "", 0);
		return;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		MessageBox(nullptr, "ERROR: Couldn't initialize png info struct", "", 0);
		png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);
		return;
	}

	png_bytep* rowsPtrs = nullptr;
	unsigned char* data = nullptr;
	
	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
		if (rowsPtrs != nullptr) delete[] rowsPtrs;
		if (data != nullptr) delete[] data;

		MessageBox(nullptr, "ERROR: An error occured while reading the PNG file", "", 0);

		return;
	}
	
	png_set_read_fn(pngPtr, (png_voidp)&fh, ReadData);

	png_set_sig_bytes(pngPtr, 8);
	png_read_info(pngPtr, infoPtr);

	png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
	png_uint_32 height = png_get_image_height(pngPtr, infoPtr);

	png_uint_32 bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	png_uint_32 channels = png_get_channels(pngPtr, infoPtr);
	png_uint_32 color_type = png_get_color_type(pngPtr, infoPtr);

	rowsPtrs = new png_bytep[height];
	data = new unsigned char[width * height * bitdepth * channels / 8];

	const unsigned int stride = width * bitdepth * channels / 8; 

	for (size_t i = 0; i < height; i++)
	{
		png_uint_32 q = (height - i - 1) * stride;
		rowsPtrs[i] = (png_bytep)data + q;
	}

	png_read_image(pngPtr, rowsPtrs);

	delete[](png_bytep)rowsPtrs;
	png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);

	hr = m_pWICFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGR, stride, width * height * bitdepth * channels / 8, data, &m_pWICBitmap);
	if (FAILED(hr))
	{
		sprintf_s(cBuffer, "Memory Error Code: 0x%x", hr);
		MessageBox(nullptr, cBuffer, "ERROR", MB_ICONERROR);
	}

	hr = m_pRenderTarget->CreateBitmapFromWicBitmap(m_pWICBitmap, &m_pPaddle);
	if (FAILED(hr))
	{
		sprintf_s(cBuffer, "Error Code: 0x%x", hr);
		MessageBox(nullptr, cBuffer, "ERROR", MB_ICONERROR);
	}
	

	delete[] data;
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

int ValidatePNG(std::fstream& source)
{
	png_byte pngsig[8];
	source.read((char*)&pngsig, 8);

	if (!source.good()) return 1;

	return png_sig_cmp(pngsig, 0, 8);
}

void Tick()
{
	Update();
	Render();
}

void Update()
{

}
void Render()
{
	m_pRenderTarget->BeginDraw();
	m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	m_pRenderTarget->DrawBitmap(m_pBitmap, D2D1::RectF(0, 0, 800, 600));
	m_pRenderTarget->DrawBitmap(m_pPaddle, D2D1::RectF(0, 0, 32, 192));
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