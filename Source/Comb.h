// Comb.h
// Spring 2021

#pragma once
#include <deque> // std::deque

// low-pass comb filter
class Comb
{
public:
    Comb(unsigned delay, double g); // filter delay in samples, lowpass parameter g
    
    void Reset(); // reset filer
    
    void SetDelay(unsigned samples);    // set delay in samples
    void SetLowPassG(double new_g);     // set lowpass parameter g
    void SetGainConstant(double new_R); // set comb gain constant
    void SetZeroFreqGain(double gain);  // set zero-frequency loop gain
    unsigned GetDelay();
    double GetLowPassG();
    double GetGainConstant();
    double GetZeroFreqGain();
    
    float operator()(float x); // filter signal value x
private:
    std::deque<double> delX; // x delays [t-(L+1), t-1]
    std::deque<double> delY; // y delays [t-L, t-1]
    
    double zfGain; // zero-frequency loop gain
    double g;      // lowpass coefficient
    double R;      // gain constant
    unsigned L;    // delay (samples)
    
};
