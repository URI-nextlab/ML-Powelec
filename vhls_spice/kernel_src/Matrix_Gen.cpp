#include "typedef.hpp"

void Matrix_Gen(d_stype A[], u32 IT, col_stream& A_up_stream_out, col_stream& A_down_stream_out){
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=M_Gen
#pragma HLS INTERFACE s_axilite port=IT
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS aggregate variable=A_up_stream_out compact=bit
#pragma HLS aggregate variable=A_down_stream_out compact=bit
	static d_htype A_local[M][M];
#pragma HLS array_partition variable=A_local dim=1 type=complete

//	*	*	*	*	*	*	*	Memory Copy	*	*	*	*	*	*	//
	d_stype *p = A;
A_memcpy_row_loop:
	for (int row = 0; row < M; row++){
A_memcpy_col_loop:
		for (int col = 0; col < M; col++){
#pragma HLS pipeline
			A_local[row][col] = *p++;
		}
	}

//	*	*	*	*	*	*	*	Out			*	*	*	*	*	*	//
M_Gen_iteration_loop:
	for (int i = 0; i < IT; i++){
M_Gen_col_loop:
		for (int col = 0; col < M; col++){
#pragma HLS pipeline
			Col col_up_temp;
			Col col_down_temp;
M_Gen_row_loop:
			for (int row = 0; row < M; row++){
#pragma HLS unroll
				if (row < MD2)
					col_up_temp.data[row] = A_local[row][col];
				else
					col_down_temp.data[row - MD2] = A_local[row][col];
			}
			A_up_stream_out << col_up_temp;
			A_down_stream_out << col_down_temp;
		}
	}
}
