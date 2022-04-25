#include "typedef.hpp"

void Diode_M_Gen(d_stype A[], u32 IT, Ctrl_Stream& S_out, Ctrl_Stream& S_in,col_stream& A_up_stream_out,col_stream& A_down_stream_out){
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=M_Gen
#pragma HLS INTERFACE s_axilite port=IT
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS aggregate variable=A_up_stream_out compact=bit
#pragma HLS aggregate variable=A_down_stream_out compact=bit
	static d_htype A_local[2 * M * Diodes][M];
#pragma HLS array_partition variable=A_local dim=1 type=complete

	u16_bitwise diode_status_local;

//	*	*	*	*	*	*	*	Memory Copy	*	*	*	*	*	*	//
	d_stype *p = A;
A_memcpy_row_loop:
	for (int row = 0; row < 2 * M * Diodes; row++){
A_memcpy_col_loop:
		for (int col = 0; col < M; col++){
#pragma HLS pipeline
			A_local[row][col] = *p++;
		}
	}

//	*	*	*	*	*	*	*	Out			*	*	*	*	*	*	//
D_M_Gen_iteration_loop:
	for (int i = 0; i < IT; i++){
#pragma HLS pipeline off
		if (i == 0){
			diode_status_local = 0;
			S_out << diode_status_local;
		}
		else{
			diode_status_local = S_in.read();
			S_out << diode_status_local;
		}
D_M_Gen_col_loop:
		for (int col = 0; col < M; col++){
#pragma HLS pipeline
			Col col_up_temp;
			Col col_down_temp;
D_M_Gen_row_loop:
			for (int row = 0; row < M; row++){
#pragma HLS unroll
				d_htype_wide temp = 0;
D_M_Gen_D_loop:
				for (int d = 0; d < Diodes;d++){
#pragma HLS unroll
					temp += diode_status_local[d]?(A_local[row + d * 2 * M + M][col]):(A_local[row + d * 2 * M][col]);
				}
				if (row < MD2)
					col_up_temp.data[row] = temp;
				else
					col_down_temp.data[row -  MD2] = temp;
			}
			A_up_stream_out << col_up_temp;
			A_down_stream_out << col_down_temp;
		}
	}
	S_in.read();
}
