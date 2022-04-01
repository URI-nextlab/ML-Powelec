#include "typedef.hpp"

extern "C" {

void switch_controller(unsigned char sw[], int swPeriod, int IT, hls::stream<u8_bitwise>& sw_stream){
#pragma HLS INTERFACE m_axi port = sw offset = slave bundle = sw
#pragma HLS INTERFACE s_axilite port = sw
#pragma HLS INTERFACE s_axilite port = swPeriod
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	int addr = 0;
	for (int i = 0; i < IT; i++){
		u8_bitwise temp = (u8_bitwise)sw[addr];
		sw_stream << temp;
		addr++;
		if (addr == swPeriod){
			addr = 0;
		}
	}
}

}
