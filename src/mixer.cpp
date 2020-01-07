#include "mixer.h"
#include <numeric>

mixer::mixer(std::deque<action_t> actions, int num_channels, int frame_data_length) {
	m_actions = actions;
	m_num_channels = num_channels;
	m_channels = new channel* [num_channels];
	m_input_data = new float* [num_channels];
	for (int i = 0; i < num_channels; i++) {
		m_input_data[i] = new float [frame_data_length];
	}
	m_time = 0;
}

int mixer::read(float* data_ptr, int num_samples) {
	int error = 0;
	int read_samples = 0;
	int all_paused = 1;
	std::vector<int> read_sizes;
	std::vector<action_t> step_actions;
	double step_time = (m_time/(2*44100));

	while (m_actions.size() > 0 && m_actions.front().time <= (m_time+num_samples)/(2*44100)) {
		action_t action = m_actions.front();
		read_sizes.push_back((action.time - step_time)*2*44100);
		step_actions.push_back(action);
		step_time = action.time;
		m_actions.pop_front();
		std::cout << std::to_string(m_actions.size()) << " actions remaining" << std::endl;
	}
	read_sizes.push_back(num_samples - std::accumulate(read_sizes.begin(), read_sizes.end(), 0));

	for (int i = 0; i < num_samples; i++) {
		data_ptr[i] = 0.0;
	}

	int offset = 0;
	for (int step_idx = 0; step_idx < read_sizes.size(); step_idx++) {
		if (read_sizes[step_idx] != 0) {
			for (int i = 0; i < m_num_channels; i++) {
				if (m_channels[i]->get_state() == play) {
					all_paused = 0;
					read_samples = m_channels[i]->read(m_input_data[i], read_sizes[step_idx]);
					if (read_samples != read_sizes[step_idx]) {
						std::cout << "failed to read from channel " << std::to_string(i) << std::endl;
						return read_samples;
					}
					for (int j = 0; j < read_sizes[step_idx]; j++) {
						data_ptr[offset+j] += m_input_data[i][j];
					}

				}	
			}
		}
		offset+=read_sizes[step_idx];
		if (step_idx != read_sizes.size() -1) {
			error = m_channels[step_actions[step_idx].channel]->apply_action(step_actions[step_idx]);
			if (error) {
				std::cout << "Error applying step_actions[step_idx] to channel " << std::to_string(step_actions[step_idx].channel) << std::endl;
				return 1;
			}
		}
	}
	if (all_paused == 1) {
		std::cout << "all channels paused, finishing" << std::endl;
		return 0;
	}
	m_time += num_samples;

	return num_samples;

}

int mixer::connect_channel(channel* ch) {
	static int count = 0;
	if (count >= m_num_channels) {
		std::cout << "Too many channels" << std::endl;
		return 1;
	}
	m_channels[count] = ch;
	count++;
	return 0;
}