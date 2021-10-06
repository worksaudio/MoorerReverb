// AllPass.h
// Spring 2021

#pragma once
#include <deque> // std::deque

// all-pass filter
class AllPass
{
public:
    AllPass(double a, unsigned delay); // coefficient a, delay in samples
    
    void Reset(); // reset allpass filter
    
    void SetCoefficient(double new_a); // set allpass coefficient a
    void SetDelay(unsigned samples);  // set allpass delay
    double GetCoefficient();
    unsigned GetDelay();
    
    float operator()(float x);
private:
    double a; // all-pass coefficient
    unsigned m; // delay in samples
    
    std::deque<double> delX, delY; // delays x[t-m], y[t-m]
};
