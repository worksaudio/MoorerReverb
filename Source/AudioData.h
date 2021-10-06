// AudioData.h
// Spring 2021

#ifndef AUDIODATA_H
#define AUDIODATA_H
#include <vector>

class AudioData {
public:
    AudioData(unsigned nframes, unsigned R=44100, unsigned nchannels=1);
    float sample(unsigned frame, unsigned channel=0) const;
    float& sample(unsigned frame, unsigned channel=0);
    
    float* data(void)             { return &fdata[0]; }
    const float* data(void) const { return &fdata[0]; }
    unsigned frames(void) const   { return frame_count; }
    unsigned rate(void) const     { return sampling_rate; }
    unsigned channels(void) const { return channel_count; }
    
    AudioData(const char *fname);
    
    // assignment
    AudioData& operator=(AudioData & d);
    
    size_t size() const { return fdata.size(); }
    
private:
    std::vector<float> fdata;
    unsigned frame_count,
    sampling_rate,
    channel_count;
};

void normalize(AudioData &ad, float dB=0);
bool waveWrite(const char *fname, const AudioData &ad, unsigned bits=16);


#endif


