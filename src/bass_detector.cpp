#include "bass_detector.h"
#include <algorithm>
#include <cmath>

bass_detector::bass_detector() : m_bp(bessel()){};

bool bass_detector::initialise(int step_size, int window_size) {
  if (step_size != window_size) {
    std::cout << "Error bass detector incorrectly initialized" << std::endl;
    return false;
  }
  m_step_size = step_size;
  m_window_size = window_size;
  m_bp.set_coefs(44110, 100, 0.4,
                 0); // doesn't do anything, parameters are hard coded in filter
  m_buffer =
      new float[window_size * 2]; // we receive mono so need space for stereo
  return true;
}

Vamp::Plugin::FeatureSet
bass_detector::process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp) {
  for (int i = 0; i < m_window_size * 2; i++) {
    m_buffer[i] = inputBuffers[0][i / 2]; // back to stereo
  }
  m_vol.push_back(get_rms());
  m_bp.process_samples(m_buffer, m_window_size * 2);

  m_bass.push_back(get_rms());

  Vamp::Plugin::FeatureSet returnFeatures;
  return returnFeatures;
}

double bass_detector::get_rms() {
  double rms = 0;
  for (int i = 0; i < (m_window_size * 2) - 1; i = i + 2) {
    rms += pow(std::abs((m_buffer[i] + m_buffer[i + 1]) * 0.5), 2);
  }
  return pow(rms / (m_window_size * 2), 0.5);
}

std::vector<double> bass_detector::get_bass_content() { return m_bass; }

double bass_detector::get_vol() {
  auto vol_max = std::max_element(m_vol.begin(), m_vol.end());
  double threshold = 0.8 * (*vol_max);
  double total_vol = 0;
  int num_vols = 0;
  for (auto vol : m_vol) {
    if (vol > threshold) {
      total_vol += vol;
      num_vols++;
    }
  }
  return total_vol / num_vols;
}