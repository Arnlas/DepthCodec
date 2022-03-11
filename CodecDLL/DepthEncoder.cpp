#include "DepthEncoder.h"

bool DepthEncoder::Init(const int preset, const int source_format, const int dest_format,
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
    codec_context->qmin = 1;
    codec_context->qmax = 99;

    int ret = av_opt_set(codec_context->priv_data, "preset", pres, 0);
    ret = av_opt_set(codec_context->priv_data, "tune", "zerolatency", 0);
    ret = av_opt_set(codec_context->priv_data, "profile", "high444", 0);
    ret = av_opt_set(codec_context->priv_data, "threads", "16", 0);

    av_opt_set_int(codec_context->priv_data, "crf", crf, 0);

    ret = avcodec_open2(codec_context, codec, nullptr);

    if (ret != 0)
    {
        return false;
    }

    is_open = true;
    return true;
}

int DepthEncoder::Encode(uint8_t* buffer_in)
{
    //Just double checking is encoder been initialised
    if (!is_open) return -1;

    //Initialise frame if it is not present
    if (frame == nullptr) frame = Extensions::init_frame(_params->width, _params->height, _params->dst_format, true);

    //Convert Depth to YUV and assign data to frame
    int size = _params->width * _params->height;
    DepthToYUV((uint16_t*)(void*)buffer_in, size, frame);

    if (!Write()) return -2;
    if (!GetPacket()) return -1;

    return 0;
}

uint8_t* DepthEncoder::ExtractResult(uint32_t& high_size, uint32_t& data_size)
{
    high_size = lossless_bits.size();
    data_size = packet->size;
	
    auto result = new uint8_t[lossless_bits.size() + packet->size];

    memcpy(result, lossless_bits.data(), lossless_bits.size());
    memcpy(result + lossless_bits.size(), packet->data, packet->size);

    return result;
}

void DepthEncoder::Dispose()
{
    avcodec_close(codec_context);
    avcodec_free_context(&codec_context);

    if (packet != nullptr) av_packet_free_side_data(packet);
    if (packet != nullptr) av_packet_free(&packet);
    if (frame != nullptr && frame->data != nullptr) av_freep(frame->data);
    if (frame != nullptr) av_frame_free(&frame);
    if (!lossless_bits.empty()) lossless_bits.clear();

    delete _params;
}


bool DepthEncoder::Write()
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

bool DepthEncoder::GetPacket()
{
    if (packet == nullptr) packet = av_packet_alloc();

    int ret = avcodec_receive_packet(codec_context, packet);
    if (ret != 0)
        return false;

    return true;
}

void DepthEncoder::DepthToYUV(uint16_t* depth, int size, AVFrame* yuv)
{
    auto y = new uint16_t[size];
    auto u = new uint16_t[size];

    const uint16_t higher_const = 4095;
    const uint16_t lower_constant = 3;

    vector<uint8_t> high_lossless(size * 2 / 8);
    int j = 0;
    uint8_t val = 0;

    for (int i = 0; i < size; i++)
    {
        int id = i % 4 * 2;
        if (id == 0 && i > 0)
        {
            high_lossless[j] = val;
            val = 0;
            j++;
        }

        val |= (depth[i] >> 6 & 192) >> id;


        y[i] = (depth[i] & higher_const) >> 2;
        u[i] = (depth[i] & lower_constant);
    }

    lossless_bits = Extensions::RLEEncoding(&high_lossless);

    memcpy(yuv->data[0], y, size * 2);
    memcpy(yuv->data[1], u, size * 2);

    high_lossless.clear();
    delete[] y;
    delete[] u;
}