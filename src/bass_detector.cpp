#include "bass_detector.h"

bass_detector::bass_detector(): m_bp(bessel()) {};

bool bass_detector::initialise(int step_size, int window_size) {
	if (step_size != window_size) {
		std::cout << "Error bass detector incorrectly initialized" << std::endl;
		return false;
	}
	m_step_size = step_size;
	m_window_size = window_size;
	m_bp.set_coefs(44110, 100, 0.4, 0);	// doesn't do anything, parameters are hard coded in filter
	m_buffer = new float[window_size*2]; // we receive mono so need space for stereo
	return true;
}
	
Vamp::Plugin::FeatureSet bass_detector::process(const float *const *inputBuffers, Vamp::RealTime timestamp) {
	double bass_temp = 0;
	for (int i = 0; i < m_window_size*2; i++) {
		m_buffer[i] = inputBuffers[0][i/2];	// back to stereo
	}
	m_bp.process_samples(m_buffer, m_window_size*2);
	for (int i = 0; i < (m_window_size*2)-1; i=i+2) {
		bass_temp += std::abs((m_buffer[i] + m_buffer[i+1])*0.5);
	}
	m_bass.push_back(bass_temp);

	Vamp::Plugin::FeatureSet returnFeatures;
	return returnFeatures;
}

std::vector<double> bass_detector::get_bass_content() {
	return m_bass;
}