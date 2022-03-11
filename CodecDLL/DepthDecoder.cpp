#include "DepthDecoder.h"

bool DepthDecoder::Init(const char* preset, const int source_format, const int dest_format,
    const uint32_t crf, const uint32_t width, const uint32_t height,
    const uint32_t stride, const double fps, const uint32_t bitrate, uint32_t gop_size)
{
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
    codec_context->qmin = 1;
    codec_context->qmax = 99;

    int ret = av_opt_set(codec_context->priv_data, "threads", "16", 0);


    ret = avcodec_open2(codec_context, codec, nullptr);

    if (ret != 0) return false;

    if (frame == nullptr) {
        frame = Extensions::init_frame(_params->width, _params->height, _params->dst_format, true);
        packet = av_packet_alloc();
    }

    is_open = true;
    return true;
}

void DepthDecoder::Dispose()
{
    if (packet != nullptr) av_packet_free(&packet);
    //av_freep(frame->data);
    if (frame != nullptr) av_frame_free(&frame);

    avcodec_close(codec_context);
    avcodec_free_context(&codec_context);

    delete[] decoded_buffer;
    delete _params;
}


int DepthDecoder::Decode(uint8_t* data, const uint32_t high_size, const uint32_t low_size)
{
    uint8_t* high_data = new uint8_t[high_size];
    uint8_t* low_data = new uint8_t[low_size];
    memcpy(high_data, data, high_size);
    memcpy(low_data, data + high_size, low_size);
	
    packet->data = low_data;
    packet->size = low_size;
	
    //Just double checking is encoder been initialised
    if (!is_open) {
        delete[] high_data;
        delete[] low_data;
        return -1;
    }

    //Initialise frame if it is not present
    if (avcodec_send_packet(codec_context, packet) != 0) {
        delete[] high_data;
        delete[] low_data;
        return -2;
    }

	
    if (avcodec_receive_frame(codec_context, frame) != 0) {
        delete[] high_data;
        delete[] low_data;
        return -1;
    }

    //Convert Depth to YUV and assign data to frame
    YUVToDepth(high_data, high_size, frame, _params->width * _params->height);

    delete[] high_data;
    delete[] low_data;
    return 0;
}

uint8_t* DepthDecoder::ExtractResults()
{
	auto result = new uint8_t[_params->width * _params->height * sizeof(uint16_t)];
    memcpy(result, decoded_buffer, _params->width * _params->height * sizeof(uint16_t));
    cout << "Returning decoded data" << endl;
    return result;
}


void DepthDecoder::YUVToDepth(uint8_t* lossless, int lossless_size, AVFrame* frame, int size)
{
    auto depth = new uint16_t[size];

    uint16_t* y = (uint16_t*)(void*)frame->data[0];
    uint16_t* u = (uint16_t*)(void*)frame->data[1];

    vector<uint8_t> decompressed = Extensions::RLEDecoding(lossless, lossless_size);
    int j = 0;


    for (int i = 0; i < size; i++)
    {
        uint16_t val = 0;

        int id = i % 4 * 2;
        if (id == 0 && i > 0)
        {
            j++;
        }

        val |= (decompressed[j] << id & 192) << 6;

        depth[i] = val | y[i] << 2 | u[i];
    }

    delete[] decoded_buffer;
	decoded_buffer = new uint8_t[_params->width * _params->height * sizeof(uint16_t)];
    memcpy(decoded_buffer, depth, size * sizeof(uint16_t));

    delete[] depth;
    decompressed.clear();
}