#pragma once
#include <cstdint>
#include <vector>


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

__declspec(dllexport) class Extensions
{
public:
    __declspec(dllexport) struct Params
    {
        uint32_t width;
        uint32_t height;
        uint32_t stride;
        double fps;
        uint32_t bitrate;
        uint32_t crf;

        enum AVPixelFormat src_format;
        enum AVPixelFormat dst_format;
    };

    __declspec(dllexport) static AVFrame* init_frame(int width, int height, AVPixelFormat format);

    __declspec(dllexport) static vector<uint8_t> RLEEncoding(vector<uint8_t>* data);
    __declspec(dllexport) static vector<uint8_t> RLEDecoding(uint8_t* data, int size);
};

