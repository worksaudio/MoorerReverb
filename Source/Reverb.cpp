// Reverb.cpp
// Spring 2021


#include <utility> // std::pair
#include <cmath>   // std::round
#include "Reverb.h"

//==============================================================================
// Moorer Default Values
const unsigned NumCombs = 6;
const unsigned fs_def = 44100;
const unsigned k_def = 0.9;
const unsigned m_def = 6;
const double   a_def = 0.7;

const unsigned Delays[NumCombs] = {50, 56, 61, 68, 72, 78};          // delays in ms
const float    G25[NumCombs] = {0.24, 0.26, 0.28, 0.29, 0.30, 0.32}; // 25kHz g values
const float    G50[NumCombs] = {0.46, 0.48, 0.50, 0.52, 0.53, 0.55}; // 50kHz g values
const unsigned Diff = 25000; // 50kHz - 25 kHz

// min, max
typedef std::pair<double,double> Range;

const Range mmR(0,1);  // [0,1)
const Range mmG(0,1);  // [0,1)
const Range mmZF(0,1); // [0,1)

// converts integer to decimal
double ConvertToDecimal(unsigned value, unsigned max)
{
    return static_cast<double>(value / max);
}

// converts decimal to integer
unsigned ConvertToInt(double value, unsigned max)
{
    return static_cast<unsigned>(value * max);
}

//==============================================================================
// Helpers

// returns interpolated g value
float GetParamG(unsigned rate, unsigned index)
{
    if (index < 0 || index > 5)
        return 0.0;
    
    return rate * ((G50[index] - G25[index]) / Diff);
}

// returns delay in samples
// rate = sampling rate, time = time in ms
unsigned GetNumSamples(unsigned rate, unsigned time)
{
    return rate * time / 1000;
}

// returns delay in ms
unsigned GetDelayMS(unsigned rate, unsigned samples)
{
    unsigned delay = std::round(1000.0 * samples / rate);
    return delay;
}

//=============================================================================
// Moorer Reverb Filter
Reverb::Reverb() : fs(fs_def), dry(k_def), wet(1.0 - dry), combs(), ap(a_def, GetNumSamples(fs, m_def))
{
    // initialize comb filters
    for (int i = 0; i < NumCombs; ++i) {
        combs.push_back(Comb(GetNumSamples(fs, Delays[i]), GetParamG(fs, i)));
    }
    
    Reset();
}

// reset filter
void Reverb::Reset()
{
    // reset comb filters
    for (Comb & c : combs) {
        c.Reset();
    }
    
    // reset allpass filter
    ap.Reset();
}

// set sampling rate
void Reverb::SetSamplingRate(unsigned rate)
{
    unsigned old = fs;
    fs = rate;

    // reset comb params
    for (Comb & c : combs) {
        c.SetDelay(c.GetDelay() * fs / old);
    }
}

// set dry percentage K
void Reverb::SetDryPercetage(unsigned new_K)
{
    dry = new_K / 100.0;
    wet = 1.0 - dry;
}

// returns sampling rate
unsigned Reverb::GetSamplingRate()
{
    return fs;
}

// returns dry percentage K
unsigned Reverb::GetDryPercentage()
{
    return dry * 100;
}

// sets allpass delay
void Reverb::SetAllPassDelay(unsigned delay)
{
    ap.SetDelay(GetNumSamples(fs, delay));
}

// sets allpass coefficient a
void Reverb::SetAllPassCoeff(double a)
{
    ap.SetCoefficient(a);
}

// returns allpass delay
float Reverb::GetAllPassDelay()
{
    return GetDelayMS(fs, ap.GetDelay());
}

// returns allpass coefficient a
float Reverb::GetAllPassCoeff()
{
    return ap.GetCoefficient();
}

// sets comb delay
void Reverb::SetCombDelay(unsigned delay, unsigned i)
{
    combs[i].SetDelay(GetNumSamples(fs, delay));
}

// sets comb lowpass param g
void Reverb::SetCombLowPassCoeff(double g, unsigned i)
{
    if (g < mmG.first) {
        combs[i].SetGainConstant(combs[i].GetZeroFreqGain());
        return;
    }
    else if (g >= mmG.second) {
        g = 0.99;
    }

    // update g and R
    combs[i].SetLowPassG(g);
    combs[i].SetGainConstant(combs[i].GetZeroFreqGain() * (1.0 - g));
    
}

// sets comb gain const R
void Reverb::SetCombGainConstant(double R, unsigned i)
{
    // R = 0 => zf = 0
    if (R < mmR.first) {
        combs[i].SetGainConstant(0.0);
        combs[i].SetZeroFreqGain(0.0);
        return;
    }
    else if (R >= mmR.second) {
        R = 0.99;
    }

    // update g and R
    combs[i].SetGainConstant(R);
    combs[i].SetLowPassG(1.0 - (R / combs[i].GetZeroFreqGain()));
}

// sets comb zero frequency gain
void Reverb::SetCombZeroFreqGain(double zf, unsigned i)
{
    if (zf < mmZF.first) {
        combs[i].SetGainConstant(0.0);
        combs[i].SetZeroFreqGain(0.0);
        return;
    }
    else if (zf >= mmZF.second) {
        zf = 0.99;
    }
    
    // update zerofreq constant and R
    combs[i].SetZeroFreqGain(zf);
    combs[i].SetGainConstant(zf * (1.0 - combs[i].GetLowPassG()));
}


// returns comb delay
unsigned Reverb::GetCombDelay(unsigned i)
{
    return GetDelayMS(fs, combs[i].GetDelay());
}

// returns comb lowpass g
double Reverb::GetCombLowPassCoeff(unsigned i)
{
    return combs[i].GetLowPassG();
}

// rturns comb gain constant R
double Reverb::GetCombGainConstant(unsigned i)
{
    return combs[i].GetGainConstant();
}

double Reverb::GetCombZeroFreqGain(unsigned i)
{
    return combs[i].GetZeroFreqGain();
}

// returns filtered signal value
float Reverb::operator()(float x)
{
    // send signal through parallel comb filters
    double temp = 0.0f;
    for (Comb & c : combs) {
        temp += c(x);
    }
    
    // send parallel comb output through allpass filter
    temp = ap(temp);
    return temp * wet + x * dry;
}
