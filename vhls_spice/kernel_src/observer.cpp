#include "typedef.hpp"
typedef ap_fixed<W*2,IW*4> d_wtype;
extern "C" {
// , hls::stream<dp_htype>& x_forward
void observer(float ob[],int RMS_id, int Mean_id, int IT, hls::stream<dp_htype>& x_in){
#pragma HLS INTERFACE m_axi port = ob offset = slave bundle = ob_mem
#pragma HLS INTERFACE s_axilite port = ob
#pragma HLS INTERFACE s_axilite port = RMS_id
#pragma HLS INTERFACE s_axilite port = Mean_id
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	int rms_idx = RMS_id;
	int mean_id = Mean_id;
	static ap_shift_reg<d_wtype,1024> shift_reg_rms;
	static ap_shift_reg<d_htype,1024> shift_reg_mean;
	d_wtype mean_acc = 0;
	d_wtype rms_acc = 0;
	for (int it = 0; it < IT; it++){
		for (int m = 0; m < M; m++){
#pragma HLS PIPELINE
			dp_htype din;
			x_in >> din;
			//x_forward << din;
			d_htype dt = din.data;
			d_wtype dts = dt * dt;
			if (m == RMS_id){
				d_wtype rms_temp = shift_reg_rms.shift(dts);
				if(it >= 1024)
					rms_acc += (dts - rms_temp);
				else
					rms_acc += dts;
			}
			if (m == Mean_id){
				d_htype mean_temp = shift_reg_mean.shift(dt);
				if(it >= 1024)
					mean_acc += (dt - mean_temp);
				else
					mean_acc += (dt);
			}
		}
	}
	ob[0] = float(mean_acc >> ((ap_int<5>)10));
	ob[1] = float(rms_acc >> ((ap_int<5>)10));
}
}
