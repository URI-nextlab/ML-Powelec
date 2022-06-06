#include "typedef.hpp"
/*
Stream Divider: make copies of the result stream, allow parallel computing and reduce the loop latency.
        ┌─────────────┐
        │        ┌──▶s_out1─▶
        │    ┌───┘    │
 ──s_in─▶ ───●──────▶s_out2─▶
        │    └───┐    │
        │        └──▶s_out3─▶
        └─────────────┘
Input:
	s_in:	Stream input
Output:
	s_out*:	Stream output
*/
void Stream_Divider(dp_stream& s_in,dp_stream& s_out1, dp_stream& s_out2){
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE
	dp din;
	s_in >> din;
	s_out1 << din;
	s_out2 << din;
//	s_out3 << din;
	// all of them are block read/write. The operation won't happen unless data is available on s_in and s_out* are ready to receive data.
}
