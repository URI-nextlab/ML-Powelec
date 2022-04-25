#include "typedef.hpp"

typedef ap_int<2> J_flag;

void Diode_Controller(dp_stream& J_stream, dp_stream& x, Ctrl_Stream& S_out, Ctrl_Stream& S_in){
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS pipeline
	static J_flag J_local[Diodes * 2][M];
#pragma HLS array_partition variable=J_local dim=1 type=complete
	static s16 J_row_counter;
	static s16 J_col_counter;
	static d_htype flag_val[Diodes * 2];
	static s16 x_counter;
#pragma HLS array_partition variable=flag_val dim=1 type=complete
	static bit data_valid;
	if (!J_stream.empty()){	// J has higher priority
		dp J_temp;
		J_stream >> J_temp;
//		printf("Get %f\n",(float)J_temp.data);
		if (J_temp.data > 0.5){
			J_local[J_row_counter][J_col_counter] = 1;
		}
		else if(J_temp.data < -0.5){
			J_local[J_row_counter][J_col_counter] = -1;
		}
		else{
			J_local[J_row_counter][J_col_counter] = 0;
		}
		if(J_col_counter < M -1){
			J_col_counter++;
		}
		else{
			J_col_counter = 0;
			if (J_row_counter < Diodes * 2 - 1){
				J_row_counter++;
			}
			else{
				J_row_counter = 0;
//				for (int i = 0; i < Diodes * 2;i++){
//					printf("Row %d:", i);
//					for (int j = 0; j < M;j++){
//						printf ("%d\t",(int)J_local[i][j]);
//					}
//					printf("\n");
//				}
			}
		}
	}
	else if ((J_row_counter == 0) && (J_col_counter == 0)){ //make sure J is complete
		dp x_temp;
		bool data_valid;
		bool x_valid = x.read_nb(x_temp);
		if (x_valid){
//			printf("Get x!\n");
			for (s16 i = 0; i < Diodes * 2; i++){
#pragma HLS unroll
				if(x_temp.user){
					if (J_local[i][x_counter] == 1){
						flag_val[i] = x_temp.data;
					}
					else if (J_local[i][x_counter] == -1){
						flag_val[i] = -x_temp.data;
					}
					else{
						flag_val[i] = 0;
					}
				}
				else{
					if (J_local[i][x_counter] == 1){
						flag_val[i] += x_temp.data;
					}
					else if (J_local[i][x_counter] == -1){
						flag_val[i] -= x_temp.data;
					}
				}
			}
			if(x_temp.last){
				u16_bitwise ctrl_temp = 0;
				u16_bitwise last_state;
				S_in >> last_state;
				for (s16 i = 0; i < 16; i++){
#pragma HLS unroll
					if (i < Diodes){
						if (last_state[i] ==  1){
							ctrl_temp[i] = flag_val[2 * i + 1] > 0;
						}
						else{
							ctrl_temp[i] = flag_val[2 * i] > 0;
						}
					}
					else{
						ctrl_temp[i] = 0;
					}
				}
				S_out << ctrl_temp;
			}
			if(x_counter < M - 1){
				x_counter++;
			}
			else{
				x_counter = 0;
			}
//			for (int i = 0; i < Diodes * 2; i++){
//				printf("flag_val %d = %f\n",i, (float)flag_val[i]);
//			}
		}
	}

}
