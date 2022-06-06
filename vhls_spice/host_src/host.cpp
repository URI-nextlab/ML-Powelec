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
#include <stdio.h>

#define M 48
#define Diodes 2
#define Switches 2

//const int Diodes = 0;
void mmult(float A[], float x[]){
	printf("[Info.]\tSoftware test!\n");
	printf("[Info.]\tSoftware calculated static:\n\t");
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

	const int IT = 3000;
    const int swPeriod = 100;
    const int srcPeriod = 1;

    int rms_id = 20;
    int mean_id = 7;

	printf("[Process]\tEmulation starts!\n");
    printf("[Info.]\tTotal time steps:\t%d\n", IT);
    printf("[Info.]\tUse DC source for test!\n");

    printf("[Process]\tStart allocating vectors...\n");
    float A_src[M * M];
    float A_react[M * M];
    float A_Switches[Switches * 2 * M * M];
    float A_Diodes[Diodes * 2 * M * M];
    float J[Diodes * 2 * M];
    float y[IT * M];
    float ob[2];
    unsigned short sw[swPeriod];

    float x0[M];
    float src[M];

    printf("[Process]\tFinish allocating vectors!\n");
	printf("[Process]\tStart loading matrixes and vectors...\n");
    fp = fopen("./host_src/to_aws.txt","r");
    if (fp == NULL){
        printf("[Error]\tFaild to load txt file!\n");
        printf("[Error]\tBreak simulation!\n");
        return 0 ;
    }

	printf("[Process]\tReading A_Src...\n");
    for(int i = 0;i < M * M;i++){
    	fscanf(fp,"%f",&A_src[i]);
	}
	printf("[Process]\tReading A_react...\n");
    for(int i = 0;i < M * M;i++){
    	fscanf(fp,"%f",&A_react[i]);
	}

	printf("[Process]\tReading A_Switches...\n");
    for(int i = 0;i < Switches * 2 * M * M;i++){
    	fscanf(fp,"%f",&A_Switches[i]);
	}

	printf("[Process]\tReading A_Diodes...\n");
    for(int i = 0;i < Diodes * 2 * M * M;i++){
    	fscanf(fp,"%f",&A_Diodes[i]);
	}

	printf("[Process]\tReading J...\n");
    for(int i = 0;i < Diodes * 2 * M;i++){
    	fscanf(fp,"%f",&J[i]);
	}


    printf("[Process]\tReading sources:\n\t");
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

    printf("[Process]\tReading x0:\n\t");
    printf("[Info.]\tInitial status:\n\t");
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


	printf("[Process]\tFinished loading matrixes and vectors!\n");


    mmult(A_src,src);

    printf("[Process]\tGenerating switch control signals...\n");
    fclose(fp);
    for(int i = 0;i < swPeriod;i++){
    	if(i > (swPeriod / 2))
    		sw[i] = 1;
    	else
    		sw[i] = 2;
	}


    printf("[Process]\tSoftware initialization done!\n\n");
    printf("[Process]\tRun hardware initialization...\n\n");

    int device_id = 0;
	std::cout << "\tOpen the device" << device_id << std::endl;
	auto device = xrt::device(device_id);
	std::cout << "\tLoad the xclbin " << binaryFile << std::endl;
	auto uuid = device.load_xclbin(binaryFile);

	// Connect to kernels
	printf("[Process]\tConnect to %s kernel...\n", "Controller");
	auto Controller = xrt::kernel(device, uuid, "Controller");

	printf("[Process]\tConnect to %s kernel...\n", "Matrix_Gen:{M_Gen_src}");
	auto M_Gen_src = xrt::kernel(device, uuid, "Matrix_Gen:{M_Gen_src}");
	printf("[Process]\tConnect to %s kernel...\n", "Matrix_Gen:{M_Gen_react}");
	auto M_Gen_react = xrt::kernel(device, uuid, "Matrix_Gen:{M_Gen_react}");


	printf("[Process]\tConnect to %s kernel...\n", "Switch_M_Gen");
	auto Switch_M_Gen = xrt::kernel(device, uuid, "Switch_M_Gen");


	printf("[Process]\tConnect to %s kernel...\n", "Diode_M_Gen");
	auto Diode_M_Gen = xrt::kernel(device, uuid, "Diode_M_Gen");

	printf("[Process]\tConnect to %s kernel...\n", "Switch_Controller");
	auto Switch_Controller = xrt::kernel(device, uuid, "Switch_Controller");

	printf("[Process]\tConnect to %s kernel...\n", "J_reloader");
	auto J_reloader = xrt::kernel(device, uuid, "J_reloader");


	printf("[Process]\tConnect to %s kernel...\n", "Source_Gen");
	auto Source_Gen = xrt::kernel(device, uuid, "Source_Gen");


	printf("[Process]\tConnect to %s kernel...\n", "result_back");
	auto result_back = xrt::kernel(device, uuid, "result_back");

	printf("[Process]\tConnect to %s kernel...\n", "observer");
	auto observer = xrt::kernel(device, uuid, "observer");


	std::cout << "[Process]\tAllocate Buffer in Global Memory...\n";
	printf("[Process]\tAllocate %s buffer...\n","A_src");
	auto bo_A_src = xrt::bo(device, sizeof(A_src), M_Gen_src.group_id(0));
	printf("[Process]\tAllocate %s buffer...\n","A_react");
	auto bo_A_react = xrt::bo(device, sizeof(A_react), M_Gen_react.group_id(0));
	printf("[Process]\tAllocate %s buffer...\n","A_Switches");
	auto bo_A_Switches = xrt::bo(device, sizeof(A_Switches), Switch_M_Gen.group_id(0));
	printf("[Process]\tAllocate %s buffer...\n","A_Diodes");
	auto bo_A_Diodes = xrt::bo(device, sizeof(A_Diodes), Diode_M_Gen.group_id(0));

	printf("[Process]\tAllocate %s buffer...\n","J");
	auto bo_J = xrt::bo(device, sizeof(J), J_reloader.group_id(0));

	printf("[Process]\tAllocate %s buffer...\n","x0");
	auto bo_x0 = xrt::bo(device, sizeof(x0), Controller.group_id(0));

	printf("[Process]\tAllocate %s buffer...\n","src");
	auto bo_src = xrt::bo(device, sizeof(src), Source_Gen.group_id(0));


	printf("[Process]\tAllocate %s buffer...\n","y");
	auto bo_y = xrt::bo(device, sizeof(y), result_back.group_id(0));

	printf("[Process]\tAllocate %s buffer...\n","y");
	auto bo_ob = xrt::bo(device, sizeof(ob), observer.group_id(0));



	printf("[Process]\tAllocate %s buffer...\n","y");
	auto bo_sw = xrt::bo(device, sizeof(sw), Switch_Controller.group_id(0));
    // Allocate Buffer in Global Memory

	bo_A_src.write(A_src);
	bo_A_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_A_react.write(A_react);
	bo_A_react.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_A_Switches.write(A_Switches);
	bo_A_Switches.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_A_Diodes.write(A_Diodes);
	bo_A_Diodes.sync(XCL_BO_SYNC_BO_TO_DEVICE);


	bo_J.write(J);
	bo_J.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_x0.write(x0);
	bo_x0.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_src.write(src);
	bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	bo_sw.write(sw);
	bo_sw.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	auto controller_run = xrt::run(Controller);
	auto result_back_run = xrt::run(result_back);
	auto swc_run = xrt::run(Switch_Controller);
	auto src_run = xrt::run(Source_Gen);
	auto reload_run = xrt::run(J_reloader);
	auto dmg_run = xrt::run(Diode_M_Gen);
	auto smg_run = xrt::run(Switch_M_Gen);
	auto asrc_run = xrt::run(M_Gen_src);
	auto areact_run = xrt::run(M_Gen_react);
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

	reload_run.set_arg(0, bo_J);

	dmg_run.set_arg(0, bo_A_Diodes);
	dmg_run.set_arg(1, IT);

	smg_run.set_arg(0, bo_A_Switches);
	smg_run.set_arg(1, IT);

	asrc_run.set_arg(0, bo_A_src);
	asrc_run.set_arg(1, IT);

	areact_run.set_arg(0, bo_A_react);
	areact_run.set_arg(1, IT);

	observer_run.set_arg(0, bo_ob);
	observer_run.set_arg(1, rms_id);
	observer_run.set_arg(2, mean_id);
	observer_run.set_arg(3, IT);



    printf("[Process]\tHardware initialization done!\n\n");
    printf("[Process]\tStart writing matrixes to hardware...\n");

    std::cout << "[Process]\tFinished writing!" << std::endl;
    printf("[Process]\tStart simulation...\n");
    for (int tt = 0; tt < 1; tt++){
    	printf("[Process]\tRound %d...\t",tt+1);
    	reload_run.start();
    	reload_run.wait();
    	observer_run.start();
    	dmg_run.start();
    	smg_run.start();
    	asrc_run.start();
    	areact_run.start();
    	swc_run.start();
    	src_run.start();
		result_back_run.start();
		controller_run.start();
		// wait
		dmg_run.wait();
		std::cout << "[Process]\t\tdmg finished!" << std::endl;
		smg_run.wait();
		std::cout << "[Process]\t\tsmg finished!" << std::endl;
		asrc_run.wait();
		std::cout << "[Process]\t\tasrc finished!" << std::endl;
		areact_run.wait();
		std::cout << "[Process]\t\tareact finished!" << std::endl;
		swc_run.wait();
		std::cout << "[Process]\t\tswc finished!" << std::endl;
		src_run.wait();
		std::cout << "[Process]\t\tsrc finished!" << std::endl;
		result_back_run.wait();
		std::cout << "[Process]\t\tresult finished!" << std::endl;
		controller_run.wait();
		std::cout << "[Process]\t\tcontroller finished!" << std::endl;
    	observer_run.wait();
		std::cout << "[Process]\t\tobserver finished!" << std::endl;
		std::cout << "[Process]\tFinished one round!" << std::endl;

		bo_y.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		bo_y.read(y);

		bo_ob.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		bo_ob.read(ob);
		printf("[Info.]\tObserved RMS = %f, MEAN = %f \n", ob[1], ob[0]);
    }


    std::cout << "[Process]\tFinished simulation!" << std::endl;


    printf("[Process]\tWriting final result to res.txt!\n");

    fp = fopen("./host_src/res.txt","w");
    if (fp == NULL){
    	printf("[Error]\tCannot open res.txt! Break!\n");
    	return 0;
    }
	for (int i = 0; i < IT; i++){
		for (int j = 0; j < M; j++) {
			fprintf(fp,"%.2f\t",y[i * M + j]);

		}
		fprintf(fp,"\n");
	}
	fclose(fp);

	printf("[Process]\tHost program finished!\n");


    return EXIT_SUCCESS;
}

