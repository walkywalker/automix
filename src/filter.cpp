/*  Modified from Mixxx (GNU GPLv2) @Matthew Walker 01/01/21
*/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include <math.h>
#include <fstream>

#include <filter.h>

extern "C" {
	#include <fidlib/fidlib.h>
}


biquad::biquad(void) {}

void biquad::set_coefs(double sample_rate,
	                   double center_freq,
	                   double Q,
	                   double dBgain) {

    snprintf(m_spec, sizeof(m_spec), "PkBq/%.10f/%.10f", Q, dBgain);

    char spec_d[FIDSPEC_LENGTH];
    if (strlen(m_spec) < sizeof(spec_d)) {
        // Copy to dynamic-ish memory to prevent fidlib API breakage.
        strcpy(spec_d, m_spec);
        // std::cout << "spec is " << spec_d << std::endl;
        // // Copy the old coefficients into m_oldCoef
        // memcpy(m_oldCoef, coef, sizeof(coef));

        m_coef[0] = fid_design_coef(m_coef+1, m_size,
        spec_d, sample_rate, center_freq, 0, 0); // fre1, adj
        for (int i = 0; i< m_size+ 1; i++) {
        	m_coef_fl[i] = float(m_coef[i]);
        }
        
        if (0) { // compile in filter debug info
            char* desc;
            FidFilter* filt = fid_design(spec_d, sample_rate, center_freq, 0, 0, &desc);
            int delay = fid_calc_delay(filt);
            std::cout << desc << "delay:" << delay <<std::endl;
            double resp0, phase0;
            resp0 = fid_response_pha(filt, center_freq / sample_rate, &phase0);
            std::cout << "center_freq:" << center_freq << " " << resp0 << " " << phase0 <<std::endl;
            if (0) {
                double resp1, phase1;
                resp1 = fid_response_pha(filt, 0 / sample_rate, &phase1);
                std::cout << "0:" << 0 << " " << resp1 << " " << phase1 <<std::endl;
            }
            double resp2, phase2;
            resp2 = fid_response_pha(filt, center_freq / sample_rate / 2, &phase2);
            std::cout << "freq2:" << center_freq / 2 << " " << resp2 <<" " <<  phase0 <<std::endl;
            double resp3, phase3;
            resp3 = fid_response_pha(filt, center_freq / sample_rate * 2, &phase3);
            std::cout << "freq3:" << center_freq * 2 << " " << resp3 << " " << phase0 <<std::endl;
            std::cout << "freq3:" << center_freq * 2 << " " << resp3 <<" " <<  phase0 <<std::endl;
            double resp4, phase4;
            resp4 = fid_response_pha(filt, center_freq / sample_rate / 2.2, &phase4);
            std::cout << "freq4:" << center_freq / 2.2 << " " << resp2 << " " << phase0 <<std::endl;
            double resp5, phase5;
            resp5 = fid_response_pha(filt, center_freq / sample_rate * 2.2, &phase5);
            std::cout << "freq5:" << center_freq * 2.2 <<" " <<  resp3 <<  " " << phase0 <<std::endl;
            free(filt);
        }
    }
    memset(m_buf1, 0, sizeof(m_buf1));
    memset(m_buf2, 0, sizeof(m_buf2));
}

void biquad::print_coefs(void) {
	for (int i = 0; i < m_size + 1; i++) {
		std::cout << "coeff[" << std::to_string(i) << "]: " << std::to_string(m_coef_fl[i]) << std::endl; 
	}
}

void biquad::process_samples(float* samples, int frame_size) {
    for (int i = 0; i < frame_size-1; i+=2) {
        samples[i] = process(m_coef_fl, m_buf1, samples[i]);
        samples[i+1] = process(m_coef_fl, m_buf2, samples[i+1]);
    }
}


inline float biquad::process(float* coef,
                      float* buf,
                      float val) {
    float tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = coef[2] * tmp;
    iir -= coef[3] * buf[0]; fir += coef[4] * buf[0];
    fir += coef[5] * iir;
    buf[1] = iir; val = fir;
    return val;
}



