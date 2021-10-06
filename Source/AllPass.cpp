// AllPass.cpp
// Spring 2021

#include "AllPass.h"

// AllPass Constructor
AllPass::AllPass(double a, unsigned delay) : a(a), m(delay)
{
    Reset();
}

// reset allpass filter
void AllPass::Reset()
{
    delX.clear(); // clear x delays
    delY.clear(); // clear y delays
    
    for (unsigned i = 0; i < m; ++i) {
        delX.push_back(0.0);
        delY.push_back(0.0);
    }
}

void AllPass::SetCoefficient(double new_a)
{
    a = new_a;
}

void AllPass::SetDelay(unsigned samples)
{
    m = samples;
}


double AllPass::GetCoefficient()
{
    return a;
}

unsigned AllPass::GetDelay()
{
    return m;
}

// returns filtered signal value
float AllPass::operator()(float x)
{
    double xm = delX.front();
    double ym = delY.front();
    delX.pop_front();
    delY.pop_front();
    
    // y[t] = x[t-m] + a * (x[t] - y[t-m])
    double y = xm + a * (x - ym);
    
    delX.push_back(x);
    delY.push_back(y);
    
    return static_cast<float>(y);
    }
