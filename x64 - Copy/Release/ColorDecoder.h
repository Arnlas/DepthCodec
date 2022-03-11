#pragma once
#include <chrono>
#include <iostream>

#include "Extension.h"

extern "C"
{
#include <libavformat\avformat.h>
#include <libavutil\rational.h>
#include <libswscale\swscale.h>
#include <libavutil\opt.h>
#include <libavutil\error.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

class ColorDecoder
{
public:
	AVFrame* frame;
	AVPacket* packet = av_packet_alloc();

	bool Init(const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);
	int Decode(uint8_t* data, uint32_t size);

	void Dispose();

	uint8_t* ExtractResults();

private:
	Extensions::Params* _params = new Extensions::Params();
	AVFrame* yuv_frame;
	AVCodecContext* codec_context;
	SwsContext* ctx;

	bool is_open = false;

	void YUVtoBGRA(AVFrame* yuv, AVFrame* bgra);
};

extern "C" __declspec(dllexport) ColorDecoder* init_color_decoder(const char* preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);

extern "C" __declspec(dllexport) void dispose_color_decoder(ColorDecoder* decoder);

extern "C" __declspec(dllexport) int decode_color(ColorDecoder* decoder, uint8_t* data, uint32_t size);

extern "C" __declspec(dllexport) uint8_t* extract_decoded_color(ColorDecoder* decoder);



