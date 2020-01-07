#include <analyzer.h>
#include <filter.h>
#include <bass_detector.h>
#include <qm/beat_track.h>
#include <qm/onset_detect.h>

#include <filesystem>
#include <cassert>


constexpr size_t window_size = 1024;
constexpr size_t step_base = 512; // Fixed
constexpr int step_div = 4;
constexpr size_t step_size = step_base/step_div;


analyzer::analyzer(std::string track_path, double input_tempo): m_track(track_path), m_input_tempo(input_tempo) {}

std::shared_ptr<tune> analyzer::get_tune() {
	double tempo;
	double first_beat;
	if (create_beat_grid(tempo, first_beat) != 0) {
		return std::make_shared<tune>(m_track.get_path());
	}
	return construct_tune(tempo, first_beat);
}

std::shared_ptr<tune> analyzer::construct_tune(const double tempo, const double first_beat) {
	m_analysis_log_file << "number of bass steps: " << std::to_string(m_bass_content.size()) << std::endl;
	std::for_each(m_bass_content.begin(), m_bass_content.end(), [this](double &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });

	// get bass content
	double four_bar_time = 16 * (60.0/tempo);
	int beat_section_length_samples = four_bar_time* 44100;
	int section_count = beat_section_length_samples + (first_beat * 44100);
	std::vector <double> four_bar_bass_content;
 	std::vector <int> four_bar_drum_content;
 	std::vector<std::pair<int, int>> drops;
 	double bass_track = 0;

	for (int step_idx = 0; step_idx < m_bass_content.size(); step_idx++) {
		if ((step_idx * step_base) > (first_beat * 44100)) {
			if ((step_idx * step_base) < section_count) {
				bass_track += m_bass_content[step_idx];
			} else {
				m_analysis_log_file << "step_idx " << std::to_string(step_idx) << " "<< std::to_string(double(section_count)/44100) << std::endl;
				four_bar_bass_content.push_back(bass_track);
				bass_track = m_bass_content[step_idx];
				section_count += beat_section_length_samples;
			}
		}
	}

	double time_temp = first_beat;

	while(time_temp < m_onset_features.back()) {
		four_bar_drum_content.push_back(get_onsets_in_range(time_temp, time_temp + four_bar_time));
		time_temp += four_bar_time;
	}

	std::for_each(four_bar_bass_content.begin(), four_bar_bass_content.end(), [this](double &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });
	std::for_each(four_bar_drum_content.begin(), four_bar_drum_content.end(), [this](int &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });

	auto bass_minmax = std::minmax_element(four_bar_bass_content.begin(), four_bar_bass_content.end());
	double bass_range = (*bass_minmax.second - *bass_minmax.first);

	m_analysis_log_file<< "bass range "<< std::to_string(bass_range) << std::endl;

	int bar = 0;
	int in_drop = 0;
	int start = 0;
	for (auto bass_val : four_bar_bass_content) {
		if ((*bass_minmax.second - bass_val) < (0.4 * bass_range)) {
			m_analysis_log_file<< "within range at bar  "<< std::to_string(bar) << std::endl;
			if (in_drop == 0) {
				start = bar;
				in_drop = 1;
			}
		} else {
			if (in_drop == 1) {
				drops.push_back(std::make_pair(start, bar));
				m_analysis_log_file << "drop start: " << std::to_string(start) << " drop end: " << std::to_string(bar)  << std::endl;
				in_drop = 0;
			}
		}
		bar += 4;
	}
	if (in_drop == 1) {
		drops.push_back(std::make_pair(start, bar));
		m_analysis_log_file << "drop start: " << std::to_string(start) << " drop end: " << std::to_string(bar)  << std::endl;

	}

	if (drops.size() <= 0) {
		std::cout << m_track.get_path() << std::endl;
	}
	assert(drops.size() > 0);

	return std::make_shared<tune>(m_track.get_path(), tempo, first_beat, drops, four_bar_drum_content, true);
}

