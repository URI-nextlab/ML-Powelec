#include "typedef.hpp"

void J_reloader(d_stype A_src[], d_stype A_react[], d_stype J[], d_stream& A_src_stream, d_stream& A_react_stream,dp_stream& J_stream_out){
#pragma HLS INTERFACE m_axi port = A_src offset = slave bundle = A_src_in
#pragma HLS INTERFACE m_axi port = A_react offset = slave bundle = A_react_in
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

		for (int j = 0; j < (M * M);j++){
			d_htype temp = (d_htype)A_src[j];
			A_src_stream << temp;
		}


		for (int k = 0; k < (M * M);k++){
			d_htype temp = (d_htype)A_react[k];
			A_react_stream << temp;
		}
}
