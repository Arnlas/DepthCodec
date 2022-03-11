#include "ColorDecoder.h"

bool ColorDecoder::Init(const char* preset, const int source_format, const int dest_format,
    const uint32_t crf, const uint32_t width, const uint32_t height,
    const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size)
{
    //auto codec = avcodec_find_decoder_by_name("h264_cuvid");
    auto codec = avcodec_find_decoder(AV_CODEC_ID_H264);

    if (codec == nullptr) return false;

    codec_context = avcodec_alloc_context3(codec);

    if (!codec_context)
    {
        return false;
    }

    _params->width = width;
    _params->height = height;
    _params->stride = stride;
    _params->fps = fps;
    _params->bitrate = bitrate;
    _params->crf = crf;
    _params->src_format = (AVPixelFormat)source_format;
    _params->dst_format = (AVPixelFormat)dest_format;
    codec_context->codec_id = AV_CODEC_ID_H264;
    codec_context->bit_rate = bitrate;
    codec_context->width = static_cast<int>(width);
    codec_context->height = static_cast<int>(height);
    codec_context->time_base = av_d2q(1.0 / fps, 120);
    codec_context->pix_fmt = (AVPixelFormat)dest_format;
    codec_context->gop_size = gop_size;
    codec_context->max_b_frames = 0;
    codec_context->qcompress = 0.5;

    int ret = avcodec_open2(codec_context, codec, nullptr);

    if (ret != 0)
    {
        return false;
    }

    if (frame == nullptr) {
        frame = Extensions::init_frame(_params->width, _params->height, _params->src_format, true);
        yuv_frame = Extensions::init_frame(_params->width, _params->height, _params->src_format, true);

        ctx = sws_getCachedContext(ctx, _params->width, _params->height,
            _params->dst_format,
            _params->width, _params->height,
            _params->src_format,
            0, 0, 0, 0);
    	
        packet = av_packet_alloc();
    }
	
    is_open = true;
    return true;
}

int ColorDecoder::Decode(uint8_t* data, uint32_t size)
{
    //Just double checking is encoder been initialised
    if (!is_open) return -1;

    packet->data = new uint8_t[size];
    std::copy(data, data + size, packet->data);
    packet->size = size;
    //Initialise frame if it is not present
    if (avcodec_send_packet(codec_context, packet) != 0) {
        delete[] packet->data;
        return -2;
    }

    if (avcodec_receive_frame(codec_context, yuv_frame) != 0) {
        delete[] packet->data;
        return -1;
    }
    //Convert Depth to YUV and assign data to frame
    //YUV2RGB(yuv_frame->data[0], yuv_frame->data[1], yuv_frame->data[2], frame->data[0], _params->width, _params->height, 4);
    YUVtoBGRA(yuv_frame, frame);

    delete[] packet->data;
    cout << "Decoded" << endl;
    return 0;
}

void ColorDecoder::YUVtoBGRA(AVFrame* yuv, AVFrame* bgra)
{
    int depth = sws_scale(ctx, yuv->data, yuv->linesize, 0, yuv->height, bgra->data, bgra->linesize); 
}

void ColorDecoder::Dispose()
{
    delete _params;
	
    if (packet != nullptr) av_packet_free(&packet);
    if (frame != nullptr) av_frame_free(&frame);
    if (yuv_frame != nullptr) av_frame_free(&yuv_frame);

    avcodec_close(codec_context);
    avcodec_free_context(&codec_context);

    if (ctx != nullptr) sws_freeContext(ctx);
}

uint8_t* ColorDecoder::ExtractResults()
{
    cout << "Extracting data" << endl;
    auto result = new uint8_t[_params->height * frame->linesize[0]];
    //memcpy(result, frame->data[0], _params->width * _params->height * 4);
    std::copy(frame->data[0], frame->data[0] + _params->height * frame->linesize[0], result);
    return frame->data[0];
}

void ColorDecoder::ExtractResults(uint8_t* result)
{
    std::copy(frame->data[0], frame->data[0] + _params->height * frame->linesize[0], result);
}

