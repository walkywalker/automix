#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"
#include "track.h"

extern "C" {
#include <libavformat/avformat.h>
#include "libavutil/audio_fifo.h"
}

#define BUF_SIZE 20480

#undef av_err2str
#define av_err2str(errnum)                                                     \
  av_make_error_string((char *)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE),     \
                       AV_ERROR_MAX_STRING_SIZE, errnum)

track::track(std::string path) : m_ring_buffer(BUF_SIZE) {
  m_path = path; // seems this is needed
  m_format_ctx = nullptr;
  m_codec_ctx = nullptr;
  m_codec = nullptr;
  m_audio_stream_index = 0;
}

void track::planar_to_interleaved(float **input_samples, float *output_samples,
                                  int length) {
  for (int i = 0; i < length; i++) {
    output_samples[i] = input_samples[i % 2][i / 2];
  }
}

int track::read(float *data_ptr, int num_samples) {
  if (m_ring_buffer.read(data_ptr, num_samples) > 0) {
    return num_samples;
  } else if (fill_output_buffer() > 0) {
    std::cout << m_path << " failed to fill output buffer" << std::endl;
    return 0;
  }
  if (m_ring_buffer.read(data_ptr, num_samples) > 0) {
    return num_samples;
  } else {
    std::cout << "failed to read from track output buffer " << m_path
              << std::endl;
    return 0;
  }
}

int track::open_audio_source() {
  int error;

  error = avformat_open_input(&m_format_ctx, m_path.c_str(), nullptr, nullptr);

  if (error < 0) {
    std::cout << "Error opening file " << m_path << " " << av_err2str(error)
              << std::endl;
    return 1;
  }

  if (avformat_find_stream_info(m_format_ctx, nullptr) != 0) {
    std::cout << "Error finding stream info" << std::endl;
    return 1;
  }

  av_dump_format(m_format_ctx, 0, m_path.c_str(), 0);
  m_audio_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_AUDIO,
                                             -1, -1, &m_codec, 0);

  if (m_audio_stream_index == AVERROR_STREAM_NOT_FOUND) {
    std::cout << "Error finding audio stream" << std::endl;
    return 1;
  } else if (!m_codec) {
    std::cout << "Error finding audio codec" << std::endl;
    return 1;
  }

  m_codec_ctx = avcodec_alloc_context3(m_codec);

  if (!m_codec_ctx) {
    std::cout << "failed to allocate codec ctx" << std::endl;
    avformat_close_input(&m_format_ctx);
    return 1;
  }

  std::cout << m_path << ": Using codec " << m_codec->name << ", stream "
            << std::to_string(m_audio_stream_index) << std::endl;

  if (avcodec_parameters_to_context(
          m_codec_ctx, m_format_ctx->streams[m_audio_stream_index]->codecpar) <
      0) {
    std::cout << "Failed to set parameters" << std::endl;
    avformat_close_input(&m_format_ctx);
    avcodec_free_context(&m_codec_ctx);
    return 1;
  }

  if (m_codec_ctx->sample_fmt != AV_SAMPLE_FMT_FLTP) {
    std::cout << "Error formas is not AV_SAMPLE_FMT_FLTP" << std::endl;
    return 1; // only support this for now
  }

  if (avcodec_open2(m_codec_ctx, m_codec, NULL) < 0) {
    std::cout << "Error cannot open m_codec" << std::endl;
    return 1;
  }

  std::cout << "	m_codec_ctx sample rate: "
            << std::to_string(m_codec_ctx->sample_rate) << std::endl;
  std::cout << "	m_codec_ctx channel_layout: "
            << std::to_string(m_codec_ctx->channel_layout) << std::endl;
  std::cout << "	m_codec_ctx channels: "
            << std::to_string(m_codec_ctx->channels) << std::endl;
  std::cout << "	m_codec_ctx bit_rate: "
            << std::to_string(m_codec_ctx->bit_rate) << std::endl;

  return 0;
}

int track::fill_output_buffer() {
  int error;
  AVFrame *dec_frame = av_frame_alloc();
  AVPacket dec_pkt;
  av_init_packet(&dec_pkt);

  int frame_data_length = m_codec_ctx->frame_size * 2;
  float *dec_samples = new float[frame_data_length];

  while (m_ring_buffer.get_space() > frame_data_length) {
    error = av_read_frame(m_format_ctx, &dec_pkt);
    if (error < 0) {
      std::cout << "error reading frame from track " << m_path << std::endl;
      return 1;
    }
    if (dec_pkt.stream_index == m_audio_stream_index) {
      error = avcodec_send_packet(m_codec_ctx, &dec_pkt);
      if (error == AVERROR(EAGAIN)) {
        std::cout << "Decoder can not take packets rn" << std::endl;
      } else if (error < 0) {
        std::cout << "Failed to send the dec_pkt to the decoder" << std::endl;
        return 1;
      }
      while (avcodec_receive_frame(m_codec_ctx, dec_frame) >= 0) {
        auto samples = (float **)(dec_frame->data);
        planar_to_interleaved(samples, dec_samples, frame_data_length);
        if (m_ring_buffer.write(dec_samples, frame_data_length) !=
            frame_data_length) {
          std::cout << "track output buffer overflow" << std::endl;
          return 1;
        }
      }
    }
  }
  delete[] dec_samples;
  return 0;
}

std::string track::get_path() { return m_path; }
