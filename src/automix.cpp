#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cassert>
#include <thread>
#include <mutex>
#include "pugixml/src/pugixml.hpp"

#include "track.h"
#include "recorder.h"
#include "mixer.h"
#include "tune.h"
#include "dj.h"
#include "analyzer.h"


std::mutex tune_mutex;
std::mutex path_mutex;

void analyze_track(std::vector<std::shared_ptr<tune>> &tune_list, std::vector<std::string> &path_list, double input_tempo) {
    while (true) { 
        path_mutex.lock();
        if (path_list.size() == 0) {
            path_mutex.unlock();
            return;
        }

        analyzer wow = analyzer(path_list.back(), input_tempo);
        path_list.pop_back();
        path_mutex.unlock();

        if (wow.process() != 0) {
            throw;
        }

        std::shared_ptr<tune> analysis_tune = wow.get_tune();
        tune_mutex.lock();
        tune_list.push_back(analysis_tune);
        tune_mutex.unlock();
    }
}

pugi::xml_node get_node_from_path(pugi::xml_document &doc, const std::string &path) {
    std::string legal_path = path;
    std::string illegal_chars = "':/*@()[]+-";
 
    for (char c: illegal_chars) {
        legal_path.erase(std::remove(legal_path.begin(), legal_path.end(), c), legal_path.end());
    }

    std::string xpath = "/tune[@path='" + legal_path + "']";
    std::cout << xpath << std::endl;
    pugi::xpath_node_set path_match = doc.select_nodes(xpath.c_str());
    
    for (pugi::xpath_node node: path_match)
    {
        return node.node();
    }
    return pugi::xml_node();
}

std::vector<std::shared_ptr<tune>> get_tunes(std::vector<std::string> track_paths, double input_tempo, pugi::xml_document &doc, bool multithreaded, bool re_analyze) {
    int num_threads = std::thread::hardware_concurrency() - 1;
    std::vector<std::thread> thread_vector;
    std::vector<std::string> paths_to_analyze;
    std::vector<std::string> paths_from_xml;
    std::vector<std::shared_ptr<tune>> tunes_from_xml;
    std::vector<std::shared_ptr<tune>> tunes_from_analysis;
    std::vector<std::shared_ptr<tune>> tunes_to_use;

    for (const auto & path : track_paths) {
        pugi::xml_node potential_node = get_node_from_path(doc, path);
        if (strcmp(potential_node.name(), "") && !re_analyze) {
            tunes_from_xml.push_back(std::make_shared<tune>(potential_node));
            paths_from_xml.push_back(path);
        } else {
            paths_to_analyze.push_back(path);
        }
    }

    std::cout << "Paths found in XML:" << std::endl;
    for (auto & path : paths_from_xml) {
        std::cout << "  " << path << std::endl;
    }
    std::cout << "Paths to analyze:" << std::endl;
    for (auto & path : paths_to_analyze) {
        std::cout << "  " << path << std::endl;
    }
    std::cout << std::endl;

    if (paths_to_analyze.size() > 0) {
        if (multithreaded) {
            std::cout << "Using " << std::to_string(num_threads) << " threads for track analysis" << std::endl;
            for (int i = 0; i < num_threads; i++) {
                thread_vector.push_back(std::thread(analyze_track, std::ref(tunes_from_analysis), std::ref(paths_to_analyze), input_tempo));
            }
            for(auto& t: thread_vector) {
                t.join();
            }
        } else {
            std::cout << "Using single thread for track analysis" << std::endl;
            analyze_track(tunes_from_analysis, paths_to_analyze, input_tempo);
        }
    }
    

    for (auto tune : tunes_from_analysis) {
        pugi::xml_node potential_node = get_node_from_path(doc, tune->m_path);
        if (strcmp(potential_node.name(), "")) {
            tune->populate_xml_node(potential_node);
        } else {
            tune->populate_xml_node(doc.append_child("tune"));
        }
    }

    for (int tune_idx = 0; tune_idx < tunes_from_xml.size(); tune_idx++) {
        if (!tunes_from_xml[tune_idx]->m_analysis_success) {
            std::cout << "Not using tune from XML due to previous analysis failure: " << tunes_from_xml[tune_idx]->m_path << std::endl;
        } else {
            tunes_to_use.push_back(tunes_from_xml[tune_idx]);
        }
    }
    
    for (int tune_idx = 0; tune_idx < tunes_from_analysis.size(); tune_idx++) {
        if (!tunes_from_analysis[tune_idx]->m_analysis_success) {
            std::cout << "Not using analyzed tune: " << tunes_from_analysis[tune_idx]->m_path << std::endl;
        } else {
            tunes_to_use.push_back(tunes_from_analysis[tune_idx]);
        }
    }

    std::cout << "Generated tunes" << std::endl << std::endl;
    return tunes_to_use;
}

