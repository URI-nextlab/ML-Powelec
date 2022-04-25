#include "typedef.hpp"

void systolic_array(col_stream &A_up, col_stream &A_down, dp_stream& x, wdp_stream& y){
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS aggregate variable=A_up compact=bit
#pragma HLS aggregate variable=A_down compact=bit
#pragma HLS pipeline ii=1

//	*	*	*	*	Define variables for systolic array	*	*	*	*	//
	// local shift regitser of A
	static hls::stream<d_htype,M*3> A_local[M - 1];
#pragma HLS array_partition variable=A_local dim=1 type=complete
	static dp x_local[M];
#pragma HLS array_partition variable=x_local dim=1 type=complete
	static d_htype_wide accumulator[M];
#pragma HLS array_partition variable=accumulator dim=1 type=complete
	static bit data_valid[M];
#pragma HLS array_partition variable=data_valid dim=1 type=complete
	static bit valid_local[M];
#pragma HLS array_partition variable=valid_local dim=1 type=complete
	static u8 out_pointer;

	static bool ready;


//	*	*	*	*	*	*	*	Input logic		*	*	*	*	*	*	//
	bool x_valid;
	dp x_temp;
	Col A_up_temp;
	Col A_down_temp;
	//SA_dp SA_temp;
	if ((!A_up.empty()) && (!A_down.empty()) && (!x.empty())){
		A_up >> A_up_temp;
		A_down >> A_down_temp;
		x >> x_temp;
		x_valid = 1;
push_A_loop:
		for (u32 i = 1; i < M; i++){
#pragma HLS unroll
			if (i < MD2){
				A_local[i - 1].write(A_up_temp.data[i]);
			}
			else{
				A_local[i - 1].write(A_down_temp.data[i - MD2]);
			}
		}
	}
	else{
		x_valid = 0;
	}


//	*	*	*	*	*	*	*	Output logic	*	*	*	*	*	*	//
	if (data_valid[out_pointer] == 1){
		wdp y_temp;
		y_temp.data = accumulator[out_pointer];
		y_temp.last = (out_pointer == (M - 1));
		y_temp.user = (out_pointer == (0));
		y_temp.keep = -1;
		y << y_temp;
		if (out_pointer < M - 1){
			out_pointer++;
		}
		else{
			out_pointer = 0;
		}
	}
	//ready = !A_local[0].empty();
//	*	*	*	*	*	*	*	Compute logic	*	*	*	*	*	*	//
systolic_array_gen_loop:
	for (int i = M - 1; i >= 0; i--){
#pragma HLS unroll
		if (i == 0){
			if(x_valid){
				d_htype a = A_up_temp.data[0];
				if (x_temp.user == 1){
					accumulator[i] = a * x_temp.data;
					data_valid[i] = 0;
				}
				else if (x_temp.last == 1){
					accumulator[i] += a * x_temp.data;
					data_valid[i] = 1;
				}
				else{
					accumulator[i] += a * x_temp.data;
					data_valid[i] = 0;
				}
			}
			else{
				data_valid[i] = 0;
			}
			valid_local[i] = x_valid;
			x_local[i] = x_temp;
		}
		else{
			if (valid_local[i - 1]){
				d_htype a = A_local[i - 1].read();
				if (x_local[i - 1].user == 1){
					accumulator[i] = a * x_local[i - 1].data;
					data_valid[i] = 0;
				}
				else if (x_local[i - 1].last == 1){
					accumulator[i] += a * x_local[i - 1].data;
					data_valid[i] = 1;
				}
				else{
					accumulator[i] += a * x_local[i - 1].data;
					data_valid[i] = 0;
				}
			}
			else{
				data_valid[i] = 0;
			}
			valid_local[i] = valid_local[i - 1];
			x_local[i] = x_local[i - 1];
		} // end if i
	}

}