void analyzer::open_log_file() {
	std::filesystem::path filesystem_path = m_track.get_path();
	std::string log_path(std::getenv("AUTOMIX_HOME"));
	log_path += "/log/";
	log_path += filesystem_path.filename();
	log_path += ".log";
	m_analysis_log_file.open(log_path.c_str());
	m_analysis_log_file << "Log file for " << m_track.get_path() << std::endl;
}

void analyzer::run() {
	int max_window_size = std::max_element(m_processes.begin(), m_processes.end(),
    [] (detector_helper const& s1, detector_helper const& s2)
    {
        return s1.window_size < s2.window_size;
    })->window_size;
    int min_step_size = std::min_element(m_processes.begin(), m_processes.end(),
    [] (detector_helper const& s1, detector_helper const& s2)
    {
        return s1.step_size < s2.step_size;
    })->step_size;

    for (auto helper : m_processes) {
    	if (helper.step_size % min_step_size != 0) {
    		throw;
    	}
    }

    float* interleaved_samples = new float[min_step_size*2];
	float* mono_samples = new float[max_window_size];

	for (int i = 0; i < min_step_size*2; i++) {
		interleaved_samples[i] = 0.0;
	}

	int buf_level = 0;
	bool noise_found = false;

	while (m_track.read(interleaved_samples, min_step_size*2) == min_step_size*2) {
		for (int i = 0; i < (min_step_size*2)-1; i=i+2) {
			mono_samples[(max_window_size-min_step_size)+(i/2)] = (interleaved_samples[i] + interleaved_samples[i+1])*0.5;
			if ((!noise_found) && ((interleaved_samples[i] + interleaved_samples[i+1])*0.5) > m_noise_threshold) {
				m_first_noise = (buf_level + (i/2))/44100;
				noise_found = true;
				m_analysis_log_file << "First noise found at time " << std::to_string(m_first_noise) << std::endl;	// should be separate detectpr
			}

		}

		buf_level+=min_step_size;

		for (detector_helper helper : m_processes) {
			if (buf_level % helper.step_size == 0 && buf_level >= max_window_size) {
				helper.process(&mono_samples, Vamp::RealTime::frame2RealTime(buf_level - helper.window_size, 44100)); // time at start of window
			}
		}

		for (int i = 0; i < max_window_size - min_step_size; i++) {
			mono_samples[i] = mono_samples[i+min_step_size];
		}
	}
}

int analyzer::process() {
	if (m_track.open_audio_source() != 0) {
		return 1;
	}

	open_log_file();

	// Set up beat tracker

	beat_tracker beat_analyzer = beat_tracker(44100, step_div);
	beat_analyzer.setParameter("inputtempo", m_input_tempo);
	std::string df("dftype");
	beat_analyzer.setParameter(df, 4);	// 4 for broadband

	if (beat_analyzer.initialise(1, step_size, window_size) != true) {
		m_analysis_log_file << "Error initialising beat track plugin" << std::endl;
		return 1;
	}
	m_analysis_log_file << "Using detection function " << beat_analyzer.getParameter(df) << std::endl;
	register_detector(beat_analyzer, step_size, window_size);

	// Set up onset detector

	onset_detector detector = onset_detector(44100);
	std::string program("Percussive onsets");
	detector.selectProgram(program);

	if (detector.initialise(1, step_base, window_size) != true) {
		m_analysis_log_file << "Error initialising onset detector plugin" << std::endl;
		return 1;
	}
	register_detector(detector, step_base, window_size);

	// Set up bass detector

	bass_detector bass_analyzer = bass_detector();
	if (bass_analyzer.initialise(step_base, step_base) != true) {
		m_analysis_log_file << "Error initialising bass detector" << std::endl;
		return 1;
	}
	register_detector(bass_analyzer, step_base, step_base);

	run();

	m_bass_content = bass_analyzer.get_bass_content();
	m_beat_features = get_timestamps(beat_analyzer.getRemainingFeatures()[0]);

	auto o_features = detector.getRemainingFeatures()[0];
	m_onset_features = get_timestamps(o_features);

	for (auto feature : o_features) {
		m_onset_values.push_back(feature.values[0]);
	}


	m_analysis_log_file << "num onsets: " << std::to_string(m_onset_features.size()) << std::endl;
	if (m_onset_features.size() > 60) {
		for (int i = 0; i < m_onset_features.size(); i++) {
			m_analysis_log_file << "time: " << m_onset_features[i]<< std::endl;
			m_analysis_log_file << "feature: " << o_features[i].values[0] << std::endl;
		}
	}

	if (m_beat_features.size() == 0) {
		m_analysis_log_file << "ERROR no beats detected" << std::endl;
		return 1;
	}

	return 0;

}

