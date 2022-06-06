#include "typedef.hpp"

/*
src_generator:	create source stream
          ┌────────────────┐
────src───▶                │
          │                │
─sPeriod──▶  src_generator ├src_stream─▶
          │                │
────IT────▶                │
          └────────────────┘
Input:
	src:		Memory mapped port. Read source vector from DDR.
	sPeriod:	Source period. The period of source signal for AC-AC AC-DC converter
	IT:			Iteration time.
Output:
	src_stream:	Stream data of source. (src_generator.src_stream-->controller.src_stream)
*/
void Source_Gen(d_stype src[], int sPeriod, int IT, dp_stream& src_stream){
#pragma HLS INTERFACE m_axi port=src offset=slave bundle=src_gmem
#pragma HLS INTERFACE s_axilite port=sPeriod
#pragma HLS INTERFACE s_axilite port=IT
#pragma HLS INTERFACE s_axilite port=return
	int it = 0; // redundant iteration counter for period, mod function is not hardware friendly
Src_Gen_iteration_loop:
	for (int i = 0; i < IT; i++){
Src_Gen_col_loop:
		for (int j = 0; j < M;j++){
#pragma HLS PIPELINE
			dp temp;
			temp.data = (d_htype)src[it * M + j];
			temp.last = (j == (M - 1));
			temp.user = (j == (0));
			temp.keep = -1;
			src_stream << temp;
		}
		if (it < sPeriod - 1){ // roll back
			it++;
		}
		else{
			it = 0;
		}
	}
}
