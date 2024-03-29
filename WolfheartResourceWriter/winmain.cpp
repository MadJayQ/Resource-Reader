#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <Windows.h>
#include <algorithm>
#include <png.h>


#pragma comment (lib, "libpng.lib")


using namespace std;

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


//Global Variables
png_structp pngPtr = nullptr;
png_infop infoPtr = nullptr;
png_bytep* rowsPtrs = nullptr;
unsigned char* data = nullptr;


void WritePNG();
void ReadPNG();
void __cdecl ReadData(png_structp pngPtr, png_bytep data, png_size_t length);
int ValidatePNG(fstream&);

int __cdecl main()
{
	HEADER h;
	h.signature = 'R' + ('E' << 8) + ('S' << 16);
	h.version = 1;
	IMAGE img;
	img.width = 128;
	img.height = 128;
	img.bbp = 4;
	img.datalength = img.width * img.bbp * img.height;
	unsigned char *buf = new unsigned char[img.datalength];
	int c = 0;
	for (int i = 0; i < img.width; i++)
	{
		for (int j = 0; j < img.height; j++)
		{
			buf[c + 0] = (unsigned char)rand(); //B
			buf[c + 1] = (unsigned char)rand(); //G
			buf[c + 2] = (unsigned char)rand(); //R
			buf[c + 3] = (unsigned char)0; //A

			c += img.bbp;
		}
	}


	std::fstream fh;
	fh.open("../swag.whd", fstream::out | fstream::binary | fstream::trunc);
	fh.write((char*)&h, sizeof(HEADER));
	fh.write((char*)&img, sizeof(IMAGE));
	fh.write((char*)buf, img.datalength);
	fh.close();

	delete[] buf;

	ReadPNG();
	WritePNG();
	

}

void ReadPNG()
{
	fstream fh;
	fh.open("../paddle.png", ios::in | ios::binary);
	if (fh.is_open())
	{
		if (ValidatePNG(fh))
		{
			MessageBox(nullptr, "Not a PNG", "", MB_ICONERROR);
		}
		pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

	}
}

void WritePNG()
{

}

int ValidatePNG(fstream& source)
{
	png_byte sigbyte[8];
	source.read((char*)&sigbyte, 8);

	if (!source.good()) return 1;

	return png_sig_cmp(sigbyte, 0, 8);
}

