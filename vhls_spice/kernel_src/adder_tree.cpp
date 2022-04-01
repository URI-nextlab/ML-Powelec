#include "typedef.hpp"

extern "C" {
/*
adder_tree: sum up the results from systolic_array according to the switch and diode control signal.
                ┌─────────────────┐          
      ────sw────▶                 │          
                │                 │          
 ─ ─ ▶──diode───▶    adder_tree   ├────y────▶
│               │                 │     │  │ 
     ▲────x─────▶                 │          
│               └─────────────────┘     │  │ 
     │                                       
│                                       │  │ 
     │─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─    
│                                          │ 
 ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ 
Input:
	x:			Stream of NM of d_htype variables. (systolic_array.y-->adder_tree.x)
	sw:			Stream of switch control signals. Each bit handles one switch. 0 for off, 1 for on. (switch_controller.sw_stream-->adder_tree.sw)
	diode:		Stream of diode control signals. Each bit handles one diode. 0 for off, 1 for on. (diode_updator.D_status-->adder_tree.diode)
Output:
	y:			Stream of summed up data.
				Sort of the NM numbers: S is the total number of switches
					(0: source response)
					(1: L,C, response)
					(2: S1 off response)
					(3: S1 on response)
					...
					(2+S*2:   D1 off response)
					(2+S*2+1: D1 on response)
					...
*/
void adder_tree(hls::stream<dp_htype>& y, hls::stream<dp_dharray>& x, hls::stream<u8_bitwise>& sw, hls::stream<u8_bitwise>& diode){
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE
	static u8_bitwise sw_status;
	static u8_bitwise diode_status;
	d_htype local_data[1 + S]; // register input data
#pragma HLS ARRAY_PARTITION variable=local_data dim=0 type=complete
	dp_dharray din;
	logic valid = x.read_nb(din); // non-block read, keep the kernel running
	if(valid){
		if (din.user){ // get new diode status once detect a first data package from x
			sw >> sw_status;
		}
		if (din.user && (!diode.empty())){ // update the diode if possible (as the initial status is not clear)
			diode_status = diode.read(); // !!!it should not be written into the previous if(din.user), the if happens in parallel rather than serial.
		}
		local_data[0] = din.data[0] + din.data[1]; // do one add. or this stage of pipeline has nothing to do.
		for (int i = 0; i < S; i++){
#pragma HLS unroll
			if (i < SWs){
				local_data[i + 1] = (sw_status[i])?din.data[2 + i * 2 + 1]:din.data[2 + i * 2]; // 1 for on, 2 + i * 2 + 1
			}
			else{
				local_data[i + 1] = (diode_status[i - SWs]))?din.data[2 + i * 2 + 1]:din.data[2 + i * 2];
			}
		}
		dp_htype dout;
		d_htype dot = 0;
		// unrolled adder tree. Vitis automativally make a tree adder structure.
		for (int i = 0;i < S + 1;i++){
#pragma HLS UNROLL
			dot += local_data[i];
		}
		dout.data = dot;
		dout.last = din.last;
		dout.user = din.user;
		y.write(dout);
	}

}
}
