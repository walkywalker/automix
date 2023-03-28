#ifndef mix_def

#include <channel.h>

class mixer {
private:
  std::deque<action_t> m_actions;
  double m_time;
  int m_num_channels;
  channel **m_channels;
  float **m_input_data;

public:
  mixer(std::deque<action_t> actions, int num_channels, int frame_data_length);
  int read(float *data_ptr, int num_samples);
  int connect_channel(channel *ch);
};
#define mix_def
#endif