int analyzer::get_bpm(double &bpm, double &time) {	// time will be a plugin beat somewhere
	std::vector<double> beat_gaps;
	double mean;
	double sd;
	for (int i = 0; i < m_beat_features.size()-1; i++) {
		if (m_beat_features[i] > m_onset_features[0]) {	// only consider beats past first drum
			beat_gaps.push_back(m_beat_features[i+1]- m_beat_features[i]);
			m_analysis_log_file << "period " << beat_gaps.back() << std::endl;
		}
	}

	auto minmax = std::minmax_element(beat_gaps.begin(), beat_gaps.end());
	get_mean_stddev(beat_gaps, mean, sd);
	
	double total = 0;
	double cntr= 0;
	for (auto it = beat_gaps.begin(); it != beat_gaps.end(); ++it) {
		if (std::abs(*it - mean) < (sd)) {
			cntr += *it;
			total++;
		}
	}

	m_analysis_log_file << "min " << std::to_string(*minmax.first);
	m_analysis_log_file << "max " << std::to_string(*minmax.second);
	m_analysis_log_file << "avg " << std::to_string(mean);
	m_analysis_log_file << "stddev " << std::to_string(sd) << std::endl;
	m_analysis_log_file << "mean within stddev" << std::to_string(double(60)/(cntr/total)) << std::endl;

	bpm = round((double(60)/(cntr/total))*4)/4; // round to nearest 0.25 bpm remove?
	m_analysis_log_file << "Assuming bpm is " << std::to_string(bpm) << std::endl;

	int found_section = 0;
	int num_beats = 0;
	int consecutive_beat_threshold = 40;
	int beat_idx;
	for (beat_idx = 1; beat_idx < m_beat_features.size()-1; beat_idx++) {
		if (m_beat_features[beat_idx] > m_onset_features[0]) {	// could be more rigourous about ensuring there are drums in the section we chose here
			if ((std::abs(m_beat_features[beat_idx] - m_beat_features[beat_idx-1] - mean) < (2*sd)) &&
				(std::abs(m_beat_features[beat_idx+1] - m_beat_features[beat_idx] - mean) < (2*sd))) {
				num_beats++;
			} else {
				if (num_beats > consecutive_beat_threshold) {
					found_section = 1;
					break;
				} else {
					num_beats = 0;
				}
			}
		}
	}

	if (found_section == 0 && num_beats < consecutive_beat_threshold) {
		m_analysis_log_file << "ERROR could not find consistent beat pattern section" << std::endl;
		return 1;
	} else {
		m_analysis_log_file << "Found " << std::to_string(num_beats) << " candidate beats beginning at beat " << std::to_string(beat_idx- num_beats) << std::endl;
	}

	std::vector<double> beat_template;
	std::vector<double> consistent_beats;

	for (int i = beat_idx - num_beats; i<beat_idx - 1; i++) {
		consistent_beats.push_back(m_beat_features[i]);
	}

	for (int i = 0; i < num_beats-1; i ++) {
		beat_template.push_back(consistent_beats[0]+((60.0*i)/bpm));
	}

	// find best grid alignment
	minimise_distance(beat_template, consistent_beats);

	std::for_each(consistent_beats.begin(), consistent_beats.end(), [this](double &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });
	std::for_each(beat_template.begin(), beat_template.end(), [this](double &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });

	time = beat_template[0];
	return 0;
}

