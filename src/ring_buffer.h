#ifndef ring_def

class ring_buffer {
	private:
		float* m_data;
		int m_size;
		int m_rdptr;
		int m_wrptr;
		int m_space;
	public:
		ring_buffer(int size);
		~ring_buffer();
		bool empty();
		int read(float *data_ptr, int num_samples);
		int write(float *data_ptr, int num_samples);
		int get_size();
		int get_space();
		int get_read_space();
};
#define ring_def
#endif
