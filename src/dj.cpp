#include "dj.h"
#include <cassert>
#include <algorithm>
#include <filesystem>

dj::dj(std::vector<std::shared_ptr<tune>> tunes): m_tunes(tunes) {}

void dj::mix(double tempo, int num_channels, int double_drop_prob, int breakdown_prob) {

    double t_time = 0;
    int ch_num = 0;

    std::vector<std::shared_ptr<tune>> tunes = m_tunes;
    std::random_shuffle(tunes.begin(), tunes.end());

    // get breakdown tunes
    std::set<std::string> breakdown_tunes = position_breakdown_transitions(tunes, breakdown_prob);

    std::shared_ptr<tune> current_tune = tunes.back();
    tunes.pop_back();
    current_tune->set_volume_ramp(current_tune->get_time_at_bar(0), current_tune->get_time_at_bar(4), 0.0, 1.0, 16);
    current_tune->set_lpf_gain(0.0, current_tune->get_time_at_bar(0));

    t_time = (current_tune->get_original_start_time()*(current_tune->get_original_tempo()/tempo));
    std::shared_ptr<tune> next_tune;
    // int current_tune_transition_bar;
    int bar_to_start;

    while (tunes.size() > 0) {
        next_tune = tunes.back();
        std::cout << "Mixing into " << next_tune->m_path << std::endl;
        tunes.pop_back();
        int tmp = rand() % 100;
        if (breakdown_tunes.count(next_tune->m_path) > 0) {
            bar_to_start = breakdown_mix(current_tune, next_tune); 
        } else if (tmp < double_drop_prob) {
            bar_to_start = double_drop_mix(current_tune, next_tune); 
        } else {
            bar_to_start = normal_mix(current_tune, next_tune); // switch to drop
        }

        current_tune->map_actions(ch_num, t_time, tempo);
        assert(current_tune->get_actions().size() > 0);
        auto it_actions = current_tune->get_actions();
        for (auto action_it = it_actions.begin(); action_it != it_actions.end(); action_it++) {
            m_actions.push_back(*action_it);
        }

        current_tune = next_tune;
        t_time += bar_to_start*4*(60.0/tempo);
        m_bar+=bar_to_start;

        ch_num = (ch_num + 1) % num_channels;
    }

    current_tune->map_actions(ch_num, t_time, tempo);
    assert(current_tune->get_actions().size() > 0);
    auto it_actions = current_tune->get_actions();
    for (auto action_it = it_actions.begin(); action_it != it_actions.end(); action_it++) {
        m_actions.push_back(*action_it);
    }
    std::sort(m_actions.begin(), m_actions.end());
}

int dj::normal_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune) {
    int current_tune_transition_bar = current_tune->get_drop_bars(0).second;
    int bar_to_start = current_tune_transition_bar - next_tune->get_drop_bars(0).first;

    if (bar_to_start < (-1*m_bar)) {
        bar_to_start = -1*m_bar;
    }
    next_tune->set_volume_ramp(next_tune->get_time_at_bar(0), next_tune->get_time_at_bar(4), 0.0, 1.0, 16);
    next_tune->set_lpf_gain(0.0, next_tune->get_time_at_bar(next_tune->get_drop_bars(0).first));

    current_tune->set_lpf_gain(-23, current_tune->get_time_at_bar(current_tune_transition_bar));
    current_tune->pause(current_tune->get_time_at_bar(current_tune_transition_bar ) - 0.1);
    return bar_to_start;
}

