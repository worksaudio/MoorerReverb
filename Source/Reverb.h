// Reverb.h
// Spring 2021

#pragma once

#include <vector>
#include "Comb.h"    // comb filter
#include "AllPass.h" // allpass filter

// Moorer Reverb Filter
class Reverb
{
public:
    Reverb();
    
    void Reset();
    
    // General Parameters
    void SetSamplingRate(unsigned rate); // set sampling rate (Hz)
    void SetDryPercetage(unsigned K);    // set dry percentage K
    unsigned GetSamplingRate();
    unsigned GetDryPercentage();
    
    // AllPass Parameters
    void SetAllPassDelay(unsigned delay); // allpass delay (ms)
    void SetAllPassCoeff(double a);       // allpass coefficient a
    float GetAllPassDelay();
    float GetAllPassCoeff();

    // Comb Parameters
    // i = comb #
    void SetCombDelay(unsigned delay, unsigned i);  // delay in ms
    void SetCombLowPassCoeff(double g, unsigned i); // lowpass coefficient g
    void SetCombGainConstant(double R, unsigned i); // gain constant R
    void SetCombZeroFreqGain(double gain, unsigned i);
    unsigned GetCombDelay(unsigned i);
    double GetCombLowPassCoeff(unsigned i);
    double GetCombGainConstant(unsigned i);
    double GetCombZeroFreqGain(unsigned i);
    
    float operator()(float x); // filter signal value x
private:
    unsigned fs; // sampling rate (Hz)
    double dry;   // dry percentage
    double wet; // wet percentage
    
    std::vector<Comb> combs; // lowpass comb filters
    AllPass ap; // allpass filter
};
