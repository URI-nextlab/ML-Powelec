#include "typedef.hpp"
/*
Controller: manage the iteration loop.
        ┌─────────────┐
        │        ┌──▶s_out1─▶
        │    ┌───┘    │
 ──s_in─▶ ───●──────▶s_out2─▶
        │    └───┐    │
        │        └──▶s_out3─▶
        └─────────────┘
Input:
	x0:	Stream input
Output:
	s_out*:	Stream output
*/
void Controller(d_stype x0[], int IT, dp_stream& x_src_out, dp_stream& x_diode_j, dp_stream& x_react, dp_stream& x_diode, dp_stream&x_switch, dp_stream& x_back,  dp_stream& x_src_in,dp_stream& result_stream){
#pragma HLS INTERFACE m_axi port = x0 offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = x0
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	// read x0 in advance to avoid trigging AXI4 bus too frequently
	static ap_shift_reg<d_htype,M> x0_local;
read_x0_loop:
	for (int j = 0;j < M; j++){
#pragma HLS pipeline
		x0_local.shift((d_htype)x0[j]);
	}
Ctrl_iteratrion_loop:
	for (int i = 0; i <= IT; i++){
		if (i == 0){
x0_forward_loop:
			for (int j = 0; j < M;j++){
#pragma HLS pipeline
				dp x_temp;
				dp x_src;
				x_src_in >> x_src;
				x_temp.data = x0_local.shift(1);
				x_temp.last = (j == (M - 1));
				x_temp.user = (j == (0));
				x_temp.keep = -1;
				x_src_out << x_src;
				x_diode_j << x_temp;
				x_diode << x_temp;
				x_switch << x_temp;
				x_react << x_temp;
			}
		}
		else if (i == IT){
stopping_loop:
			for (int j = 0; j < M;j++){
#pragma HLS pipeline
				dp x_temp;
				x_back >> x_temp;
				result_stream << x_temp;
			}
		}
		else{
iteration_forwarding_loop:
			for (int j = 0; j < M;j++){
#pragma HLS pipeline
				dp x_temp;
				dp x_src;
				x_back >> x_temp;
				x_src_in >> x_src;
				result_stream << x_temp;
				x_diode_j << x_temp;
				x_src_out << x_src;
				x_diode << x_temp;
				x_switch << x_temp;
				x_react << x_temp;
			}
		}
	}
}