std::vector<std::string> get_track_paths(std::string track_dir_path, int max_num_tracks) {
    std::vector<std::string> paths;
    for (const auto & entry : std::filesystem::directory_iterator(track_dir_path)) {
        if (entry.path().extension() == ".mp3" || entry.path().extension() == ".m4a") {    // only support mp3 and m4a for now
            paths.push_back(entry.path());
        }
    }
    std::random_shuffle(paths.begin(), paths.end());
    if (paths.size() > max_num_tracks) {
        paths.resize(max_num_tracks);
    }
    return paths;
}

int check_environment_exists(bool &xml_file_exists, std::string &xml_file) {
    const char* home_dir_var = std::getenv("AUTOMIX_HOME");

    if (home_dir_var == nullptr) {
        std::cerr << "Error environment variable AUTOMIX_HOME not set" << std::endl;
        return 1;
    }

    std::string home_dir(home_dir_var);
    std::string log_dir = home_dir + "/log";
    std::string tmp_dir = home_dir + "/tmp";
    xml_file = tmp_dir + "/automix.xml";

    if (!std::filesystem::is_directory(std::filesystem::path(home_dir))) {
        std::cerr << "Error " << home_dir << " is not an existing directory" << std::endl;
        return 1;
    }

    if (!std::filesystem::is_directory(std::filesystem::path(log_dir)))  {
        std::cerr << "Error " << log_dir << " is not an existing directory" << std::endl;
        return 1;
    }

    if (!std::filesystem::is_directory(std::filesystem::path(tmp_dir)))  {
        std::cerr << "Error " << tmp_dir << " is not an existing directory" << std::endl;
        return 1;
    }

    xml_file_exists = std::filesystem::is_regular_file(std::filesystem::path(xml_file));

    return 0;
}

void display_help_info() {
    std::stringstream help_stream;
    help_stream << "Welcome to Automix, the automated DJ program! Automix takes a directory containing audio files as input and outputs a mp3 file containing the resultant mix" << std::endl << std::endl;;
    help_stream << "Mandatory arguements:" << std::endl;
    help_stream << "-i      Input directory" << std::endl;
    help_stream << "-o      Output file" << std::endl << std::endl;
    help_stream << "Optional arguements:" << std::endl;
    help_stream << "-dd     Double drop probability (%)   Default: 20" << std::endl;
    help_stream << "-bd     Break down probability  (%)   Default: 20" << std::endl;
    help_stream << "-ot     Output mix tempo        (BPM) Default: 87.5" << std::endl;
    help_stream << "-it     Input tempo hint        (BPM) Default: 87.5" << std::endl;
    help_stream << "-s      Random seed                   Default: 1" << std::endl;
    help_stream << "-l      Max number of tracks          Default: 25" << std::endl;
    help_stream << "-m      Use multiple threads          Default: false" << std::endl << std::endl;
    help_stream << "For more detailed descriptions of the functionality of these options please refer to the README" << std::endl;

    std::cout << help_stream.str() << std::endl;
}

class input_parser{
    //https://stackoverflow.com/questions/865668/how-to-parse-command-line-arguments-in-c
    public:
        input_parser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }
        /// @author iain
        const std::string get_option(const std::string &option) const{
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }
        /// @author iain
        bool option_exists(const std::string &option) const{
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        std::vector <std::string> tokens;
};


