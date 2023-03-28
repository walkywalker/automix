#include <iostream>
#include <tempo.h>

#ifndef BUF_SIZE
#define BUF_SIZE 20480
#endif

#define DEFAULT_READ_SIZE 2048

tempo::tempo() : m_ring_buffer(BUF_SIZE) {
  m_ratio = 1.0;
  m_track = nullptr;
  m_input_samples = new float[DEFAULT_READ_SIZE];
  m_output_samples = new float[DEFAULT_READ_SIZE * 2];

  m_data.src_ratio = m_ratio;
  m_data.data_in = m_input_samples;
  m_data.data_out = m_output_samples;
  m_data.input_frames =
      DEFAULT_READ_SIZE / 2; // input frames no of 2 channel frames?
  m_data.output_frames = DEFAULT_READ_SIZE;
  m_data.end_of_input = 0;
}

int tempo::init() {
  int error;
  if ((m_state = src_new(0, 2, &error)) == NULL) {
    std::cout << "new failed!" << std::endl;
    return 1;
  } else {
    std::cout << "samplerate instance created" << std::endl;
    return 0;
  }
}

int tempo::read(float *data_ptr, int num_samples) {
  if (m_ring_buffer.read(data_ptr, num_samples) > 0) {
    return num_samples;
  } else if (fill_output_buffer() > 0) {
    std::cout << "tempo failed to fill output buffer" << std::endl;
    return 0;
  }
  if (m_ring_buffer.read(data_ptr, num_samples) == num_samples) {
    return num_samples;
  } else {
    std::cout << "tempo failed to read from track output buffer " << std::endl;
    return 0;
  }
}

int tempo::fill_output_buffer() {
  int error;
  while (m_ring_buffer.get_space() > (DEFAULT_READ_SIZE * m_ratio)) {
    int read_samples = m_track->read(m_input_samples, DEFAULT_READ_SIZE);
    if (read_samples != DEFAULT_READ_SIZE) {
      std::cout << "failed to read from track" << std::endl;
      return read_samples;
    } else {
      if ((error = src_process(m_state, &m_data))) {
        std::cout << "ERROR " << src_strerror(error) << std::endl;
        return 1;
      }
      m_ring_buffer.write(m_output_samples, m_data.output_frames_gen * 2);
    }
  }
  return 0;
}

void tempo::set_tempo_ratio(float tempo_ratio) {
  m_ratio = tempo_ratio;
  m_data.src_ratio = m_ratio;
}

void tempo::load(track *track) {
  m_track = track;
  std::cout << "tempo loaded path " << m_track->get_path() << std::endl;
  m_ring_buffer.empty();
  init();
}