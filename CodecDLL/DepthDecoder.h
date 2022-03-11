#pragma once
#include "Extension.h"

#include <chrono>
#include <iostream>

extern "C"
{
//#include <libavformat\avformat.h>
//#include <libavutil\rational.h>
#include <libswscale\swscale.h>
//#include <libavutil\opt.h>
//#include <libavutil\error.h>
#include <libavcodec/avcodec.h>
//#include <libavutil/imgutils.h>
}

class DepthDecoder
{
public:
	AVFrame* frame;
	AVPacket* packet;

	bool Init(const char* preset, const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);
	
	int Decode(uint8_t* data, const uint32_t high_size, const uint32_t low_size);

	uint8_t* ExtractResults();

	void Dispose();


private:
	Extensions::Params* _params = new Extensions::Params();
	AVCodecContext* codec_context;

	uint8_t* decoded_buffer;

	bool is_open = false;

	void YUVToDepth(uint8_t* lossless, int lossless_size, AVFrame* frame, int size);
};

extern "C" __declspec(dllexport) DepthDecoder* init_depth_decoder(const char* preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size)
{
	auto decoder = new DepthDecoder();
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

extern "C" __declspec(dllexport) void dispose_depth_decoder(DepthDecoder* decoder)
{
	decoder->Dispose();
	delete decoder;
}

extern "C" __declspec(dllexport) int decode_depth(DepthDecoder* decoder, uint8_t* data, uint32_t high_data_size, uint32_t low_data_size)
{
	return decoder->Decode(data, high_data_size, low_data_size);
}

extern "C" __declspec(dllexport) uint8_t* extract_decoded_depth(DepthDecoder* decoder)
{
	return decoder->ExtractResults();
}



