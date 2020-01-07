#include "track.h"
#include "filter.h"
#include <tempo.h>
#include <queue>

typedef enum {
	LOAD,
	PLAY,
	PAUSE,
	TEMPO,
	VOL,
	LPF
} control_t;

struct action_t {
	control_t control;
	std::string path;
	double value;
	double time;
	int channel;

	bool operator<(action_t const &other) { return time < other.time; }
};

typedef enum {
	pause = 0,
	play = 1
} state_t;

class channel {
	private:
		track* m_track;
		tempo m_tempo;
		biquad m_lpf;
		bessel m_bp;
		double m_time;
		float m_volume;
		state_t m_state;
	public:
		channel();
		int read(float* data_ptr, int num_samples);
		int apply_action(action_t action);
		void load(track* track);
		state_t get_state();
};
