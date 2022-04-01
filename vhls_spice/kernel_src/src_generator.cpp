#include "typedef.hpp"

extern "C" {

void src_generator(d_stype src[], int sPeriod, int IT, hls::stream<d_htype>& src_stream){
#pragma HLS INTERFACE m_axi port = src offset = slave bundle = src
#pragma HLS INTERFACE s_axilite port = src
#pragma HLS INTERFACE s_axilite port = sPeriod
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	int it = 0;
	for (int i = 0; i < IT; i++){
		for (int j = 0; j < M;j++){
#pragma HLS PIPELINE
			d_htype temp = (d_htype)src[it * M + j];
			src_stream << temp;
		}

		it++;
		if (it == sPeriod){
			it = 0;
		}
	}
}

}
