#pragma once
#include <chrono>
#include <future>
#include <iostream>

#include "Extension.h"

extern "C"
{
#include <libswscale\swscale.h>
#include <libavcodec/avcodec.h>
}

//#include <boost/range/algorithm.hpp>

//#include <thread>


class ColorDecoder
{
public:
	AVFrame* frame;
	AVPacket* packet;
	thread t;
	future<int> ret;

	bool Init(const char* preset, const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);
	int Decode(uint8_t* data, uint32_t size);

	//void DecodeThread(uint8_t* data, uint32_t size);
	//int Join();

	void Dispose();

	uint8_t* ExtractResults();
	void ExtractResults(uint8_t* result);

private:
	Extensions::Params* _params = new Extensions::Params();
	AVFrame* yuv_frame;
	AVCodecContext* codec_context;
	SwsContext* ctx;

	bool is_open = false;
	void DecodeT(uint8_t* data, uint32_t size, std::promise<int>&& p);
	void YUVtoBGRA(AVFrame* yuv, AVFrame* bgra);

	static void YUV2RGB(void* yDataIn, void* uDataIn, void* vDataIn, void* rgbDataOut, int w, int h, int outNCh);
	static void ProcessTwoRows(uint8_t* pY, uint8_t* pY2, uint8_t* pU, uint8_t* pV, uint8_t* pRGB, uint8_t* pRGB2);
};

extern "C" __declspec(dllexport) ColorDecoder* init_color_decoder(const char* preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size)
{
	auto decoder = new ColorDecoder();
	bool inited = decoder->Init(preset, source_format, dest_format, crf, width, height, stride,
		fps, bitrate, gop_size);

	if (!inited)
	{
		cout << "Not initialised" << endl;
		decoder->Dispose();
		delete decoder;
		return nullptr;
	}

	cout << "Decoder Initialised" << endl;
	return decoder;
}

extern "C" __declspec(dllexport) void dispose_color_decoder(ColorDecoder* decoder)
{
	decoder->Dispose();
	delete decoder;
}

extern "C" __declspec(dllexport) int decode_color(ColorDecoder* decoder, uint8_t* data, uint32_t size)
{
	return decoder->Decode(data, size);
}

extern "C" __declspec(dllexport) uint8_t* extract_decoded_color(ColorDecoder* decoder)
{
	return decoder->ExtractResults();
}

extern "C" __declspec(dllexport) void extract_decoded_color_non_cpy(ColorDecoder* decoder, uint8_t* result)
{
	return decoder->ExtractResults(result);
}



