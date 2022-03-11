#pragma once

#include <chrono>

#include <iostream>
#include <stdio.h>

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

using namespace std;

class ColorEncoder
{
public:
	AVPacket* packet;

	bool Init(const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);
	
	int Encode(uint8_t* buffer_in);

	void Dispose();

	uint8_t* ExtractResults(uint32_t& data_size);


private:
	Extensions::Params* _params = new Extensions::Params();
	AVCodecContext* codec_context;
    AVFrame* bgra_frame;
    AVFrame* frame;
	SwsContext* ctx;

	bool is_open = false;

	bool Write();
	bool GetPacket();
	void RGBAtoYUV(AVFrame* in_frame, AVFrame* frame);
};

extern "C" __declspec(dllexport) ColorEncoder* init_color_encoder(const char* preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);

extern "C" __declspec(dllexport) void dispose_color_encoder(ColorEncoder* encoder);

extern "C" __declspec(dllexport) int encode_color(ColorEncoder* encoder, uint8_t* buffer);

extern "C" __declspec(dllexport) uint8_t* extract_encoded_color(ColorEncoder* encoder, uint32_t& data_size);

