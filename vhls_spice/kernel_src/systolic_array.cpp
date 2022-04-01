#include "typedef.hpp"

/*
Class MtV (Matrix times Vector)
local variables:
	A_local[M][M]:		local saved matrix, partitioned in the first dimenstion (M rows is available at the same time).
	last_val[M]:		variable in systolic pe, hold the tlast signal from its previous stage.
	user_val[M]:		variable in systolic pe, hold the tuser signal from its previous stage.
	valid_val[M]:		variable in systolic pe, hold the tvalid signal from its previous stage.
	acc[M]:				variable in systolic pe. The accumulator to sum up A[*][i]x[i]
	idx[M]:				variable in systolic pe, points to corresponding column of a row.
	row_counter:		counter for reload, counts how many data has been loaded in a row.
	col_counter:		counter for reload, counts how many rows has been loaded in the matrix.
	o_counter:			output counter, point to the row that could have valid output. The valid output comes in series.
*/
class MtV{
public:
	//shift_reg<d_htype,M> A_local[M];
	d_htype A_local[M][M];
	logic last_val[M];
	logic user_val[M];
	logic valid_val[M];
	d_htype acc[M];
	d_htype d_val[M];
	int idx[M];
	int row_counter = 0;
	int o_counter = 0;
	int col_counter = 0;
	MtV() {}
	/*
	run the pipe lined systolic array.
	input:
		A_valid:	valid signal for A
		A:			data for the matrix reloading
		x_valid:	valid signal for x
		x:			data for computing
	output:
		y_valid:	valid signal for output y
		y:			data output
	*/
	void run(logic& y_valid, dp_htype& y, logic A_valid, dp_htype A, logic x_valid, dp_htype x){
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->A_local
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->last_val
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->user_val
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->valid_val
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->acc
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->d_val
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=this->idx
		// reload has the higest privilige
		if(A_valid){
			this->A_local[this->col_counter][this->row_counter] = A.data;
			this->row_counter++;
			if (this->row_counter == M){	// one row finished
				this->row_counter = 0;
				this->col_counter++;
				if (this->col_counter == M){ // one round finished
					this->col_counter = 0;
				}
			}
			// this is to avoid uninitialized output
			y_valid = False;
			y.data = 0;
			y.last = False;
			y.user = False;
		}
		else{
			// set the data according to the input valid
			d_htype din_i = x_valid?x.data:(d_htype)0;
			logic last_i = x_valid?x.last:(logic)0;
			logic user_i = x_valid?x.user:(logic)0;
			// UNROLLed systolic array, each loop generates a PE
			for (int i = 0; i < M;i++){
	#pragma HLS UNROLL
				if(i == 0){ // first PE, depend on the input
					if(x_valid){
						if (user_i){ // first data, clear the acc and idx at the same time
							idx[i] = 0;
							d_htype a = this->A_local[i][idx[i]];
							this->acc[i] = a * din_i;
						}
						else{ // normal input, idx increase, do accumulation
							idx[i]++;
							d_htype a = this->A_local[i][idx[i]];
							this->acc[i] += a * din_i;
						}
					}
				}
				else{ // normal PE, depend on its previous PE
					if(this->valid_val[i-1]){
						if (this->user_val[i-1]){
							idx[i] = 0;
							d_htype a = this->A_local[i][idx[i]];
							this->acc[i] = a * this->d_val[i-1];
						}
						else{
							idx[i]++;
							d_htype a = this->A_local[i][idx[i]];
							this->acc[i] += a * this->d_val[i-1];
						}
					}
				}
			}
			// shift registers
			for (int i = M - 1; i > 0;i--){
	#pragma HLS UNROLL
				this->user_val[i] = this->user_val[i-1];
			}
			this->user_val[0] = user_i;

			for (int i = M - 1; i > 0;i--){
	#pragma HLS UNROLL
				this->valid_val[i] = this->valid_val[i-1];
			}
			this->valid_val[0] = x_valid;

			for (int i = M - 1; i > 0;i--){
	#pragma HLS UNROLL
				this->last_val[i] = this->last_val[i-1];
			}
			this->last_val[0] = last_i;

			for (int i = M - 1; i > 0;i--){
	#pragma HLS UNROLL
				this->d_val[i] = this->d_val[i-1];
			}
			this->d_val[0] = din_i;

			// determine the output
			if (this->last_val[this->o_counter] == True){ // last is True means a complete x vector has been received.
				y.data = this->acc[this->o_counter];
				y_valid = True;
				// New last and user signal have to be generated
				if (this->o_counter == 0){ // first
					y.user = True;
					y.last = False;
				}
				else if (this->o_counter == M - 1){ // last
					y.user = False;
					y.last = True;
				}
				else{
					y.user = False;
					y.last = False;
				}
				this->o_counter++;
				if (this->o_counter == M){ // roll back
					this->o_counter = 0;
				}
			}
			else{
				y.data = 0;
				y.last = False;
				y.user = False;
				y_valid = False;
			}
		}
	}
};