void ColorDecoder::YUV2RGB(void* yDataIn, void* uDataIn, void* vDataIn, void* rgbDataOut, int w, int h, int outNCh)
{

    const int ch2 = 2 * outNCh;

    uint8_t* pRGBs = static_cast<uint8_t*>(rgbDataOut);
    uint8_t* pYs = static_cast<uint8_t*>(yDataIn);
    uint8_t* pUs = static_cast<uint8_t*>(uDataIn);
    uint8_t* pVs = static_cast<uint8_t*>(vDataIn);

#pragma omp parallel for
    for (int r = 0; r < h - 1; r += 2)
    {
        uint8_t* pRGB = pRGBs + r * w * outNCh;
        uint8_t* pRGB2 = pRGBs + (r + 1) * w * outNCh;
        
        uint8_t* pY = pYs + r * w;
        uint8_t* pY2 = pYs + (r + 1) * w;
        
        uint8_t* pU = pUs + (r / 2) * (w / 2);
        uint8_t* pV = pVs + (r / 2) * (w / 2);
        

        //process two pixels at a time
//#pragma omp parallel for
        for (int c = 0; c < w - 1; c += 2)
        {
            ProcessTwoRows(pY, pY2, pU, pV, pRGB, pRGB2);

            pRGB += ch2;
            pRGB2 += ch2;
            pY += 2;
            pY2 += 2;
            pU += 1;
            pV += 1;
        }
    }
}

void ColorDecoder::ProcessTwoRows(uint8_t* pY, uint8_t* pY2, uint8_t* pU, uint8_t* pV, uint8_t* pRGB, uint8_t* pRGB2)
{
    const int C1 = 298 * (pY[0] - 16);
    const int C2 = 298 * (pY[1] - 16);
    const int C3 = 298 * (pY2[0] - 16);
    const int C4 = 298 * (pY2[1] - 16);

    const int D = pU[0] - 128;
    const int E = pV[0] - 128;
    const int D516 = 516 * D + 128;
    const int E409 = 409 * E + 128;
    const int DE = -100 * D - 208 * E + 128;

    int R1 = (C1 + E409) >> 8;
    int G1 = (C1 + DE) >> 8;
    int B1 = (C1 + D516) >> 8;

    int R2 = (C2 + E409) >> 8;
    int G2 = (C2 + DE) >> 8;
    int B2 = (C2 + D516) >> 8;

    int R3 = (C3 + E409) >> 8;
    int G3 = (C3 + DE) >> 8;
    int B3 = (C3 + D516) >> 8;

    int R4 = (C4 + E409) >> 8;
    int G4 = (C4 + DE) >> 8;
    int B4 = (C4 + D516) >> 8;

    //unsurprisingly this takes the bulk of the time.
    pRGB[0] = max(0, R1); //static_cast<uint8_t>(R1 < 0 ? 0 : R1 > 255 ? 255 : R1);
    pRGB[1] = max(0, G1); // static_cast<uint8_t>(G1 < 0 ? 0 : G1 > 255 ? 255 : G1);
    pRGB[2] = max(0, B1); // static_cast<uint8_t>(B1 < 0 ? 0 : B1 > 255 ? 255 : B1);

    pRGB[4] = max(0, R2); //static_cast<uint8_t>(R2 < 0 ? 0 : R2 > 255 ? 255 : R2);
    pRGB[5] = max(0, G2); //static_cast<uint8_t>(G2 < 0 ? 0 : G2 > 255 ? 255 : G2);
    pRGB[6] = max(0, B2); //static_cast<uint8_t>(B2 < 0 ? 0 : B2 > 255 ? 255 : B2);

    pRGB2[0] = max(0, R3); //static_cast<uint8_t>(R3 < 0 ? 0 : R3 > 255 ? 255 : R3);
    pRGB2[1] = max(0, G3); //static_cast<uint8_t>(G3 < 0 ? 0 : G3 > 255 ? 255 : G3);
    pRGB2[2] = max(0, B3); //static_cast<uint8_t>(B3 < 0 ? 0 : B3 > 255 ? 255 : B3);

    pRGB2[4] = max(0, R4); //static_cast<uint8_t>(R4 < 0 ? 0 : R4 > 255 ? 255 : R4);
    pRGB2[5] = max(0, G4); //static_cast<uint8_t>(G4 < 0 ? 0 : G4 > 255 ? 255 : G4);
    pRGB2[6] = max(0, B4); //static_cast<uint8_t>(B4 < 0 ? 0 : B4 > 255 ? 255 : B4);
}