bessel::bessel(void) {}

void bessel::set_coefs(double sample_rate,
                       double center_freq,
                       double Q,
                       double dBgain) {

    snprintf(m_spec, sizeof(m_spec), "BpBe4");

    char spec_d[FIDSPEC_LENGTH];
    if (strlen(m_spec) < sizeof(spec_d)) {
        // Copy to dynamic-ish memory to prevent fidlib API breakage.
        strcpy(spec_d, m_spec);
        // std::cout << "spec is " << spec_d << std::endl;
        // // Copy the old coefficients into m_oldCoef
        // memcpy(m_oldCoef, coef, sizeof(coef));

        m_coef[0] = fid_design_coef(m_coef+1, m_size,
        spec_d, sample_rate, 20, 150, 0); // fre1, adj
        for (int i = 0; i< m_size+ 1; i++) {
            m_coef_fl[i] = float(m_coef[i]);
        }
        
        if (0) {    // compile in filter debug info
            char* desc;
            FidFilter* filt = fid_design(spec_d, sample_rate, center_freq, 0, 0, &desc);
            int delay = fid_calc_delay(filt);
            std::cout << desc << "delay:" << delay <<std::endl;
            double resp0, phase0;
            resp0 = fid_response_pha(filt, center_freq / sample_rate, &phase0);
            std::cout << "center_freq:" << center_freq << " " << resp0 << " " << phase0 <<std::endl;
            if (0) {
                double resp1, phase1;
                resp1 = fid_response_pha(filt, 0 / sample_rate, &phase1);
                std::cout << "0:" << 0 << " " << resp1 << " " << phase1 <<std::endl;
            }
            double resp2, phase2;
            resp2 = fid_response_pha(filt, center_freq / sample_rate / 2, &phase2);
            std::cout << "freq2:" << center_freq / 2 << " " << resp2 <<" " <<  phase0 <<std::endl;
            double resp3, phase3;
            resp3 = fid_response_pha(filt, center_freq / sample_rate * 2, &phase3);
            std::cout << "freq3:" << center_freq * 2 << " " << resp3 << " " << phase0 <<std::endl;
            std::cout << "freq3:" << center_freq * 2 << " " << resp3 <<" " <<  phase0 <<std::endl;
            double resp4, phase4;
            resp4 = fid_response_pha(filt, center_freq / sample_rate / 2.2, &phase4);
            std::cout << "freq4:" << center_freq / 2.2 << " " << resp2 << " " << phase0 <<std::endl;
            double resp5, phase5;
            resp5 = fid_response_pha(filt, center_freq / sample_rate * 2.2, &phase5);
            std::cout << "freq5:" << center_freq * 2.2 <<" " <<  resp3 <<  " " << phase0 <<std::endl;
            free(filt);
        }
    }
    memset(m_buf1, 0, sizeof(m_buf1));
    memset(m_buf2, 0, sizeof(m_buf2));
}

void bessel::process_samples(float* samples, int frame_size) {
    for (int i = 0; i < frame_size-1; i+=2) {
        samples[i] = process(m_coef_fl, m_buf1, samples[i]);
        samples[i+1] = process(m_coef_fl, m_buf2, samples[i+1]);
    }
}

inline float bessel::process(float* coef,
                              float* buf,
                              float val) {
    float tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    buf[3] = buf[4]; buf[4] = buf[5]; buf[5] = buf[6]; buf[6] = buf[7];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val= fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += -buf[2] - buf[2];
    fir += iir;
    tmp = buf[3]; buf[3] = iir; val= fir;
    iir = val;
    iir -= coef[5] * tmp; fir = tmp;
    iir -= coef[6] * buf[4]; fir += buf[4] + buf[4];
    fir += iir;
    tmp = buf[5]; buf[5] = iir; val= fir;
    iir = val;
    iir -= coef[7] * tmp; fir = tmp;
    iir -= coef[8] * buf[6]; fir += buf[6] + buf[6];
    fir += iir;
    buf[7] = iir; val = fir;
    return val;
}
