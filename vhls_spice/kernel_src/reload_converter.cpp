#include "typedef.hpp"

extern "C" {
/*
reload_converter:	Convert bare data stream into formated packages to systolic_array and diode_updator
               ┌─────────────────┐                
               │                 │──A_stream_out─▶
 A_stream_in───▶ reload_converter│                
               │                 │──J_stream_out─▶
               └─────────────────┘                
Input:
	A_stream_in:	Stream of bare data from reload DMA. (reload.A_stream_out-->reload_converter.A_stream_in)
Output:
	A_stream_out:	Stream of packaged data to systolic_array. (reload_converter.A_stream_out-->systolic_array.A)
	J_stream_out:	Stream of packaged data to diode_updator. (reload_converter.A_stream_out-->diode_updator.A)
*/
void reload_converter(hls::stream<d_stype>& A_stream_in, hls::stream<dp_htype>& A_stream_out, hls::stream<dp_htype>& J_stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
	// reload systolic_array first
	for (int nm = 0; nm < NM; nm++){
		for (int row = 0; row < M; row++){
			for (int col = 0; col < M; col++){
#pragma HLS PIPELINE
				dp_htype temp;
				d_stype ds_temp;
				A_stream_in >> ds_temp;
				temp.data = (d_htype)ds_temp;
				// create last signal at the end of matrix
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
	// reload diode_updator second
	for (int i = 0; i < Diodes * 2; i++){
		for (int col = 0; col < M;col++){
#pragma HLS PIPELINE
			dp_htype temp;
			d_stype ds_temp;
			A_stream_in >> ds_temp;
			temp.data = (d_htype)ds_temp;
			// create last signal at the end of vector
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
}
}
