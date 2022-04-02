#include "typedef.hpp"

extern "C" {
/*
result_back:	A DMA module that write stream result data back to the host
                ┌───────────────────┐        
   result_stream│                   │        
        ────────▶    result_back    ├result─▶
                │                   │        
                └───────────────────┘        
Input:
	result_stream:	Stream from control module. (controller.result_stream-->result_back.result_stream)
Output:
	resultp[]:		Memory mapped AXI4 bus. Write data to the host DDR.
*/
void result_back(d_stype result[], int IT, hls::stream<d_htype>& result_stream){
#pragma HLS INTERFACE m_axi port = result offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	for (int i = 0; i < IT * M; i++){ // total number of data equals iteration times times M
		d_htype temp;
		result_stream >> temp;
		result[i] = (d_stype)temp;
	}
}

}
