#include "typedef.hpp"

void Stream_Join(dp_stream& x, wdp_stream& x_src, wdp_stream& x_react, wdp_stream& x_switch, wdp_stream& x_diode){
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS pipeline II=1
	wdp x_src_temp ,x_react_temp, x_switch_temp, x_diode_temp;
	dp x_temp;

	x_src >> x_src_temp;
	x_react >> x_react_temp;
	x_switch >> x_switch_temp;
	x_diode >> x_diode_temp;

	x_temp.data = (d_htype)(x_src_temp.data + x_react_temp.data + x_switch_temp.data + x_diode_temp.data);
	x_temp.last = x_src_temp.last;
	x_temp.user = x_src_temp.user;
	x_temp.keep = -1;
	x << x_temp;
}
