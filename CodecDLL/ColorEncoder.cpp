#include "ColorEncoder.h"

bool ColorEncoder::Init(const uint32_t preset, const int source_format, const int dest_format,
    const uint32_t crf, const uint32_t width, const uint32_t height,
    const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size, float qcompress)
{
    auto codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (codec == nullptr) return false;

    codec_context = avcodec_alloc_context3(codec);

    if (!codec_context)
    {
        return false;
    }

    const char* pres = "ultrafast";
    switch (preset)
    {
    case 1:
        pres = "veryfast";
        break;
    case 2:
        pres = "fast";
        break;
    case 3:
        pres = "medium";
        break;
    case 4:
        pres = "slow";
        break;
    case 5:
        pres = "veryslow";
        break;
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
    codec_context->qcompress = qcompress;
    //codec_context->active_thread_type = 1;


    int ret = av_opt_set(codec_context->priv_data, "preset", pres, 0);
    //ret = av_opt_set(codec_context->priv_data, "tune", "fastdecode", 0);
    ret = av_opt_set(codec_context->priv_data, "tune", "zerolatency", 0);
	//This parameters aimed to boost decoding at cost of encoded size
    ret = av_opt_set(codec_context->priv_data, "coder", "0", 0);

	

    av_opt_set_int(codec_context->priv_data, "crf", crf, 0);

    ret = avcodec_open2(codec_context, codec, nullptr);

    if (ret != 0)
    {
        return false;
    }

    if (frame == nullptr) {
        frame = Extensions::init_frame(_params->width, _params->height, _params->dst_format, true);
        bgra_frame = Extensions::init_frame(_params->width, _params->height, _params->src_format, false);
        bgra_frame->linesize[0] = _params->stride;
        ctx = sws_alloc_context();
        packet = av_packet_alloc();
    }
    is_open = true;
    return true;
}

int ColorEncoder::Encode(uint8_t* buffer_in)
{
    //Just double checking is encoder been initialised
    if (!is_open) return -1;
	
    bgra_frame->data[0] = buffer_in;
    RGBAtoYUV(bgra_frame, frame);

    int ret = 0;

    if (!Write()) ret = -2;
    if (ret == 0 && !GetPacket()) ret = -1;

    return ret;
}

bool ColorEncoder::Write()
{
    if (!is_open)
        return false;

    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0)
    {
        return false;
    }

    return true;
}

bool ColorEncoder::GetPacket()
{
    int ret;

    if (packet == nullptr) packet = av_packet_alloc();

    ret = avcodec_receive_packet(codec_context, packet);
    if (ret != 0)
        return false;

    return true;
}

void ColorEncoder::RGBAtoYUV(AVFrame* in_frame, AVFrame* frame)
{
    ctx = sws_getCachedContext(ctx, in_frame->width, in_frame->height,
        _params->src_format,
        in_frame->width, in_frame->height,
        _params->dst_format,
        0, 0, 0, 0);

    int depth = sws_scale(ctx, in_frame->data, in_frame->linesize, 0, in_frame->height, frame->data, frame->linesize);
}

void ColorEncoder::Dispose()
{
    avcodec_close(codec_context);
    avcodec_free_context(&codec_context);
    sws_freeContext(ctx);
	
    delete _params;
    if (packet != nullptr) av_packet_free(&packet);
    if (frame != nullptr) av_frame_free(&frame);
    if (bgra_frame != nullptr) av_frame_free(&bgra_frame);
}

uint8_t* ColorEncoder::ExtractResults(uint32_t& data_size)
{
    auto result = new uint8_t[packet->size];
    memcpy(result, packet->data, packet->size);
    data_size = packet->size;
    return result;
}

