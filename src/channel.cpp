#include <channel.h>
#include "filter.h"

static const double kStartupLoFreq = 50.0;
static const double kQKill = 0.9;
static const double lpf_gain = 0;


channel::channel(): m_tempo(tempo()), m_lpf(biquad()), m_bp(bessel()) {
	m_track = nullptr;
	m_lpf.set_coefs(44100, kStartupLoFreq, kQKill, lpf_gain);
	m_bp.set_coefs(44110, 50, 3.0, 1);	// Warning, hardcoded in filter
	m_time = 0;
	m_volume = 0.0;
	m_state = pause;
}

int channel::read(float* data_ptr, int num_samples) {
	if (m_track) {
		int read_samples = m_tempo.read(data_ptr, num_samples);
		m_time += read_samples;
		if (read_samples != num_samples) {
			return read_samples;
		} else {
			m_lpf.process_samples(data_ptr, num_samples);
			if (0) {
				m_bp.process_samples(data_ptr, num_samples);	// for filter testing purposes
			}
			for (int i = 0; i < num_samples; i++) {
				data_ptr[i] = data_ptr[i] * m_volume;
			}
			return num_samples;
		}
	} else {
		std::cout << "Error no track loaded" << std::endl;
		return 1;
	}
}

state_t channel::get_state() {
	return m_state;
}


int channel::apply_action(action_t action) {
	if (action.control == LPF) {
		std::cout << "setting lpf gain to " << std::to_string(action.value) << " at time " << std::to_string(m_time) << std::endl;
		m_lpf.set_coefs(44100, kStartupLoFreq, kQKill, action.value);
	} else if (action.control == VOL) {
		std::cout << "setting volume to " << std::to_string(action.value) << " at time " << std::to_string(m_time) << std::endl;
		m_volume = action.value;
	} else if (action.control == PLAY) {
		std::cout << "playing at time " << std::to_string(m_time) << std::endl;
		m_state = play;
	} else if (action.control == PAUSE) {
		std::cout << "pausing at time " << std::to_string(m_time) << std::endl;
		m_state = pause;
	} else if (action.control == TEMPO) {
		std::cout << "changing tempo at time " << std::to_string(m_time) << std::endl;
		m_tempo.set_tempo_ratio(action.value);
	} else {
		m_track = new track(action.path);
		m_tempo.load(m_track);
		int err = m_track->open_audio_source();
		if (err) {
			std::cout << "Error creating track" << std::endl;
			return 1;
		}
	}
	return 0;
}

void channel::load(track* track) {
	m_track = track;
}