#ifndef mix_def
#include "mixer.h"
#define mix_def
#endif

class recorder {
	private:
		AVFormatContext* m_format_ctx;
		AVCodecContext* m_codec_ctx;
		AVCodec* m_codec;
		AVStream *m_stream;
		mixer* m_source;
		int m_frame_size;
		void interleaved_to_planar(float* in_samples, float** out_samples, int length);
	public:
		std::string m_path;
		recorder(std::string path);
		void connect(mixer* input);
		int create_output();
		int run();
		int get_frame_data_size();
};