extern "C" {
/*
systolic_array: compute matrix times vector. Reload has higer privilege
        ┌───────────────────────┐         
        │                       │         
────A───▶                       │         
        │    systolic_array     ├───y───▶ 
────u───▶                       │       │ 
▲       │                       │         
        └───────────────────────┘       │ 
│                                         
                                        │ 
│ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─    
Input:
	A:		Stream of reload data. (reload_converter.A_stream_out-->systolic_array.A)
	u:		Strean of data for computing; u contains source and last status. (controller.u-->systolic_array.u)
Output:
	y:		Stream of output data. Paralled NM d_htype data. (systolic_array.y-->adder_tree.x)
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
void systolic_array(hls::stream<dp_dharray>& y, hls::stream<dp_htype>& A, hls::stream<dp_src>& u){
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS PIPELINE
	static int inst_counter = 0; // counter to determined where the reload data goes to
	static MtV PE[NM]; // instance for systolic arrays (SA)
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=PE
	logic y_valid[NM]; // output for each SA
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=y_valid
	logic A_valid[NM]; // input for each SA
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=A_valid
	logic x_valid; // input for all SAs
	dp_htype yy[NM]; // output for each SA
	dp_htype AA; // input of the reload stream
	dp_htype xx; // input of the last status
	dp_htype ss; // input of source
	dp_src UU; // temp variable to receive u
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=yy
	if(!A.empty()){  // has reload data
		for(int i = 0; i < NM;i++){ // only the current reloading SA gets valid data.
	#pragma HLS UNROLL
			A_valid[i] = (i == inst_counter)?True:False;
		}
		A >> AA; // read from A
		// this is to avoid uninitailied data, complete the if-else
		x_valid = False;
		xx.data = 0;
		xx.last = False;
		xx.user = False;
		ss.data = 0;
		ss.user = False;
		ss.last = False;
		if (AA.last == True){ // last data of a matrix
			inst_counter++;
			if(inst_counter == NM){
				inst_counter = 0;
			}
		}
	}
	else{ // no reload data
		// complete the if-else
		for(int i = 0; i < NM;i++){
	#pragma HLS UNROLL
			A_valid[i] = False;
		}
		AA.data = 1;
		AA.last = False;
		AA.user = False;
		x_valid = u.read_nb(UU); // non-block read. Keep the kernel running even when no data is available. Avoid stuck.
		// pass data to source and last status temp veriable.
		xx.data = UU.data[1];
		ss.data = UU.data[0];
		xx.user = UU.user;
		ss.user = UU.user;
		xx.last = UU.last;
		ss.last = UU.last;

	}
	// UNROLLed instance for create NM SAs
	for (int i = 0; i < NM;i++){
#pragma HLS UNROLL
		if(i == 0) // first one is to handle the source, give ss
			PE[i].run(y_valid[i],yy[i],A_valid[i],AA,x_valid,ss);
		else // the others depend on the last status, give xx
			PE[i].run(y_valid[i],yy[i],A_valid[i],AA,x_valid,xx);
	}
	if (y_valid[0]){  // check if output data availale, any one of y_valid is ok.
		dp_dharray y_out;
		for (int i = 0; i < NM;i++){
#pragma HLS UNROLL
			y_out.data[i] = yy[i].data;
		}
		y_out.last = yy[0].last;
		y_out.user = yy[0].user;
		y << y_out; // out
	}

}
}
