#include "typedef.hpp"

void x_pickup(u8 idx[], int IT, dp_stream& x, x_picked_stream& xp_stream){
#pragma HLS interface s_axilite port=return
#pragma HLS INTERFACE m_axi port=idx offset=slave bundle=idx_Mgen
    u8 reload_counter;
    u8 idx_local[4 * Diodes];
#pragma HLS array_partition variable=idx_local dim=1 type=complete
    for (int i = 0; i < 4 * Diodes;i++){
        idx_local[i] = idx[i];
    }

    for (int it = 0; it < IT; it++){
        x_picked x_picked_temp;
        for (int i = 0; i < M;i++){
#pragma HLS PIPELINE style=frp
            dp x_temp;
            x >> x_temp;
            for (int j = 0; j < 4 * Diodes;j++){
#pragma HLS UNROLL
                if (idx_local[j] == i){
                    x_picked_temp.data[j] = x_temp.data;
                }
                else{
                    x_picked_temp.data[j] = x_picked_temp.data[j];
                }
            }
        }
        xp_stream << x_picked_temp;
    }
}
