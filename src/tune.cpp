#include "tune.h"

#include <algorithm>

// All timing is set relative to original tempo and start time, then shifted in
// map_actions

tune::tune(std::string track_path)
    : m_path(track_path), m_analysis_success(false) {}

tune::tune(std::string track_path, double tempo, double track_start_time,
           double volume, std::vector<std::pair<int, int>> drops,
           std::vector<int> four_bar_drum_content, bool analysis_success)
    : m_path(track_path), m_original_tempo(tempo), m_original_volume(volume),
      m_drops(drops), m_track_start_time(track_start_time),
      m_drums(four_bar_drum_content), m_analysis_success(analysis_success) {
  set_initial_controls();
}

tune::tune(pugi::xml_node tune_node) {
  m_path = tune_node.attribute("path").as_string();
  m_analysis_success = tune_node.attribute("analysis_success").as_bool();
  if (m_analysis_success) {
    m_original_tempo = tune_node.attribute("original_tempo").as_double();
    m_track_start_time = tune_node.attribute("original_start_time").as_double();
    m_original_volume = tune_node.attribute("original_volume").as_double();

    int tmp = 0;
    for (pugi::xml_attribute_iterator ait =
             tune_node.child("drops").attributes_begin();
         ait != tune_node.child("drops").attributes_end(); ++ait) {
      if (tmp % 2 == 0) {
        m_drops.push_back(std::make_pair(ait->as_double(), 0));
      } else {
        m_drops[tmp / 2].second = ait->as_double();
      }
      tmp++;
    }

    for (pugi::xml_attribute_iterator ait =
             tune_node.child("drums").attributes_begin();
         ait != tune_node.child("drums").attributes_end(); ++ait) {
      m_drums.push_back(ait->as_int());
    }
    set_initial_controls();
  }
}

void tune::set_initial_controls() {
  action_t action;
  action.channel = 0;
  action.time = 0;

  action.value = 0;
  action.control = LOAD;
  action.path = m_path;
  m_actions.push_back(action);

  action.control = VOL;
  m_actions.push_back(action);

  action.control = PLAY;
  m_actions.push_back(action);

  action.control = LPF;
  action.value = -23;
  m_actions.push_back(action);
}

int tune::populate_xml_node(pugi::xml_node tune_node) {
  tune_node.append_attribute("path") = m_path.c_str();
  tune_node.append_attribute("analysis_success") = m_analysis_success;
  if (m_analysis_success) {
    tune_node.append_attribute("original_tempo") = m_original_tempo;
    tune_node.append_attribute("original_start_time") = m_track_start_time;
    tune_node.append_attribute("original_volume") = m_original_volume;

    pugi::xml_node drums_node = tune_node.append_child("drums");
    for (auto drum : m_drums) {
      drums_node.append_attribute("drum") = drum;
    }
    pugi::xml_node drops_node = tune_node.append_child("drops");
    for (auto drop : m_drops) {
      drops_node.append_attribute("drop_start") = drop.first;
      drops_node.append_attribute("drop_end") = drop.second;
    }
  }

  return 0;
}

int tune::get_drums(int bar_idx) { return m_drums[bar_idx / 4]; }

double tune::get_original_start_time() { return m_track_start_time; }

double tune::get_original_tempo() { return m_original_tempo; }

double tune::get_original_volume() { return m_original_volume; }

std::deque<action_t> tune::get_actions() { return m_actions; }

double tune::get_time_at_beat(int beat_idx) {
  double beat_time = 60.0 / m_original_tempo;
  return m_track_start_time + (beat_time * beat_idx);
}

double tune::get_time_at_bar(int bar_idx) {
  return get_time_at_beat(bar_idx * m_beats_to_bar);
}

double tune::get_mapped_time_at_bar(int bar_idx) {
  double beat_time = 60.0 / m_set_tempo;
  return m_global_beat_start_time + (m_beats_to_bar * beat_time);
}

int tune::get_num_drops() { return m_drops.size(); }

std::pair<int, int> tune::get_drop_bars(int drop_idx) {
  return m_drops[drop_idx];
}

void tune::set_volume_ramp(double start_time, double end_time,
                           double start_volume, double end_volume,
                           int num_steps) {
  double time_step = (end_time - start_time) / num_steps;
  double volume_step = (end_volume - start_volume) / num_steps;
  action_t action;
  action.control = VOL;
  action.channel = 0; // set later
  for (int i = 1; i <= num_steps; i++) {
    action.time = start_time + (time_step * i);
    action.value = start_volume + (volume_step * i);
    m_actions.push_back(action);
  }
}

void tune::set_lpf_gain(double gain, double time) {
  action_t action;
  action.control = LPF;
  action.value = gain;
  action.time = time;
  m_actions.push_back(action);
}

void tune::pause(double time) {
  action_t action;
  action.control = PAUSE;
  action.time = time;
  action.path = m_path;
  m_actions.push_back(action);
}

void tune::map_actions(int channel, double global_beat_start_time,
                       double set_tempo, double volume_ratio) {
  m_set_tempo = set_tempo;
  m_global_beat_start_time = global_beat_start_time;
  action_t action;
  action.control = TEMPO;
  action.channel = 0;
  action.time = 0;
  action.value = m_original_tempo / set_tempo;

  m_actions.push_back(action);

  for (auto it = m_actions.begin(); it != m_actions.end(); it++) {
    it->time =
        ((it->time - m_track_start_time) * (m_original_tempo / set_tempo)) +
        global_beat_start_time;
    it->channel = channel;
    if (it->control == VOL) {
      it->value *= volume_ratio;
    }
  }
}