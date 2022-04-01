#ifndef __TYPEDEF_HPP__
#define __TYPEDEF_HPP__

#include "hls_stream.h"
#include "ap_fixed.h"
#include "ap_axi_sdata.h"
#include "ap_shift_reg.h"

#define W 32		// Width of the fixed point number
#define IW 12		// Width of interger in the fixed point number

#define M 48		// Supported dimension of matrix
#define NM 10		// Number of Matrixes

const int S = (NM - 2) / 2;			// Number of total switches
const int SWs = (NM - 2) / 4;		// Number of switches
const int Diodes = (NM - 2) / 4;	// Number of diodes

typedef float d_stype;				// Software (host) data type
typedef ap_fixed<W,IW> d_htype;		// Hardware (fpga) data type for computing
typedef ap_uint<8> u8_bitwise;		// Switch signals, allow bitwise access
typedef ap_uint<1> logic;			// Logic 1 bit

// d_htype data package
typedef struct __attribute__((packed)){
	d_htype data;
	logic last;
	logic user;
}dp_htype;

// two d_htype data package, for streaming source and last circuit status
typedef struct __attribute__((packed)){
	d_htype data[2];
	logic last;
	logic user;
}dp_src;

// d_htype array data package, for streaming the result of systolic arrays to tree adder.
typedef struct __attribute__((packed)){
	d_htype data[NM];
	logic last;
	logic user;
}dp_dharray;

// Definition of logical values
const logic True = (ap_uint<1>)1;
const logic False = (ap_uint<1>)0;


#endif
