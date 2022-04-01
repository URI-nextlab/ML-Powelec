#include "typedef.hpp"

extern "C" {

void reload(d_stype A[], hls::stream<d_stype>& A_stream_out){
#pragma HLS INTERFACE m_axi port = A offset = slave bundle = A_in
#pragma HLS INTERFACE s_axilite port = A
#pragma HLS INTERFACE s_axilite port = return
		for (int i = 0; i < (NM * M * M + M * Diodes * 2);i++){
			d_stype temp = A[i];
			A_stream_out << temp;
		}
}

}
