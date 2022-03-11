#pragma once
#include "Extension.h"

#include <chrono>

#include <iostream>
#include <stdio.h>

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

#include <bitset>

using namespace std;

class DepthEncoder
{
public:
	AVPacket* packet;
	vector<uint8_t> lossless_bits;

	bool Init(const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);
	
	int Encode(uint8_t* buffer_in);

	uint8_t* ExtractResult(uint32_t& high_size, uint32_t& data_size);

	void Dispose();


private:
	Extensions::Params* _params = new Extensions::Params();
	AVCodecContext* codec_context;
	AVFrame* frame;

	bool is_open = false;

	bool Write();
	bool GetPacket();
	void DepthToYUV(uint16_t* depth, int size, AVFrame* yuv);
};

extern "C" __declspec(dllexport) DepthEncoder* init_depth_encoder(const char* preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size);

extern "C" __declspec(dllexport) void dispose_depth_encoder(DepthEncoder* encoder);

extern "C" __declspec(dllexport) int encode_depth(DepthEncoder* encoder, uint8_t* buffer);

extern "C" __declspec(dllexport) uint8_t* extract_encoded_depth(DepthEncoder* encoder, uint32_t& high_size, uint32_t& data_size);