std::set<std::string> dj::position_breakdown_transitions(std::vector<std::shared_ptr<tune>> &tunes, int breakdown_prob) {
    std::set<std::string> breakdown_tunes;
    std::vector<int> breakdown_tune_idxs;
    int count = 0;
    int dummy;
    int num_breakdown_tunes = ceil(tunes.size()*(double(breakdown_prob)/100));
     if (num_breakdown_tunes < 1) {
        return breakdown_tunes;
     }

    for (auto tune : tunes) {
        if (find_buildup_no_drums(tune, dummy) && breakdown_tune_idxs.size() < num_breakdown_tunes) {
            breakdown_tune_idxs.push_back(count);
            breakdown_tunes.insert({tune->m_path});
            std::cout << "Selecting " << tune->m_path << " for breakdown transition" << std::endl;
        }
        count++;
    }

    if (breakdown_tune_idxs.size() < num_breakdown_tunes) {
        std::cout << "Could only find " << std::to_string(breakdown_tune_idxs.size()) << " breakdown tunes" << std::endl;
        num_breakdown_tunes = breakdown_tune_idxs.size();
    }

    std::vector<int> breakdown_tune_positions;

    for (int i = 1; i <= num_breakdown_tunes; i++) {
        breakdown_tune_positions.push_back((tunes.size()*i)/(num_breakdown_tunes+1));

        std::iter_swap(tunes.begin() + breakdown_tune_idxs[i-1], tunes.begin() + ((tunes.size()*i)/(num_breakdown_tunes+1)));
    }

    return breakdown_tunes;
}

bool dj::find_buildup_no_drums(std::shared_ptr<tune> tune, int &bar_to_switch_too) {
    // look for a section with no drums in the next build up
    bool found = false;
    for (int bar_idx = tune->get_drop_bars(0).first - 8; bar_idx >= 8; bar_idx-=8) {
        if (tune->get_drums(bar_idx) < 16) {
            found = true;
            std::cout << "Found buildup section to switch too as bar " << std::to_string(bar_idx) << std::endl;
            bar_to_switch_too = bar_idx;
            break;
        }
    }
    return found;
}

int dj::breakdown_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune) {
    int next_tune_start_bar;

    if (!find_buildup_no_drums(next_tune, next_tune_start_bar)) {
        std::cout << "Could not find suitable section in buildup"<< std::endl;
        next_tune_start_bar = next_tune->get_drop_bars(0).first;
    }

    int current_tune_transition_bar = current_tune->get_drop_bars(0).second;
    int bar_to_start = current_tune_transition_bar - next_tune_start_bar;

    if (bar_to_start < (-1*m_bar)) {
        bar_to_start = -1*m_bar;
    }

    next_tune->set_volume_ramp(next_tune->get_time_at_bar(0), next_tune->get_time_at_bar(4), 0.0, 1.0, 16);
    next_tune->set_lpf_gain(0.0, next_tune->get_time_at_bar(next_tune_start_bar));

    current_tune->set_lpf_gain(-23, current_tune->get_time_at_bar(current_tune_transition_bar));
    current_tune->pause(current_tune->get_time_at_bar(current_tune_transition_bar ) - 0.1);
    return bar_to_start;
}

int dj::double_drop_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune) {
    int current_tune_transition_bar = current_tune->get_drop_bars(0).first + 16;    // todo look for a good spot rather than hard code
    int bar_to_start = current_tune_transition_bar - next_tune->get_drop_bars(0).first;

    if (bar_to_start < (-1*m_bar)) {
        bar_to_start = -1*m_bar;
    }

    next_tune->set_volume_ramp(next_tune->get_drop_bars(0).first - 8, next_tune->get_drop_bars(0).first - 4, 0.0, 1.0, 16);
    next_tune->set_lpf_gain(0.0, next_tune->get_time_at_bar(next_tune->get_drop_bars(0).first));    // todo look at drums and maybe switch base earlier

    current_tune->set_lpf_gain(-23, current_tune->get_time_at_bar(current_tune_transition_bar)-.1);
    current_tune->pause(current_tune->get_time_at_bar(current_tune_transition_bar + 16));   // todo look and ensure we are stil in the drop here
    return bar_to_start;
}

void dj::print_tracklist() {
    // assuming tracks are only ever played and paused once
    std::cout << std::endl << "Tracklist:" << std::endl;
    for (auto it = m_actions.begin(); it != m_actions.end() ; it++) {
        if (it->control == PLAY) {
            std::cout << std::to_string(it->time) << " " << std::filesystem::path(it->path).filename() << std::endl;
        } 
    }
}

std::deque<action_t> dj::get_actions() {
    return m_actions;
}