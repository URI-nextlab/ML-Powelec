#include "typedef.hpp"

void systolic_array_switch(d_stype A[][M], int IT, x_picked_stream& xp_stream, wdp_stream& y,Ctrl_Stream& sw_in){
#pragma HLS interface s_axilite port=return
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=SAS_Mgen
#pragma HLS INTERFACE s_axilite port=IT
//	*	*	*	*	Define variables for systolic array	*	*	*	*	//
	// local shift regitser of A
	static d_htype A_local[M][4 * Switches];
#pragma HLS array_partition variable=A_local dim=2 type=complete
	static d_htype_wide accumulator[M];
#pragma HLS array_partition variable=accumulator dim=1 type=complete
	for (int j = 0; j < 4 * Switches;j++){
		for (int i = 0; i < M;i++){
			A_local[i][j] = A[j][i];
		}
	}
	for (int it = 0; it < IT; it++){
		u16_bitwise switch_status;
		sw_in >> switch_status;
		x_picked x_temp;
		xp_stream >> x_temp;
y_s_out_loop:
		for (int i = 0; i < M;i++){
#pragma HLS pipeline
			d_htype y_temp[Switches][2];
			for (int s = 0; s < Switches; s++){
				#pragma HLS unroll
				y_temp[s][0] = x_temp.data[s * 4] * A_local[i][s * 4] + x_temp.data[s * 4 + 1] * A_local[i][s * 4 + 1];
				y_temp[s][1] = x_temp.data[s * 4 + 2] * A_local[i][s * 4 + 2] + x_temp.data[s * 4 + 3] * A_local[i][s * 4 + 3];
			}
			d_htype_wide y_wide = 0;
			
			for (int s = 0; s < Switches; s++){
				#pragma HLS unroll
				y_wide += switch_status[s]?y_temp[s][1]:y_temp[s][0];
			}
			wdp y_out;
			y_out.data = y_wide;
			y_out.last = i == (M - 1);
			y_out.user = i == 0;
			y << y_out;

		}
	}
}
