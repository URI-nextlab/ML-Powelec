#include "typedef.hpp"

extern "C" {
/*
swicth_controller:	Generate switch control signal
           ┌──────────────────────┐             
────sw─────▶                      │             
           │                      │             
─swPeriod──▶   switch_controller  ├─sw_stream──▶
           │                      │             
─────IT────▶                      │             
           └──────────────────────┘             
Input:
	sw:			Memory mapped switch control signal
	swPeriod:	Period of switch control signal
	IT:			Iteration times.
Output:
	sw_stream:	Stream of switch control signal. (switch_controller.sw_stream-->adder_tree.sw)
*/
void switch_controller(unsigned char sw[], int swPeriod, int IT, hls::stream<u8_bitwise>& sw_stream){
#pragma HLS INTERFACE m_axi port = sw offset = slave bundle = sw
#pragma HLS INTERFACE s_axilite port = sw
#pragma HLS INTERFACE s_axilite port = swPeriod
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	int addr = 0; // read address. Redundant counter to avoid using mod function.
	for (int i = 0; i < IT; i++){
		u8_bitwise temp = (u8_bitwise)sw[addr];
		sw_stream << temp;
		addr++;
		if (addr == swPeriod){ // roll back
			addr = 0;
		}
	}
}

}
