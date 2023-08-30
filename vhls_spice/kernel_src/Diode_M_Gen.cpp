#include "typedef.hpp"

void Diode_M_Gen(d_stype A[], u8 idx_ref[], u32 IT, Ctrl_Stream& S_out, Ctrl_Stream& S_in,col_stream& A_up_stream_out,col_stream& A_down_stream_out){
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=M_Gen
#pragma HLS INTERFACE m_axi port=idx_ref offset=slave bundle=idx_ref_gen_mem
#pragma HLS INTERFACE s_axilite port=IT
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS aggregate variable=A_up_stream_out compact=bit
#pragma HLS aggregate variable=A_down_stream_out compact=bit
	static d_htype A_local[2 * M][M];
#pragma HLS array_partition variable=A_local dim=1 type=complete
	static u8 col_idx_ref[M];
	u16_bitwise diode_status_local;

//	*	*	*	*	*	*	*	Memory Copy	*	*	*	*	*	*	//
	d_stype *p = A;
A_memcpy_row_loop:
	for (int row = 0; row < 2 * M; row++){
A_memcpy_col_loop:
		for (int col = 0; col < M; col++){
#pragma HLS pipeline
			A_local[row][col] = *p++;
		}
	}
	for (int i = 0; i < M; i++){
		col_idx_ref[i] = idx_ref[i];
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
				d_htype temp = diode_status_local[col_idx_ref[col]]?(A_local[row + M][col]):(A_local[row][col]);
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
