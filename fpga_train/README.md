# vhls_spice

# Introduction

This is a vitis project that creates bit_container.xclbin for hardware acceleration of power electronics time domain simulation. It contains following folders:
> host_src  
>
> * Host sources to run Hardware emulation. Software emulation is not supported because free running kernels are used.
>
> kernel_src
>
> * Kernel sources contains c++ kernels and the linker file to connect the kernels.

To create the bit_container.xclbin, correct pathes should be set in the Makefile. Here is the example.

```makefile
# tool chain path
XILINX_VITIS ?= /tools/Xilinx/Vitis/2021.2
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /tools/Xilinx/Vivado/2021.2
XILINX_HLS_INCLUDES ?= /tools/Xilinx/Vitis_HLS/2021.2/include

# Platform path
VITIS_PLATFORM = xilinx_aws-vu9p-f1_shell-v04261818_201920_2
VITIS_PLATFORM_DIR = /home/Nextlab_Share/aws-fpga/Vitis/aws_platform/xilinx_aws-vu9p-f1_shell-v04261818_201920_2
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM_DIR)/xilinx_aws-vu9p-f1_shell-v04261818_201920_2.xpfm
```

The default target is hardware emulation. To run, just type:

```shell  
make all -j8
```

To make xclbin that can be run on FPGA, specify the TARGET as hw:  

```shell
make all TARGET=hw -j8
```

# Time domain simulation algorithm
The time domain simulation is based on Modified Nodal Analysis (MNA) [^MNA]. MNA itself doesn't support simulate reactive components such as inductors and capcaitors in time domain at the begining. To solve the problem, the reactive componets are modeled as a resistor and a current source connected in parallel. The resistor in the model is constant during the time domain simulation while the current source changes depending on the circuit status from the previous time step. In general, MNA is solving the linear equation:  
$$
Azs
$$

# Hardware kernels  

## Data type definitions

All global data type definitions are wrote in *typedef.hpp*. The hardware connot process floating point data efficiently, therefore, all data are transfered to a fixed point number when read from the host, and transfered back to corresponding software data type when they are writen back.  
The data transferred between free running kernels must use AXI stream interface. Xilinx officially supports AXI stream interface with side channels (*tlast*, *tuser*, ...). However, it some times gives some errors when using the AXI stream with side channels. Therefore, the side channels and data are packed in a structure, such as:  

``` c
typedef ap_fixed<W,IW> d_htype;
typedef ap_uint<1> logic;
typedef struct __attribute__((packed)){
    d_htype data;
    logic last;
    logic user;
}dp_htype;
```  

The *\_\_attribute\_\_((packed))* is to avoid wasting data width because C may automatically align to 32 bits or 4 bytes[^Structure on interface].

## systolic_array
The systlic_array kernel contains NM (Number of Matrixes) of matrix multiplication processing units using systolic structure. The first PE is used to calculate the response of directly applied sources while the other PEs are used to calculate the reponse of reactive components such as L, C, and switches. The results are packed into an array and transfered to the adder_tree kernel to sum them up accordingly.  

```
               ┌───────────────────────────────────────────────┐                
               │                                               │                
               │                                               │                
               │                ┌───────────────┐              │                
               │                │               │              │                
               │       ┌────────▶      PE3      ├─────────┐    │                
               │       │        │               │         │    │                
               │       │        └───────────────┘         │    │                
               │       │                                  │    │                
               │       │        ┌───────────────┐         │    │                
               │       │        │               │         │    │                
               │       ├────────▶      PE2      ├────────┐│    │                
               │       │        │               │        ││    │                
               │       │        └───────────────┘        ││    │                
               │       │                                 ││    │                
               │       │        ┌───────────────┐        ││    │                
               │       │        │               │        ││    ├────────────────
               │       ├────────▶      PE1      ├───────┐│└────┼─────▶          
               │       │        │               │       │└─────┼─────▶          
               │       │        └───────────────┘       └──────┼─────▶          
               │       │                                  ┌────┼─────▶          
  last_status  │       │        ┌───────────────┐         │    ├────────────────
  ─────────────┼───────┘        │               │         │    │                
  source       │                │      PE0      ├─────────┘    │                
  ─────────────┼────────────────▶               │              │                
               │                └───────────────┘              │                
               │                                               │                
               │                systolic_array                 │                
               └───────────────────────────────────────────────┘                
```

## adder_tree
The final circuit status is sum up of all valid results from the systolic_array (enabled by superposition). The swithes and diodes have two different status so it is required to pick between two different 

[^Structure on interface]: <https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Structs-on-the-Interface>
[^MNA]: <https://lpsa.swarthmore.edu/Systems/Electrical/mna/MNA1.html>