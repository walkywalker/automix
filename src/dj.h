#include "tune.h"
#include <memory>
#include <set>

class dj {
	private:
		std::vector<std::shared_ptr<tune>> m_tunes;
		std::deque<action_t> m_actions;
		int m_bar = 0;
	public:
		std::string m_path;
		dj(std::vector<std::shared_ptr<tune>> tunes);
		void mix(double tempo, int num_channels, int double_drop_prob, int breakdown_prob);
		void print_tracklist();
		std::deque<action_t> get_actions();

		std::set<std::string> position_breakdown_transitions(std::vector<std::shared_ptr<tune>> &tunes, int breakdown_prob);

		int normal_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune);
		int breakdown_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune);
		int double_drop_mix(std::shared_ptr<tune> current_tune, std::shared_ptr<tune> next_tune);

		bool find_buildup_no_drums(std::shared_ptr<tune> tune, int &bar_to_switch_too);
};