#include "typedef.hpp"

void Switch_Controller(unsigned short sw[], int sPeriod, u32 IT, Ctrl_Stream& sw_stream_out){
#pragma HLS INTERFACE m_axi port=sw offset=slave bundle=sw_Gen
#pragma HLS INTERFACE s_axilite port=sPeriod
#pragma HLS INTERFACE s_axilite port=IT
#pragma HLS INTERFACE s_axilite port=return
	int addr = 0; // read address. Redundant counter to avoid using mod function.
	for (int i = 0; i < IT; i++){
		u16_bitwise temp = (u16_bitwise)sw[addr];
		sw_stream_out << temp;
		if (addr < sPeriod - 1){
			addr++;
		}
		else{
			addr = 0;
		}
	}
}
