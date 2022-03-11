#include "Extension.h"


AVFrame* Extensions::init_frame(int width, int height, AVPixelFormat format, bool init_memory)
{
    AVFrame* frame = av_frame_alloc();

    frame->format = format;
    frame->width = width;
    frame->height = height;

    if (init_memory) av_image_alloc(frame->data, frame->linesize, width, height, format, 32);

    return frame;
}

vector<uint8_t> Extensions::RLEEncoding(vector<uint8_t>* data)
{
    vector<uint8_t> compressed;


    for (uint32_t i = 0; i < data->size(); i++)
    {
        uint8_t count = 1;
        uint8_t val = data->at(i);

        while (i < data->size() - 1 && count + 1 <= 255 && data->at(i + 1) == val)
        {
            count++;
            i++;
        }

        compressed.push_back(count);
        compressed.push_back(val);
    }

    return compressed;
}

vector<uint8_t> Extensions::RLEDecoding(uint8_t* data, int size)
{
    vector<uint8_t> uncompressed;

    for (int i = 0; i < size; i += 2)
    {
        int count = data[i];
        int value = data[i + 1];

        for (int j = 0; j < count; j++) uncompressed.push_back(value);
    }

    return uncompressed;
}

uint8_t* Extensions::LZ4Encode(uint8_t* data, uint32_t size, uint32_t& compressed_size)
{
    int max_compressed_size = LZ4_compressBound(size);
    char* compressed = new char[max_compressed_size];
    compressed_size = LZ4_compress_default((const char*) data, compressed, size, max_compressed_size);
    return (uint8_t*)compressed;
}

uint8_t* Extensions::ZSTDEncode(uint32_t level, uint8_t* data, uint32_t size, uint32_t& compressed_size)
{
    int max_compressed_size = ZSTD_compressBound(size);//LZ4_compressBound(size);
    char* compressed = new char[max_compressed_size];
    compressed_size = ZSTD_compress(compressed, max_compressed_size, data, size, level);
    return (uint8_t*)compressed;
}

uint8_t* Extensions::LZ4Decode(uint8_t* data, uint32_t compressed_size, uint32_t expected_size)
{
    char* uncompressed = new char[expected_size];
    LZ4_decompress_safe((const char*)data, uncompressed, compressed_size, expected_size);
	return (uint8_t*) uncompressed;
}

uint8_t* Extensions::ZSTDDecode(uint8_t* data, uint32_t compressed_size, uint32_t expected_size)
{
    char* uncompressed = new char[expected_size];
    ZSTD_decompress(uncompressed, expected_size, data, compressed_size);
    return (uint8_t*)uncompressed;
}

extern "C" __declspec(dllexport) void free_ptr(uint8_t * ptr)
{
    delete[] ptr;
}

extern "C" __declspec(dllexport) uint8_t * lz4_encode(uint8_t * data, uint32_t size, uint32_t & compressed_size)
{
    return Extensions::LZ4Encode(data, size, compressed_size);
}

extern "C" __declspec(dllexport) uint8_t * lz4_decode(uint8_t * data, uint32_t compressed_size, uint32_t expected_size)
{
    return Extensions::LZ4Decode(data, compressed_size, expected_size);
}

extern "C" __declspec(dllexport) uint8_t * zstd_encode(uint32_t level, uint8_t * data, uint32_t size, uint32_t& compressed_size)
{
    return Extensions::ZSTDEncode(level, data, size, compressed_size);
}

extern "C" __declspec(dllexport) uint8_t * zstd_decode(uint8_t * data, uint32_t compressed_size, uint32_t expected_size)
{
    return Extensions::ZSTDDecode(data, compressed_size, expected_size);
}