#include <recorder.h>

#undef av_err2str
#define av_err2str(errnum)                                                     \
  av_make_error_string((char *)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE),     \
                       AV_ERROR_MAX_STRING_SIZE, errnum)

void recorder::interleaved_to_planar(float *in_samples, float **out_samples,
                                     int length) {
  for (int i = 0; i < length; i++) {
    out_samples[i % 2][i / 2] = in_samples[i];
  }
}

recorder::recorder(std::string path) : m_path(path) {
  m_frame_size = 2048;
  m_source = nullptr;
  m_stream = nullptr;
  m_format_ctx = nullptr;
  m_codec_ctx = nullptr;
  m_codec = nullptr;
}

void recorder::connect(mixer *input) { m_source = input; }

int recorder::create_output() {
  avformat_alloc_output_context2(&m_format_ctx, nullptr, nullptr,
                                 m_path.c_str());
  if (!m_format_ctx) {
    std::cout << "Could not create output context" << std::endl;
    return 1;
  }

  m_stream = avformat_new_stream(m_format_ctx, nullptr);
  if (!m_stream) {
    std::cout << "Failed allocating output stream" << std::endl;
    return 1;
  }

  if (!(m_codec = avcodec_find_encoder(AV_CODEC_ID_MP3))) {
    std::cout << "could not find encoder" << std::endl;
    return 1;
  }

  m_codec_ctx = avcodec_alloc_context3(m_codec);
  if (!m_codec_ctx) {
    std::cout << "out of memory" << std::endl;
    return 1;
  }

  m_codec_ctx->sample_rate = 44100;
  m_codec_ctx->channel_layout = 3;
  m_codec_ctx->channels = 2;
  m_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  m_codec_ctx->time_base = (AVRational){1, m_codec_ctx->sample_rate};
  m_codec_ctx->bit_rate = 320000;

  if (m_format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    m_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  int ret = avcodec_parameters_from_context(m_stream->codecpar, m_codec_ctx);
  if (ret < 0) {
    std::cout << "Failed to copy encoder parameters to output stream"
              << std::endl;
  }
  m_stream->time_base = m_codec_ctx->time_base;
  ret = avcodec_open2(m_codec_ctx, m_codec, NULL);
  if (ret < 0) {
    std::cout << "Could not open output file" << std::endl;
  }
  std::cout << "encoder frame size" << std::to_string(m_codec_ctx->frame_size)
            << std::endl;
  ret = avio_open(&m_format_ctx->pb, m_path.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    std::cout << "Could not open output file" << std::endl;
  }
  ret = avformat_write_header(m_format_ctx, nullptr);
  if (ret < 0) {
    std::cout << "Error occurred when opening output file" << std::endl;
  }

  std::cout << "successfully created ouput context " << m_path << std::endl;

  std::cout << "m_codec_ctx sample rate: "
            << std::to_string(m_codec_ctx->sample_rate) << std::endl;
  std::cout << "m_codec_ctx channel_layout: "
            << std::to_string(m_codec_ctx->channel_layout) << std::endl;
  std::cout << "m_codec_ctx channels: " << std::to_string(m_codec_ctx->channels)
            << std::endl;
  std::cout << "m_codec_ctx bit_rate: " << std::to_string(m_codec_ctx->bit_rate)
            << std::endl;

  return 0;
}

int recorder::get_frame_data_size() { return m_codec_ctx->frame_size * 2; }

int recorder::run() {
  AVFrame *enc_frame = nullptr;
  AVPacket enc_pkt;

  int err;
  int frame_data_length = m_codec_ctx->frame_size * 2;

  float *samples = new float[frame_data_length];
  enc_frame = av_frame_alloc();
  av_init_packet(&enc_pkt);

  float **planar_samples = new float *[2];
  for (int i = 0; i < 2; i++) {
    planar_samples[i] = new float[m_codec_ctx->frame_size];
  }

  enc_frame->nb_samples = m_codec_ctx->frame_size;
  enc_frame->format = m_codec_ctx->sample_fmt;
  enc_frame->channel_layout = m_codec_ctx->channel_layout;

  err = av_frame_get_buffer(enc_frame, 0);
  if (err < 0) {
    std::cout << "Could not allocate audio data buffers" << std::endl;
    return 1;
  }

  while (m_source->read(samples, frame_data_length) == frame_data_length) {
    interleaved_to_planar(samples, planar_samples, frame_data_length);
    auto frame_data = (uint8_t **)(planar_samples);
    err = av_frame_make_writable(enc_frame);
    if (err < 0) {
      std::cout << "frame cannot be made writeable" << std::endl;
      return 1;
    }
    enc_frame->data[0] = frame_data[0];
    enc_frame->data[1] = frame_data[1];
    enc_frame->linesize[0] = frame_data_length * 4;
    enc_frame->linesize[1] = frame_data_length * 4;

    err = avcodec_send_frame(m_codec_ctx, enc_frame);

    if (err < 0) {
      std::cout << "Could not send packet for encoding: " << av_err2str(err)
                << std::endl;
      std::cout << "Frame size: " << std::to_string(enc_frame->nb_samples)
                << std::endl;
      return 1;
    }

    while (avcodec_receive_packet(m_codec_ctx, &enc_pkt) >= 0) {
      enc_pkt.stream_index = 0;

      err = av_write_frame(m_format_ctx, &enc_pkt);
    }
  }
  av_frame_free(&enc_frame);
  av_write_trailer(m_format_ctx);

  return 0;
}