int analyzer::align_beat_grid(const double &bpm, double &time) {	// time will be our best guess at first beat
	int resolution = 8;
	std::vector<double> likely_kicks;
	std::vector<int> likely_kick_beat_offsets;	// multiples of 1/resolution beats
	std::vector<int> offset_count(resolution, 0);
	double kick_mean;
	double kick_sd;
	double beat_interval = 60.0/bpm;

	// get potential kicks, this is a bit useless atm
	get_mean_stddev(m_onset_values, kick_mean, kick_sd);

	for (int onset_idx = 0; onset_idx < m_onset_values.size(); onset_idx++) {
		if (m_onset_values[onset_idx] > (kick_mean + (kick_sd*0.5))) {
			likely_kicks.push_back(m_onset_features[onset_idx]);
		}
	}
	
	// check first noise exists
	if (m_first_noise < 0) {
		m_analysis_log_file << "Error no noise found" << std::endl;
		return 1;
	}

	// shift first beat back to start
	double init_beat = time;
	while(init_beat > m_first_noise + beat_interval) {
		init_beat -= beat_interval;
	}

	// if there is an onset before the first noise something has gone wrong
	double silence_to_first_onset = m_onset_features[0] - m_first_noise; 

	if (silence_to_first_onset < -1*(1024.0/44100)) {
		m_analysis_log_file << "DIFF " << std::to_string(m_onset_features[0]- m_first_noise - (1024/44100)) << std::endl;
		m_analysis_log_file << "Error first onset in silence" << std::endl;
		return 1;
	}

	// Work out beat offsets for likely kicks
	double temp_offset;
	m_analysis_log_file << "likely KICKS mean " << std::to_string(kick_mean) << " stddev " << std::to_string(kick_sd) << std::endl;
	for (auto kick : likely_kicks) {
		if (kick > init_beat) {
			temp_offset = round((fmod((kick - init_beat), beat_interval)/beat_interval)*resolution);
			if (temp_offset == resolution) {
				temp_offset = 0;	// rounding has casued us to make up a whole beat
			}

			m_analysis_log_file<<std::to_string(kick) << " " << std::to_string(temp_offset) << std::endl;
			likely_kick_beat_offsets.push_back(temp_offset);
			offset_count.at(temp_offset)++;
		}
	}

	std::for_each(offset_count.begin(), offset_count.end(), [this](int &n){ m_analysis_log_file<<std::to_string(n) << std::endl; });
	m_analysis_log_file << "Initial beat is " << std::to_string(init_beat) << std::endl;

	double kick_per_beat_mean;
	double kick_per_beat_sd;

	get_mean_stddev(offset_count, kick_per_beat_mean, kick_per_beat_sd);

	// choose either first noise or onset as guide to alignment

	double initial_onset_offset_time;

	if (silence_to_first_onset > beat_interval) {
		m_analysis_log_file<< "Using non-silence as starting point: " << std::to_string(silence_to_first_onset) << std::endl;
		initial_onset_offset_time = fmod((m_first_noise - init_beat), beat_interval);
	} else {
		m_analysis_log_file<< "Using first onset as starting point" << std::endl;
		initial_onset_offset_time = fmod((m_onset_features[0] - init_beat), beat_interval);
	}

	int initial_onset_offset_beat = round(initial_onset_offset_time/beat_interval*resolution);

	m_analysis_log_file<< "First onset beat offset: " << std::to_string(initial_onset_offset_beat) << std::endl;

	int abs_initial_onset_offset_beat;
	if (initial_onset_offset_beat >= 0) {
		abs_initial_onset_offset_beat = initial_onset_offset_beat;
	} else {
		abs_initial_onset_offset_beat = resolution + initial_onset_offset_beat;
	}

	// try use kicks to confirm the shift is reasonable

	bool shift_good_for_kicks = offset_count[abs_initial_onset_offset_beat] > kick_per_beat_mean;

	if (!shift_good_for_kicks) {
		m_analysis_log_file<< "Shift of : " << std::to_string(initial_onset_offset_beat) << " Not good for kicks, output: " << std::to_string(offset_count[abs_initial_onset_offset_beat]) << " mean: " << std::to_string(kick_per_beat_mean) << std::endl;
		return 1;
	}

	// perform shift
		
	init_beat += (beat_interval/resolution)*initial_onset_offset_beat;
	m_analysis_log_file << "Initial beat shifted is " << std::to_string(init_beat) << std::endl;

	time = init_beat;
	
    return 0;
}


