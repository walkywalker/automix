#ifndef tune_def

#include "mixer.h"
#include "pugixml/src/pugixml.hpp"

class tune {
	private:
		std::deque<action_t> m_actions;
		double m_original_tempo;
		double m_set_tempo;
		double m_global_beat_start_time;
		double m_track_start_time;
		int m_beats_to_bar = 4; // assume this is always the case for now
		std::vector<std::pair<int, int>> m_drops;
		std::vector<int> m_drums;
		void set_initial_controls();
	public:
		tune(std::string track_path, double tempo, double track_start_time, std::vector<std::pair<int, int>> drops, std::vector<int> four_bar_drum_content, bool analysis_success);
		tune(pugi::xml_node tune_node);
		tune(std::string track_path);
		std::string m_path;
		bool m_analysis_success;
		int populate_xml_node(pugi::xml_node tune_node) ;
		std::deque<action_t> get_actions();
		double get_original_start_time();
		double get_original_tempo();
		double get_time_at_beat(int beat_idx);
		double get_time_at_bar(int bar_idx);
		double get_mapped_time_at_bar(int bar_idx);
		int get_num_drops();
		int get_drums(int bar_idx);
		std::pair<int, int> get_drop_bars(int drop_idx);
		void set_volume_ramp(double start_time, double end_time, double start_volume, double end_volume, int num_steps);
		void set_lpf_gain(double gain, double time);
		void pause(double time);
		void map_actions(int channel, double global_beat_start_time, double set_tempo);
};
#define tune_def
#endif
