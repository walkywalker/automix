/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM Vamp Plugin Set

    Centre for Digital Music, Queen Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/* Edited @Matthew Walker 01/01/21 - add m_spec_diff
*/

#ifndef _ONSET_DETECT_PLUGIN_H_
#define _ONSET_DETECT_PLUGIN_H_

#include <vamp-plugin-sdk/vamp-sdk/Plugin.h>

class onset_detectorData;

class onset_detector : public Vamp::Plugin
{
public:
    onset_detector(float inputSampleRate);
    virtual ~onset_detector();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    InputDomain getInputDomain() const { return FrequencyDomain; }

    std::string getIdentifier() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    int getPluginVersion() const;
    std::string getCopyright() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(std::string) const;
    void setParameter(std::string, float);

    ProgramList getPrograms() const;
    std::string getCurrentProgram() const;
    void selectProgram(std::string program);

    size_t getPreferredStepSize() const;
    size_t getPreferredBlockSize() const;

    double extract_freq_component(double* magnitudes);
    OutputList getOutputDescriptors() const;

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    onset_detectorData *m_d;
    int m_dfType;
    float m_sensitivity;
    bool m_whiten;
    std::string m_program;
    static float m_preferredStepSecs;
    std::vector<double> m_spec_diff;
};


#endif
