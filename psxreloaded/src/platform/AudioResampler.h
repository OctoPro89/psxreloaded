#pragma once

#include <core/Types.h>

class AudioResampler
{
public:
    void Init(unsigned int inputRate, unsigned int outputRate);
    // STATIC resampling
    unsigned int Process(const int16_t* input, unsigned int inputFrames, int16_t* output, unsigned int outputCapacityFrames);
private:
    double m_position = 0.0;
    double m_step = 1.0;

    int16_t m_previous[2] = {};
    bool m_hasPrevious = false;
};