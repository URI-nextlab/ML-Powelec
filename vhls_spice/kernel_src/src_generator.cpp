#include "typedef.hpp"

extern "C" {
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
void src_generator(d_stype src[], int sPeriod, int IT, hls::stream<d_htype>& src_stream){
#pragma HLS INTERFACE m_axi port = src offset = slave bundle = src
#pragma HLS INTERFACE s_axilite port = src
#pragma HLS INTERFACE s_axilite port = sPeriod
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	int it = 0; // redundant iteration counter for period, mod function is not hardware friendly
	for (int i = 0; i < IT; i++){
		for (int j = 0; j < M;j++){
#pragma HLS PIPELINE
			d_htype temp = (d_htype)src[it * M + j];
			src_stream << temp;
		}
		it++;
		if (it == sPeriod){ // roll back
			it = 0;
		}
	}
}

}
