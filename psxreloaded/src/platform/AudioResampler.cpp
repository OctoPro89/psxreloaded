#include <platform/AudioResampler.h>
#include <string.h>

void AudioResampler::Init(unsigned int inputRate, unsigned int outputRate)
{
    m_position = 0.0;

    // Advance this much through the input for every output frame
    m_step = (double)inputRate / (double)outputRate;

    m_previous[0] = 0;
    m_previous[1] = 0;
    m_hasPrevious = false;
}

/*
    The last sample of chunk 0 is needed when calculating the first output samples of chunk 1

    Without this:
    chunk 1:
     [0][1][2][3][4]

    chunk 2:
     [0][1][2][3][4]

    With it:
    chunk 1:
     [0][1][2][3][4]

    chunk 2:
     [4][5][6][7][8][9]

     ^^ Carries over the previous sample. Removes clicking and other junk
*/

unsigned int AudioResampler::Process(const int16_t* input, unsigned int inputFrames, int16_t* output, unsigned int outputCapacityFrames)
{
    if (m_step == 1.0)
    {
        unsigned int frames = inputFrames < outputCapacityFrames
            ? inputFrames
            : outputCapacityFrames;

        memcpy(output, input, frames * sizeof(int16_t) * 2);
        return frames;
    }

    unsigned int outputFrames = 0;

    while (outputFrames < outputCapacityFrames)
    {
        int16_t sample0[2];
        int16_t sample1[2];

        unsigned int index = (unsigned int)m_position;

        //
        // Get sample 0
        //
        if (index == 0)
        {
            if (!m_hasPrevious)
                break;

            sample0[0] = m_previous[0];
            sample0[1] = m_previous[1];
        }
        else
        {
            sample0[0] = input[(index - 1) * 2 + 0];
            sample0[1] = input[(index - 1) * 2 + 1];
        }

        //
        // Get sample 1
        //
        if (index >= inputFrames)
            break;

        sample1[0] = input[index * 2 + 0];
        sample1[1] = input[index * 2 + 1];


        double t = m_position - (double)index;

        for (int ch = 0; ch < 2; ch++)
        {
            double value =
                sample0[ch] +
                ((double)sample1[ch] - sample0[ch]) * t;

            output[outputFrames * 2 + ch] = (int16_t)value;
        }

        outputFrames++;

        m_position += m_step;
    }


    //
    // Save the final input frame for the next call.
    //
    m_previous[0] = input[(inputFrames - 1) * 2 + 0];
    m_previous[1] = input[(inputFrames - 1) * 2 + 1];
    m_hasPrevious = true;


    //
    // Keep only the fractional part.
    //
    m_position -= (double)(unsigned int)m_position;


    return outputFrames;
}