int analyzer::create_beat_grid(double &tempo, double &first_beat) {	// todo move to own class

	if (get_bpm(tempo, first_beat) !=0 ) {
		return 1;
	}

	if (align_beat_grid(tempo, first_beat) != 0) {
		return 1;
	}
	return 0;
}


int analyzer::get_onsets_in_range(double start_time, double end_time) {
	int num_onsets = 0;
	for (int i = 0; i < m_onset_features.size(); i++) {
		if (m_onset_features[i] < end_time && m_onset_features[i] > start_time) {
			num_onsets++;
		}
	}
	return num_onsets;
}

std::vector<double> analyzer::get_timestamps(Vamp::Plugin::FeatureList features) {
	std::vector<double> output;
	for (auto feature : features) {
		output.push_back(feature.timestamp.sec + (double(feature.timestamp.nsec)/1000000000));
	}
	return output;
}

double analyzer::get_distance(std::vector<double> &v1, std::vector<double> &v2) {
	assert(v1.size() == v2.size());
	double distance = 0;
	for (int i = 0; i < v1.size(); i++) {
		distance += std::abs(v1[i]-v2[i]);
	}
	return distance;
}

void analyzer::shift(std::vector<double> &v1, double shift_val) {
	std::for_each(v1.begin(), v1.end(), [shift_val](double &n){ n+=shift_val; });
}

void analyzer::minimise_distance(std::vector<double> &move, std::vector<double> &nomove) {
	// Primitive local minimum algorithm
	int direction = 1;
	double init_distance = get_distance(move, nomove);
	shift(move, 0.001);
	double fwd_distance = get_distance(move, nomove);
	shift(move, -0.002);
	double bwd_distance = get_distance(move, nomove);
	shift(move, 0.001);
	if (init_distance < fwd_distance && init_distance < bwd_distance) {
		return;
	} else if (fwd_distance > bwd_distance) {
		direction = -1;
	}
	double nxt_distance = init_distance;
	double prev_distance;
	while(1) {
		shift(move, direction*0.001);
		prev_distance = nxt_distance;
		nxt_distance = get_distance(move, nomove);
		if (nxt_distance > prev_distance) {
			shift(move, direction*-0.001);
			return;
		}
	}
}

void analyzer::get_mean_stddev(std::vector<double> input, double &mean, double &stddev) {
	double cntr= 0;
	for (auto it = input.begin(); it != input.end(); ++it) {
		cntr += *it;
	}
	mean = cntr/input.size();

	double var = 0;
	for (auto it = input.begin(); it != input.end(); ++it) {
		var += (*it - mean) * (*it - mean);
	}
	stddev = sqrt(var/input.size());
}

void analyzer::get_mean_stddev(std::vector<int> input, double &mean, double &stddev) {
	double cntr= 0;
	for (auto it = input.begin(); it != input.end(); ++it) {
		cntr += *it;
	}
	mean = cntr/input.size();

	double var = 0;
	for (auto it = input.begin(); it != input.end(); ++it) {
		var += (*it - mean) * (*it - mean);
	}
	stddev = sqrt(var/input.size());
}