/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

/*
   Shift Register

   This example demonstrates how to perform a shift register operation to
   implement a Finite Impulse Response(FIR) filter.

   NOTE: See the fir.cl file for additional information.
  */
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include <stdio.h>
typedef float d_in_type;
typedef float d_out_type;
const int cus = 1;
using std::default_random_engine;
using std::inner_product;
using std::string;
using std::uniform_int_distribution;
using std::vector;

#define M 48
#define NM 10
#define K M
const int Diodes = (NM - 2) / 4;
//const int Diodes = 0;
// test if the matrix loaded correctly
void mmult(d_in_type A[],d_in_type x[]){
	printf("Software test!\n");
	printf("Software calculated static:\n\t");
	// static response equals iA * src
	for (int i = 0; i < M;i++){
		float sum = 0;
		for (int j = 0; j < M;j++){
			sum += A[i * M + j] * x[j];
		}
		if (i < 11)
			printf("%.2f\t",sum);
		else
			printf(".");
	}
	printf("\n\n");
}

int main(int argc, char** argv) {
	FILE* fp;
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];
    bool do_reload = true;

	const int IT = 2000; // iteration time
    const int swPeriod = 100; // switch period
    const int srcPeriod = 1; // source period (Period = 1 -> DC source)

    int rms_id = 20; // rms observation ID
    int mean_id = 7; // mean observation ID

	printf("Emulation starts!\n");
    printf("\tTotal time steps:\t%d\n", IT);
    printf("\tDC source\n");

    printf("Start allocating vectors...\n");
    d_in_type A[NM * M * M + M * Diodes * 2];
    d_in_type x0[M];
    d_in_type src[M * srcPeriod];
    d_out_type y[IT * M];
    unsigned char sw[swPeriod];
    d_out_type ob[2];


    auto A_size_in_bytes = (NM * M * M + M * Diodes * 2) * sizeof(d_in_type);
    auto x0_size_in_bytes = M * sizeof(d_in_type);
    auto src_size_in_bytes = srcPeriod * M * sizeof(d_in_type);
    auto y_size_in_bytes = IT * M * sizeof(d_out_type);
    auto sw_size_in_bytes = swPeriod * sizeof(unsigned char);
    auto ob_size_in_bytes = 2 * sizeof(d_in_type);

    printf("Finish allocating vectors!\n");
	printf("Start loading matrixes and vectors...\n");
    fp = fopen("./host_src/to_aws.txt","r");
    if (fp == NULL){
        printf("Faild to load txt file!\n");
        printf("Break simulation!\n");
        return 0 ;
    }
    for(int i = 0;i < NM * M * M + M * Diodes * 2;i++){
    	fscanf(fp,"%f",&A[i]);
	}
    printf("Sources:\n\t");
    for(int i = 0;i < M;i++){
    	float temp = 0;
    	fscanf(fp,"%f",&temp);
    	src[i] = temp;
    	if (i < 11){
    		printf("%.2f\t",src[i]);
    	}
    	else{
    		printf(".");
    	}
	}



    printf("\n");
	printf("Finished loading matrixes and vectors!\n");
	printf("Initial status:\n\t");
    for(int i = 0;i < M;i++){
    	float temp = 0;
    	fscanf(fp,"%f",&temp);
    	x0[i] = temp;
    	if (i < 11){
    		printf("%.2f\t",x0[i]);
    	}
    	else{
    		printf(".");
    	}
	}
    printf("\n");

    printf("\n\t");

	// Check software computed static response
    mmult(A,src);

    printf("Generating switch control signals...\n");
    fclose(fp);
    for(int i = 0;i < swPeriod;i++){
    	if(i > (swPeriod / 2))
    		sw[i] = 1; // Control phase 1
    	else
    		sw[i] = 2; // Control phase 2
	}


    printf("\nSoftware initialization done!\n\n");
    printf("Run hardware initialization...\n\n");

    bool valid_device = false;
    int device_id = 0;
	std::cout << "\tOpen the device" << device_id << std::endl;
	auto device = xrt::device(device_id);
	// Creating Context and Command Queue for selected Device
	std::cout << "\tLoad the xclbin " << binaryFile << std::endl;
	auto uuid = device.load_xclbin(binaryFile);
	printf("\tConnect to %s kernel...\n", "controller");
	auto controller = xrt::kernel(device, uuid, "controller");
	printf("\tConnect to %s kernel...\n", "reload");
	auto reload = xrt::kernel(device, uuid, "reload");
	printf("\tConnect to %s kernel...\n", "switch_controller");
	auto switch_controller = xrt::kernel(device, uuid, "switch_controller");
	printf("\tConnect to %s kernel...\n", "src_generator");
	auto src_generator = xrt::kernel(device, uuid, "src_generator");
	printf("\tConnect to %s kernel...\n", "observer");
	auto observer = xrt::kernel(device, uuid, "observer");
	auto result_back = xrt::kernel(device, uuid, "result_back");
	printf("\tConnect to %s kernel...\n", "result_back");

	std::cout << "Allocate Buffer in Global Memory...\n";
	printf("\tAllocate %s buffer...\n","A");
	auto bo_A = xrt::bo(device, A_size_in_bytes, reload.group_id(0));

	printf("\tAllocate %s buffer...\n","x0");
	auto bo_x0 = xrt::bo(device, x0_size_in_bytes, controller.group_id(0));
	printf("\tAllocate %s buffer...\n","y");
	auto bo_y = xrt::bo(device, y_size_in_bytes, result_back.group_id(0));

	printf("\tAllocate %s buffer...\n","sw");
	auto bo_sw = xrt::bo(device, sw_size_in_bytes, switch_controller.group_id(0));

	printf("\tAllocate %s buffer...\n","src");
	auto bo_src = xrt::bo(device, src_size_in_bytes, src_generator.group_id(0));

	printf("\tAllocate %s buffer...\n","ob");
	auto bo_ob = xrt::bo(device, ob_size_in_bytes, observer.group_id(0));
    // Allocate Buffer in Global Memory

	bo_A.write(A);
	bo_A.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_x0.write(x0);
	bo_x0.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_src.write(src);
	bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_sw.write(sw);
	bo_sw.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	auto controller_run = xrt::run(controller);
	auto result_back_run = xrt::run(result_back);
	auto swc_run = xrt::run(switch_controller);
	auto src_run = xrt::run(src_generator);
	auto reload_run = xrt::run(reload);
	auto observer_run = xrt::run(observer);

	result_back_run.set_arg(0, bo_y);
	result_back_run.set_arg(1, IT);

	controller_run.set_arg(0, bo_x0);
	controller_run.set_arg(1, IT);

	swc_run.set_arg(0, bo_sw);
	swc_run.set_arg(1, swPeriod);
	swc_run.set_arg(2, IT);

	src_run.set_arg(0,bo_src);
	src_run.set_arg(1,srcPeriod);
	src_run.set_arg(2,IT);

	reload_run.set_arg(0, bo_A);

	observer_run.set_arg(0,bo_ob);
	observer_run.set_arg(1,rms_id);
	observer_run.set_arg(2,mean_id);
	observer_run.set_arg(3,IT);


    printf("Hardware initialization done!\n\n");
    printf("Start writing matrixes to hardware...\n");

    std::cout << "Finished writing!" << std::endl;
    printf("Start simulation...\n");
    for (int tt = 0; tt < 2; tt++){
    	printf("\tRound %d...",tt+1);
		// Ctrl Phase 1
    	reload_run.start();
    	reload_run.wait();
		// Ctrl Phase 2
		swc_run.start();
		src_run.start();
		result_back_run.start();
		observer_run.start();
		// Ctrl Phase 3
		controller_run.start();
		swc_run.wait();
		controller_run.wait();
		src_run.wait();
		observer_run.wait();
		result_back_run.wait();
		std::cout << "\tFinished one round!" << std::endl;
		// read back
		bo_y.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		bo_y.read(y);
		// read back
		bo_ob.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		bo_ob.read(ob);
		printf("\tObserved mean = %.2f; Observed RMS = %.2f.\n",ob[0],ob[1]);
    }


    std::cout << "Finished simulation!" << std::endl;


    printf("Writing final result to res.txt!\n");

    fp = fopen("./host_src/res.txt","w");
    if (fp == NULL){
    	printf("Cannot open res.txt! Break!\n");
    	return 0;
    }
	for (int i = 0; i < IT; i++){
		for (int j = 0; j < M; j++) {
			fprintf(fp,"%.2f\t",y[i * K + j]);

		}
		fprintf(fp,"\n");
	}
	fclose(fp);

	printf("Host program finished!\n");


    return EXIT_SUCCESS;
}

