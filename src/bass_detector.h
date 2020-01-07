#include "filter.h"
#include "qm/beat_track.h"
#include <vamp-plugin-sdk/vamp-sdk/RealTime.h>	// Would be nice to get rid of this

#include <vector>

class bass_detector{
	private:
		int m_step_size;
		int m_window_size;
		float* m_buffer;
		bessel m_bp;
		std::vector<double> m_bass;

	public:
		bass_detector();
		bool initialise(int step_size, int window_size);
		Vamp::Plugin::FeatureSet process(const float *const *inputBuffers, Vamp::RealTime timestamp);
		std::vector<double> get_bass_content();
};