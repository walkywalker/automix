#include "track.h"
#include "tune.h"
#include <vamp-plugin-sdk/vamp-sdk/RealTime.h>	// Would be nice to get rid of this

#include <memory>
#include <algorithm>

#ifndef analyzer_def

struct detector_helper {
	std::function<Vamp::Plugin::FeatureSet(const float *const *inputBuffers, Vamp::RealTime timestamp)> process;
	int step_size;
	int window_size;
};

class analyzer {
	private:
		track m_track;
		double m_input_tempo;
		double m_vol;
		std::ofstream m_analysis_log_file;
		std::vector<detector_helper> m_processes;
		std::vector<double> m_beat_features;
		std::vector<double> m_onset_features;
		std::vector<double> m_onset_values;
		std::vector<double> m_bass_content;
		double m_noise_threshold = 0.03;
		double m_first_noise = -1;
		template<class T>
		void register_detector(T &detector, int step_size, int window_size);
		void run();
		int create_beat_grid(double &tempo, double &first_beat);
		int align_beat_grid(const double &bpm, double &time);
		int get_bpm(double &bpm, double &time);
		std::shared_ptr<tune> construct_tune(const double tempo, const double first_beat);
		std::vector<double> get_timestamps(Vamp::Plugin::FeatureList features);
		double get_distance(std::vector<double> &v1, std::vector<double> &v2);
		void shift(std::vector<double> &v1, double shift_val);
		void minimise_distance(std::vector<double> &move, std::vector<double> &nomove);
		void get_mean_stddev(std::vector<double> input, double &mean, double &stddev);
		void get_mean_stddev(std::vector<int> input, double &mean, double &stddev);
	public:
		analyzer(std::string track_path, double input_tempo);
		void open_log_file();
		int process();
		std::shared_ptr<tune> get_tune();
		int get_onsets_in_range(double start_time, double end_time);
};

template<class T>
void analyzer::register_detector(T &detector, int step_size, int window_size) {
	detector_helper helper;
	helper.process = std::bind(&T::process, &detector, std::placeholders::_1, std::placeholders::_2);
	helper.step_size = step_size;
	helper.window_size = window_size;
	m_processes.push_back(helper);
}

#define analyzer_def
#endif
