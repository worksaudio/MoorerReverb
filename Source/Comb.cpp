// Comb.cpp
// Spring 2021

#include "Comb.h"

const double zf_def = 0.83;

Comb::Comb(unsigned delay, double g) : delX(), delY(), zfGain(zf_def), g(g), R(zf_def*(1 - g)), L(delay)
{
    Reset();
}

void Comb::Reset()
{
    delX.clear(); // clear x delays
    delY.clear(); // clear y delays
    
    for (unsigned i = 0; i < L; ++i) {
        delX.push_back(0.0);
        delY.push_back(0.0);
    }
    
    delX.push_back(0.0);
}

void Comb::SetLowPassG(double new_g)
{
    g = new_g;
}

void Comb::SetGainConstant(double new_R)
{
    R = new_R;
}

void Comb::SetDelay(unsigned samples)
{
    L = samples;
}

void Comb::SetZeroFreqGain(double gain)
{
    zfGain = gain;
}

double Comb::GetLowPassG()
{
    return g;
}

double Comb::GetGainConstant()
{
    return R;
}

unsigned Comb::GetDelay()
{
    return L;
}

double Comb::GetZeroFreqGain()
{
    return zfGain;
}

// returns filtered signal value
float Comb::operator()(float x)
{
    double xl1 = delX.front(); // x[t-(L+1)]
    delX.pop_front();
    
    double xl = delX.front();  // x[t-L]
    double yl = delY.front();  // y[t-L]
    double y1 = delY.back();   // y[t-1]
    delY.pop_front();
    
    // y[t] = x[t-L] + g * (y[t-1] - x[t-(L+1)]) + R * y[t-L]
    double y = xl + g * (y1 - xl1) + R * yl;
    
    // store current values
    delY.push_back(y);
    delX.push_back(x);
    return y;
    }
