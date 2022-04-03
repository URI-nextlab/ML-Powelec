#include "typedef.hpp"

extern "C" {
/*
reload:		A DMA module, read matrix from DDR
       ┌──────────┐               
 ───A──▶  reload  ├A_stream_out─▶ 
       └──────────┘               
Input:
	A:				Memory mapped data
Output:
	A_stream_out:	Stream data. (reload.A_stream_out-->reload_converter.A_stream_in)
*/
void reload(d_stype A[], hls::stream<d_stype>& A_stream_out){
#pragma HLS INTERFACE m_axi port = A offset = slave bundle = A_in
#pragma HLS INTERFACE s_axilite port = A
#pragma HLS INTERFACE s_axilite port = return
		// just read
		for (int i = 0; i < (NM * M * M + M * Diodes * 2);i++){
			d_stype temp = A[i];
			A_stream_out << temp;
		}
}
}
