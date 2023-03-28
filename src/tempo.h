#include "ring_buffer.h"
#include "track.h"

extern "C" {
#include <libsamplerate/include/samplerate.h>
}

class tempo {
private:
  ring_buffer m_ring_buffer;
  float m_ratio;
  track *m_track;
  float *m_input_samples;
  float *m_output_samples;
  SRC_DATA m_data;
  SRC_STATE *m_state;

public:
  tempo();
  int read(float *data_ptr, int num_samples);
  int fill_output_buffer();
  void set_tempo_ratio(float tempo_ratio);
  void load(track *track);
  int init();
};