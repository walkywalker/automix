#ifndef filter_def

#define FIDSPEC_LENGTH 40
#define FILT_SIZE 5

class biquad {
	private:
		int m_size = FILT_SIZE;
		double m_coef[FILT_SIZE + 1];
		float  m_coef_fl[FILT_SIZE + 1];
		float m_buf1[FILT_SIZE];
		float m_buf2[FILT_SIZE];
		char m_spec[FIDSPEC_LENGTH];
	public:
		biquad();
		void set_coefs(double sample_rate,
	                   double center_freq,
	                   double Q,
	                   double dBgain);
		void print_coefs(void);
		void process_samples(float* samples, int frame_size);
		float process(float* coef,
		                      float* buf,
		                      float val);
};

class bessel: public biquad {
	private:
		int m_size = 8;
		double m_coef[8 + 1];
		float  m_coef_fl[8 + 1];
		float m_buf1[8];
		float m_buf2[8];
		char m_spec[FIDSPEC_LENGTH];
	public:
		bessel();
		void set_coefs(double sample_rate,
	                   double center_freq,
	                   double Q,
	                   double dBgain);
		void print_coefs(void);
		void process_samples(float* samples, int frame_size);
		float process(float* coef,
		                      float* buf,
		                      float val);
};
#define filter_def
#endif
