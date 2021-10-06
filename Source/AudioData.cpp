// AudioData.cpp
// Spring 2021

#include "AudioData.h"
#include <cmath>
#include <stdexcept>
#include <fstream>
#include <cstring>

using namespace std;

const float max16 = static_cast<float>((1 << 15) - 1);
const float max8 = static_cast<float>((1 << 7) - 1);

// audio data constructor
AudioData::AudioData(unsigned nframes, unsigned R, unsigned nchannels) :
frame_count(nframes),
sampling_rate(R),
channel_count(nchannels)
{
    fdata.resize(nframes * nchannels);
    unsigned size = nframes * nchannels;
    for (unsigned i = 0; i < size; ++i) {
        fdata[i] = 0.0;
    }
}

// contructs an audio data object from existing wave file
AudioData::AudioData(const char *fname)
{
    // input file stream
    fstream in(fname, ios_base::binary | ios_base::in);
    
    if (!in)
        throw runtime_error("unable to open file");
    
    
    struct { char label[4]; unsigned size; } chunk_head;
    char wave_tag[4];
    
    // reads in file label, size, and file type tag
    in.read(reinterpret_cast<char*>(&chunk_head), 8);
    in.read(wave_tag, 4);
    
    // checks for valid wave file label and tag
    if (!in || strncmp(chunk_head.label, "RIFF", 4) != 0 || strncmp(wave_tag, "WAVE", 4) != 0)
        throw runtime_error("not a valid WAVE file");
    
    
    // looks for format chunk
    in.read(reinterpret_cast<char*>(&chunk_head), 8);
    while (in && strncmp(chunk_head.label, "fmt ", 4) != 0)
    {
        in.seekg(chunk_head.size, ios_base::cur);
        in.read(reinterpret_cast<char*>(&chunk_head), 8);
    }
    
    if (!in)
        throw runtime_error("WAVE file missing format chunk");
    
    // gets format chunk info
    char format[16];
    in.read(format, 16);
    
    if (*reinterpret_cast<unsigned short*>(format) != 1)
        throw runtime_error("invalid WAVE file: compressed audio data");
    
    channel_count = static_cast<unsigned>(*reinterpret_cast<unsigned short*>(format + 2));
    sampling_rate = *reinterpret_cast<unsigned*>(format + 4);
    unsigned int bytes_per_second = *reinterpret_cast<unsigned*>(format + 8);
    unsigned short bytes_per_sample = *reinterpret_cast<unsigned short*>(format + 12);
    unsigned short bits = *reinterpret_cast<unsigned short*>(format + 14);
    
    // checks for valid bit resolution
    if (bits != 8 && bits != 16)
        throw runtime_error("invalid WAVE file: invalid bit resolution");
    
    // checks for consistent bytes-per-sample
    if (bytes_per_sample != channel_count * (bits / 8))
        throw runtime_error("invalid WAVE file: bytes-per-sample not consistent");
    
    // checks for consistent bytes-per-second
    if (bytes_per_second != sampling_rate * bytes_per_sample)
        throw runtime_error("invalid WAVE file: bytes-per-second not consistent");
    
    in.seekg(chunk_head.size - 16, ios_base::cur);
    
    // looks for data chunk
    in.read(reinterpret_cast<char*>(&chunk_head), 8);
    while (in && strncmp(chunk_head.label, "data", 4) != 0)
    {
        in.seekg(chunk_head.size, ios_base::cur);
        in.read(reinterpret_cast<char*>(&chunk_head), 8);
    }
    
    if (!in)
        throw runtime_error("WAVE file missing data chunk");
    
    // calculates frame count
    frame_count = static_cast<unsigned>((8 * chunk_head.size) / (channel_count * bits));
    
    // resizes fdata vector
    unsigned size = frame_count * channel_count;
    fdata.resize(size);
    
    unsigned bytes = bits / 8;
    
    // reads in data
    short *data = new short[size];
    in.read(reinterpret_cast<char*>(data), bytes * size);
    
    if (!in)
        throw runtime_error("WAVE file missing data");
    
    // if 16 bit
    if (bits == 16)
    {
        for (unsigned i = 0; i < size; i++)
            fdata[i] = static_cast<float>(data[i] / float(1<<15));
    }
    
    // if 8 bit
    else if (bits == 8)
    {
        unsigned char *data8 = reinterpret_cast<unsigned char*>(data);
        for (unsigned i = 0; i < size; i++)
            fdata[i] = (static_cast<float>(data8[i] - 128)) / 128;
    }
    
    delete[] data;
}

