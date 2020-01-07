#ifndef track_def

#include <ring_buffer.h>
#include <fstream>
#include <vamp-plugin-sdk/vamp-sdk/Plugin.h>
extern "C" {
  #include <libavformat/avformat.h>
}

class track {
	private:
		std::string m_path;
		ring_buffer m_ring_buffer;
		AVFormatContext* m_format_ctx;
		AVCodecContext* m_codec_ctx;
		AVCodec* m_codec;
		int m_audio_stream_index;
		int fill_output_buffer();
		void planar_to_interleaved(float** input_samples, float* output_samples, int length);

	public:
		track(std::string path);
		int read(float *data_ptr, int num_samples);
		int open_audio_source();
		std::string get_path();
};

#define track_def
#endif
