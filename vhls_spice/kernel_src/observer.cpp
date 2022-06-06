#include "typedef.hpp"


extern "C" {
/*
obsever:	observe one mean value and one RMS^2 value.
          ┌──────────────┐
  RMS_id──▶              │
          │              │
  Mean_id─▶              │
          │   observer   ├─ob───▶
   ─IT────▶              │
          │              │
   x_in───▶              │
          └──────────────┘
Input:
	RMS_id:		The index of the value that requires rms in the M*1 x vector
	Mean_id:	The index of the value that requires mean in the M*1 x vector
	IT:			Total iteration times
	x_in:		Stream of the x vector. (Stream_Divider.s_out*-->observer.x_in)
Output:
	ob:			Memory mapped bus. Write the two observation results back

The shift window averager is implemented as:
             ┌──────────────────────────────┐     ┌───┐
         ┌───▶   shift resgister, depth=N   ├────▶│   │
         │   └──────────────────────────────┘   - │   │    ┌────────────┐
         │                                        │   ├────▶ accumulator│
         │                                        │   │    └────────────┘
─────────┴───────────────────────────────────────▶│   │
                                                + └───┘
*/
void observer(float ob[],int RMS_id, int Mean_id, int IT, dp_stream& x_in){
#pragma HLS INTERFACE m_axi port = ob offset = slave bundle = ob_mem
#pragma HLS INTERFACE s_axilite port = ob
#pragma HLS INTERFACE s_axilite port = RMS_id
#pragma HLS INTERFACE s_axilite port = Mean_id
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	// local shift registers. Defined as static to reduce initalization latency
	static ap_shift_reg<d_htype_acc,1024> shift_reg_rms;
	static ap_shift_reg<d_htype,1024> shift_reg_mean;
	// accumulators
	d_htype_acc mean_acc = 0;
	d_htype_acc rms_acc = 0;
	for (int it = 0; it < IT; it++){ // iteration loop
		for (int m = 0; m < M; m++){
#pragma HLS PIPELINE
			dp din;
			x_in >> din;
			d_htype dt = din.data;
			d_htype_acc dts = dt * dt; // get square
			if (m == RMS_id){ // RMS
				d_htype_wide rms_temp = shift_reg_rms.shift(dts);
				// The first 1024 data from the shift resgitser come from last iteration, must be ignored
				if(it >= 1024)
					rms_acc += (dts - rms_temp);
				else
					rms_acc += dts;
			}
			if (m == Mean_id){ // Mean
				d_htype mean_temp = shift_reg_mean.shift(dt);
				if(it >= 1024)
					mean_acc += (dt - mean_temp);
				else
					mean_acc += (dt);
			}
		}
	}
	// Divide by 1024, signed shift right by 10 bits
	ob[0] = float(mean_acc >> ((ap_int<5>)10));
	ob[1] = float(rms_acc >> ((ap_int<5>)10));
}
}
