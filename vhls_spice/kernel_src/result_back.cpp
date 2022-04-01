#include "typedef.hpp"

extern "C" {

void result_back(d_stype result[], int IT, hls::stream<d_htype>& result_stream){
#pragma HLS INTERFACE m_axi port = result offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	for (int i = 0; i < IT * M; i++){
		d_htype temp;
		result_stream >> temp;
		result[i] = (d_stype)temp;
	}
}

}
