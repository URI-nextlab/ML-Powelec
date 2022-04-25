#include "typedef.hpp"

void J_reloader(d_stype J[], dp_stream& J_stream_out){
#pragma HLS INTERFACE m_axi port = J offset = slave bundle = J_in
#pragma HLS INTERFACE s_axilite port = return
		// just read
		for (int i = 0; i < (M * Diodes * 2);i++){
			dp J_temp;
			J_temp.data = (d_htype)J[i];
			J_temp.user = 0;
			J_temp.last = 0;
			J_temp.keep = -1;
			J_stream_out << J_temp;
		}
}
