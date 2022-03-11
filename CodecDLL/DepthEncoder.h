#pragma once
#include "Extension.h"

#include <chrono>

#include <iostream>

extern "C"
{
#include <libswscale\swscale.h>
#include <libavcodec/avcodec.h>
}

#include <bitset>

using namespace std;

class DepthEncoder
{
public:
	AVPacket* packet;
	vector<uint8_t> lossless_bits;

	bool Init(const int preset, const int source_format, const int dest_format,
		const uint32_t crf, const uint32_t width, const uint32_t height,
		const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size, float qcompress);
	
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

extern "C" __declspec(dllexport) DepthEncoder* init_depth_encoder(const int preset, const int source_format, const int dest_format,
	const uint32_t crf, const uint32_t width, const uint32_t height,
	const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size, float qcompress)
{
	auto encoder = new DepthEncoder();
	bool inited = encoder->Init(preset, source_format, dest_format, crf, width, height, stride,
		fps, bitrate, gop_size, qcompress);

	if (!inited)
	{
		encoder->Dispose();
		delete encoder;
		return nullptr;
	}

	return encoder;
}

extern "C" __declspec(dllexport) void dispose_depth_encoder(DepthEncoder* encoder)
{
	encoder->Dispose();
	delete encoder;
}

extern "C" __declspec(dllexport) int encode_depth(DepthEncoder* encoder, uint8_t* buffer)
{
	cout << "Entered Encoding 555" << endl;
	return encoder->Encode(buffer);
}

extern "C" __declspec(dllexport) uint8_t* extract_encoded_depth(DepthEncoder* encoder, uint32_t& high_size, uint32_t& data_size)
{
	cout << "Extraction Entered" << endl;
	uint8_t* data = encoder->ExtractResult(high_size, data_size);

	return data;
}