// returns sample value (retrieves)
float AudioData::sample(unsigned frame, unsigned channel) const
{
    unsigned index = frame * channel_count + channel;
    return fdata[index];
}

// returns reference to sample value (sets)
float& AudioData::sample(unsigned frame, unsigned channel)
{
    unsigned index = frame * channel_count + channel;
    return fdata[index];
}

AudioData& AudioData::operator=(AudioData & d)
{
    fdata = d.fdata;
    frame_count = d.frame_count;
    sampling_rate = d.sampling_rate;
    channel_count = d.channel_count;
    
    
    return *this;
}

// normalizes audio data
void normalize(AudioData &ad, float dB)
{
    unsigned frames = ad.frames();
    unsigned channels = ad.channels();
    float targetmax = std::abs(pow(10, static_cast<float>(dB/20.0f)));
    float currentmax = 0.0f;
    
    // removes dc offset for each channel
    for (unsigned i = 0; i < channels; i++)
    {
        float offset = 0.0f;
        
        // adds all frame values in channel
        for (unsigned j = 0; j < frames; j++)
            offset += ad.sample(j,i);
        
        // divides by num frames to get dc offset for channel
        offset = offset / frames;
        
        // removes dc offset from data
        for (unsigned j = 0; j < frames; j++)
        {
            ad.sample(j,i) -= offset;
            
            float value = std::abs(ad.sample(j,i));
            
            if (value >= currentmax)
                currentmax = value;
        }
    }
    
    // calculates gain factor
    float m = targetmax / currentmax;
    
    // scales each data value
    for (unsigned i = 0; i < channels; i++)
        for (unsigned j = 0; j < frames; j++)
            ad.sample(j,i) *= m;
}

bool waveWrite(const char *fname, const AudioData &ad, unsigned bits)
{
    if (!fname || ad.data() == nullptr)
        return false;
    
    if (bits != 8 && bits != 16)
        return false;
    
    unsigned frames = ad.frames(), channels = ad.channels(), rate = ad.rate();
    unsigned bytes = bits / 8;
    unsigned size = frames * channels * bytes;
    
    // header for wave file
    char header[44];
    strncpy(header + 0, "RIFF", 4);
    *reinterpret_cast<unsigned*>(header + 4) = 36 + size;
    strncpy(header + 8, "WAVE", 4);
    strncpy(header + 12, "fmt ", 4);
    *reinterpret_cast<unsigned*>(header + 16) = 16;
    *reinterpret_cast<unsigned short*>(header + 20) = 1;
    *reinterpret_cast<unsigned short*>(header + 22) = channels;
    *reinterpret_cast<unsigned*>(header + 24) = rate;
    *reinterpret_cast<unsigned*>(header + 28) = rate * channels * bytes;
    *reinterpret_cast<unsigned short*>(header + 32) = channels * bytes;
    *reinterpret_cast<unsigned short*>(header + 34) = bits;
    strncpy(header + 36, "data", 4);
    *reinterpret_cast<unsigned*>(header + 40) = size;
    
    // output file stream
    fstream out(fname, ios_base::binary | ios_base::out);
    
    // file failed to open
    if (!out.is_open())
        return false;
    
    // writes wave header to file
    out.write(reinterpret_cast<char*>(&header), sizeof(header));
    
    char *data = new char[size]; // data to be written to file
    unsigned framecount = frames * channels; // frame count
    
    // if 16 bit
    if (bits == 16)
    {
        // writes data as short
        short *data16 = reinterpret_cast<short*>(data);
        for (unsigned i = 0; i < framecount; i++)
            data16[i] = static_cast<short>(max16 * ad.data()[i]);
    }
    
    // if 8 bit
    else if (bits == 8)
    {
        // writes data as unsigned char
        unsigned char *data8 = reinterpret_cast<unsigned char*>(data);
        for (unsigned i = 0; i < framecount; i++)
            data8[i] = static_cast<unsigned char>(max8 * ad.data()[i] + 128);
    }
    
    // writes data to file, destroys data
    out.write(reinterpret_cast<char*>(data), size);
    delete[] data;
    
    return true; // wave write successful
}







