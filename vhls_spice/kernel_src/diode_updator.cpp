#include "typedef.hpp"

/*
Class VtV (Vector times Vector)
local variables:
	A_local[M]:			local saved matrix.
	last_val[M]:		variable in systolic pe, hold the tlast signal from its previous stage.
	user_val[M]:		variable in systolic pe, hold the tuser signal from its previous stage.
	valid_val[M]:		variable in systolic pe, hold the tvalid signal from its previous stage.
	acc[M]:				variable in systolic pe. The accumulator to sum up A[*][i]x[i]
	idx[M]:				variable in systolic pe, points to corresponding column of a row.
	row_counter:		counter for reload, counts how many data has been loaded in a row.
	col_counter:		counter for reload, counts how many rows has been loaded in the matrix.
*/
class VtV{
public:
	d_htype A_local[M];
	logic last_val;
	logic user_val;
	logic valid_val;
	d_htype acc;
	d_htype d_val;
	unsigned char idx = 0;
	int row_counter = 0;
	int col_counter = 0;
	VtV() {}
	void inline run(logic& y_valid, dp_htype& y, logic A_valid, dp_htype A, logic x_valid, dp_htype x){
#pragma HLS INLINE
		if(A_valid){
			A_local[row_counter] = A.data;
			y_valid = 0;
			row_counter++;
			if (row_counter == M){
				row_counter = 0;
			}
		}
		else{
			d_htype din = x_valid?x.data:(d_htype)0;
			logic last = x_valid?x.last:(logic)0;
			logic user = x_valid?x.user:(logic)0;


			if(x_valid){
				if (user){
					idx = 0;
					d_htype a = A_local[idx];
					acc = a * din;
				}
				else{
					idx++;
					d_htype a = A_local[idx];
					acc += a * din;
				}
			}


			if (last == 1){
				y.data = acc;
				y_valid = 1;
				y.user = user;
				y.last = last;
			}
			else{
				y.data = 0;
				y.last = 0;
				y.user = 0;
				y_valid = 0;
			}
		}
	}
};

extern "C" {
/*
diode_updator: determine the diode status according to input circuit status x.
        ┌────────────────────┐           
        │                    │           
────A───▶                    │           
        │    diode_updator   ├─D_status─▶
───x_in─▶                    │           
        │                    │           
        └────────────────────┘           
Input:
	A:			Stream of reload data. (reload_converter.J_stream_out-->diode_updator.A)
	x_in:		Strean of data for computing; u contains source and last status. (adder_tree.y-->diode_updator.x_in)
Output:
	D_Status:	Stream of output data. Paralled NM d_htype data. (diode_updator.D_Status-->adder_tree.diode)
*/
void diode_updator(hls::stream<dp_htype>& A,hls::stream<u8_bitwise>& D_status, hls::stream<dp_htype>& x_in){
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS PIPELINE
	static logic ds[Diodes]; // local diode status register. The new status depends on the original status.
#pragma HLS ARRAY_PARTITION variable=ds dim=0 type=complete
	static int inst_counter = 0; // for index which VtV  instance  to reload.
	static VtV inst[Diodes*2]; // VtV instances.
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=inst
// Similiar to systolic_array module, please refer to that module for more information.
	logic y_valid[Diodes*2];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=y_valid
	logic A_valid[Diodes*2];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=A_valid
	logic x_valid;
	dp_htype yy[Diodes*2],AA,xx;
	dp_htype UU;
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=yy
	if(!A.empty()){
		for(int i = 0; i < Diodes*2;i++){
	#pragma HLS UNROLL
			A_valid[i] = (i == inst_counter)?True:False;
		}
		A >> AA;
		x_valid = 0;
		xx.data = 0;
		xx.last = 0;
		xx.user = 0;
		if (AA.last == True){
			inst_counter++;
			if(inst_counter == Diodes*2){
				inst_counter = 0;
			}
		}
	}
	else{

		for(int i = 0; i < Diodes*2;i++){
	#pragma HLS UNROLL
			A_valid[i] = False;
		}
		AA.data = 1;
		AA.last = 0;
		AA.user = 0;
		x_valid = x_in.read_nb(UU);
		xx.data = UU.data;
		xx.last = UU.last;
		xx.user = UU.user;
	}
	for (int i = 0; i < Diodes * 2;i++){
#pragma HLS UNROLL
		inst[i].run(y_valid[i],yy[i],A_valid[i],AA,x_valid,xx);
	}
	if (y_valid[0]){
		for (int i = 0; i < Diodes;i++){
	#pragma HLS UNROLL
				// The diode status will be on if the corresponding result is positive
				ds[i] = ds[i]?(yy[2*i + 1].data > (d_htype)(0)):(yy[2*i].data > (d_htype)(0));
			}
		u8_bitwise diode_status = 0;
		// Generate new diode status
		for (int i = 0; i < Diodes;i++){
#pragma HLS UNROLL
			if (ds[i]){
				diode_status[i] = 1;
			}
		}
		D_status.write(diode_status);

	}

}
}



