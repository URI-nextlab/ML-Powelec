#include "typedef.hpp"

extern "C" {

void reload_converter(hls::stream<d_stype>& A_stream_in, hls::stream<dp_htype>& A_stream_out, hls::stream<dp_htype>& J_stream_out){
//#pragma HLS INTERFACE m_axi port = A offset = slave bundle = A_in
//#pragma HLS INTERFACE s_axilite port = A
#pragma HLS INTERFACE ap_ctrl_none port = return
	//while(1){
		for (int nm = 0; nm < NM; nm++){
			for (int row = 0; row < M; row++){
				for (int col = 0; col < M; col++){
#pragma HLS PIPELINE
					dp_htype temp;
					d_stype dt;
					A_stream_in >> dt;
					temp.data = (d_htype)dt;
					if ((col == M - 1) && (row == M - 1)){
						temp.last = True;
					}
					else{
						temp.last = False;
					}
					temp.user = False;
					A_stream_out << temp;
				}
			}
		}
		for (int i = 0; i < Diodes * 2; i++){
			for (int col = 0; col < M;col++){
#pragma HLS PIPELINE
				dp_htype temp;
				d_stype dt;
				A_stream_in >> dt;
				temp.data = (d_htype)dt;
				if (col == M - 1){
					temp.last = True;
				}
				else{
					temp.last = False;
				}
				temp.user = False;
				J_stream_out << temp;
			}
		}
	//}
}

}
