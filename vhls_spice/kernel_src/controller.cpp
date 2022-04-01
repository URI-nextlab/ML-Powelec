#include "typedef.hpp"
extern "C" {
/*
controller: handle the loop of iterations
            ┌────────────────┐            
  ────x0────▶                │            
            │               result_stream 
  ────IT────▶                ├────────▶   
            │   controller   │            
src_stream──▶                ├───u────▶   
            │                │         │  
 ▲─x_back───▶                │            
            └────────────────┘         │  
 │                                        
                                       │  
 ┴ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─  
input:
	x0(host): 	The initial status of the circuit, M*1 dimension. It is read only once.
	IT(host):	The iteration times, or time steps.
	x_back:		The looped back circuit status vector x stream. (Stream_Divider.s_out1-->controller.x_back)
	src_stream:	The stream to receive independent sources. (src_generator.src_stream-->controller.src_stream)
outputs:
	result_stream:	The circuit status output stream. (controller.result_stream-->result_back.result_stream)
	u:				The systolic_array input stream, conatains independet sources and current status in the last time step:
					(controller.u-->systolic_array.u)
*/

void controller(d_stype x0[], int IT, hls::stream<d_htype>& result_stream, hls::stream<d_htype>& src_stream, hls::stream<dp_src>& u, hls::stream<dp_htype>& x_back){
#pragma HLS INTERFACE m_axi port = x0 offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = x0
#pragma HLS INTERFACE s_axilite port = IT
#pragma HLS INTERFACE s_axilite port = return
	static dp_src temp; // temp variable to create u:
	static dp_htype dback; // temp variable to receive loop backed data
	
	// read x0 in advance to avoid trigging AXI4 bus too frequently
	static ap_shift_reg<d_htype,M> x0_local;
	for (int j = 0;j < M; j++){
		x0_local.shift((d_htype)x0[j]);
	}
	int itj = 0; // it: iteration; j: M variables in one loop. Counts how many efficient loops has been made.
	do {
#pragma HLS PIPELINE
		if (itj < M){	// first iteration, no back data, stream out first u
			d_htype src_val;
			d_htype x0_val = x0_local.shift(1);
			src_stream >> src_val;
			temp.data[0] = src_val;
			temp.data[1] = x0_val;
			// generate tuser, valid when it is the first data of x (M*1 vector)
			if (itj == 0){
				temp.user = True;
			}
			else{
				temp.user = False;
			}
			// generate tlasy, valid when it is the last data of x (M*1 vector)
			if (itj == M - 1){
				temp.last = True;
			}
			else{
				temp.last = False;
			}
			u << temp;
			itj++;
		}
		else if (itj >= ((IT)*M)){ // last iteration, just write back result, do not give u to systolic_array any more
			logic new_data = x_back.read_nb(dback);
			if(new_data){
				result_stream << dback.data;
				itj++;
			}
		}
		else{	// normal iteration, provide u when a xback is received.
			logic new_data = x_back.read_nb(dback); // must be non-block read, as x_back may not have available data.
			if (new_data){	// got a data back
				d_htype src_val;
				src_stream >> src_val;
				temp.data[0] = src_val;
				temp.data[1] = dback.data;
				// the user and last are preserved in the loop, so it is not required to generate it again.
				temp.user = dback.user;
				temp.last = dback.last;
				result_stream << dback.data;
				u << temp;
				itj++;
			}
		}
	}while(itj < (IT + 1) * M); // total efficient loop should be (IT + 1) * M
}

}