int main(int argc, char **argv)
{
    input_parser in(argc, argv);
    // See if user wants help
    if (in.option_exists("--help") || in.option_exists("-h")) {
        display_help_info();
        return 0;
    }

    // variables for command line arguements
    bool multithreaded = false;
    bool update_xml = false; 
    int double_drop_prob = 20;
    int breakdown_prob = 20;
    int max_length = 25;
    int seed = 1;
    double input_tempo = 87.5;
    double output_tempo = 87.5;
    const int num_channels = 6;   // should be enough so there are no timing issues

    // Process arguements
    std::string input_dir_path = in.get_option("-i");
    if (input_dir_path.empty()) {
        std::cerr << "Invalid arguement(s), try --help for usage examples" << std::endl;
        return 1;
    }

    std::string output_file_path = in.get_option("-o");
    if (output_file_path.empty()) {
        std::cerr << "Invalid arguement(s), try --help for usage examples" << std::endl;
        return 1;
    }

    if(in.option_exists("-dd")) {
        double_drop_prob = std::stoi(in.get_option("-dd"));
    }

    if(in.option_exists("-bd")) {
        breakdown_prob = std::stoi(in.get_option("-bd"));
    }

    if(in.option_exists("-ot")) {
        output_tempo = std::stod(in.get_option("-ot"));
    }

    if(in.option_exists("-it")) {
        input_tempo = std::stod(in.get_option("-it"));
    }

    if(in.option_exists("-s")) {
        seed = std::stoi(in.get_option("-s"));
    }

    if(in.option_exists("-l")) {
        max_length = std::stoi(in.get_option("-l"));
    }

    if(in.option_exists("-m")) {
        multithreaded = true;
    }

    if(in.option_exists("-u")) {
        // update_xml = true;   TODO: updating XML node need implementing
    }

    std::stringstream option_message;
    option_message << "Automix" << std::endl;
    option_message << "-------" << std::endl;
    option_message << "Operation parameters:" << std::endl;
    option_message << "     Input Directory Path:   " << input_dir_path << std::endl;
    option_message << "     Output File Path:       " << output_file_path << std::endl;
    option_message << "     Multithreaded:          " << multithreaded << std::endl;
    // option_message << "     Re-process:             " << update_xml << std::endl;
    option_message << "Mix parameters:" << std::endl;
    option_message << "     Seed:                   " << seed << std::endl;
    option_message << "     Max Number of Tracks:   " << max_length << std::endl;
    option_message << "     Double Drop Percentage: " << double_drop_prob << std::endl;
    option_message << "     Break Down Percentage:  " << breakdown_prob << std::endl;
    option_message << "     Tempo:                  " << output_tempo << " BPM" << std::endl;
    std::cout << option_message.str() << std::endl;

    srand(seed);

    // check environment and inputs exist
    bool xml_file_exists;
    std::string xml_file;

    if (check_environment_exists(xml_file_exists, xml_file) != 0) {
        return 1;
    }

    if (!std::filesystem::is_directory(std::filesystem::path(input_dir_path))) {
        std::cerr << "Error input " << input_dir_path << " is not an existing directory" << std::endl;
        return 1;
    }

    if (!std::filesystem::is_directory(std::filesystem::path(output_file_path).parent_path())) {
        std::cerr << "Error output location " << output_file_path << " is not an existing directory" << std::endl;
        return 1;
    }

    // Get XML document
    pugi::xml_document doc;
    if (xml_file_exists) {
        pugi::xml_parse_result result = doc.load_file(xml_file.c_str());
        if (!result) {
            std::cerr << "Error XML parsing failed: " << result.description() << std::endl;
            return 1;
        }

    } else {
        doc = pugi::xml_document();
        doc.append_child("test");   // prevents strange pugixml segfault
    }

    std::vector<std::string> track_paths = get_track_paths(input_dir_path, max_length);

    std::vector<std::shared_ptr<tune>> tune_list = get_tunes(track_paths, input_tempo, doc, multithreaded, 0);

    if (tune_list.size() == 0) {
        std::cerr << "Error could not find any tracks suitable for mixing" << std::endl;
        return 1;
    }

    doc.save_file(xml_file.c_str());    // tunes will have been updated in document

    dj dnb_dj(tune_list);
    dnb_dj.mix(output_tempo, num_channels, double_drop_prob, breakdown_prob);
    dnb_dj.print_tracklist();

    recorder out = recorder(output_file_path);
    if (out.create_output() != 0) {
        std::cerr << "Error failed to create output file" << std::endl;
        return 1;
    }

    mixer mix = mixer(dnb_dj.get_actions(), num_channels, out.get_frame_data_size());

    for (int i = 0; i < num_channels; i++) {
        mix.connect_channel(new channel());
    }

    out.connect(&mix);
    std::cout << std::endl << "Performing!" << std::endl << std::endl;
    out.run();
    std::cout << "Finished" << std::endl;
 }