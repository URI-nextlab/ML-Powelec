import pynq
import numpy as np
import os
import time
from MNA import MNA

class FPGA_Spice_Accelerator:
    def __init__(self, overlay = 'spice_dma.awsxclbin', device_id = 0):
        self.accumulated_time = 0
        self.simulation_count = 0
        self.matrixGen = MNA()
        self.fpgaKernel = pynq.Overlay(overlay)
        self.k_controller = self.fpgaKernel.controller_1
        self.k_swGen = self.fpgaKernel.switch_controller_1
        self.k_srcGen = self.fpgaKernel.src_generator_1
        self.k_reloader = self.fpgaKernel.reload_1
        self.k_observer = self.fpgaKernel.observer_1
        self.k_resultBack = self.fpgaKernel.result_back_1
        self.bufferAllocated = False
        self.timeSteps = -1
        self.switchPeriod = -1
        self.sourcePeriod = -1
        self.switchSignal = None
        self.sourceSignal = None
        self.M = 0
        self.S = 0
        self.M_NUM = 0
        self.h = 0
        self.netlist = None
        self.timeSteps = 0
    
    def getMemBank(self,arg):
        if arg.mem == 'bank0':
            return self.fpgaKernel.bank0
        elif arg.mem == 'bank1':
            return self.fpgaKernel.bank1
        elif arg.mem == 'bank2':
            return self.fpgaKernel.bank2
        elif arg.mem == 'bank3':
            return self.fpgaKernel.bank3
    
    def set_simulator(self, netlist, timeSteps, h, swPeriod, srcPeriod, M, S, RMS_ID, MEAN_ID):
        self.switchPeriod = swPeriod
        self.sourcePeriod = srcPeriod
        self.M = M
        self.S = S
        self.M_NUM = 4 * self.S + 2
        self.timeSteps = timeSteps
        self.netlist = netlist
        self.h = h
        self.RMS_ID = int(RMS_ID)
        self.MEAN_ID = int(MEAN_ID)
        print('Creating cirucit model...')
        self.matrixGen.load_nodes(self.netlist)
        self.matrixGen.set_time_step(self.h,1)
        self.matrixGen.time_domain_model()
        self.matrixGen.init_simulator()
        print('Creating buffers...')
        self.A_buffer = pynq.allocate((self.M * self.M_NUM + self.S * 2,self.M),'float32',target=self.getMemBank(self.k_reloader.args['A']))
        self.x0_buffer = pynq.allocate((self.sourcePeriod, self.M),'float32',target=self.getMemBank(self.k_controller.args['x0']))
        self.x0_buffer[:] = 0
        self.src_buffer = pynq.allocate((self.sourcePeriod,self.M),'float32',target=self.getMemBank(self.k_srcGen.args['src']))
        self.src_buffer[:] = 0
        self.sw_buffer = pynq.allocate((self.switchPeriod,1),'u1',target=self.getMemBank(self.k_swGen.args['sw']))
        self.result_buffer = pynq.allocate((self.timeSteps,self.M),'float32',target=self.getMemBank(self.k_resultBack.args['result']))
        self.ob_buffer = pynq.allocate((1,2),'float32',target=self.getMemBank(self.k_observer.args['ob']))
        print('Generate switch signals...')
        for i in range(self.switchPeriod):
            if i < self.switchPeriod / 2:
                self.sw_buffer[i] = 1
            else:
                self.sw_buffer[i] = 2
        self.sw_buffer.sync_to_device()

    def run(self):
        # start time
        ss = time.time()
        # regenerate matrixes
        self.matrixGen.init_simulator()
        # export matrixes to buffers
        self.matrixGen.export(self.A_buffer[:],self.x0_buffer,self.src_buffer,M=self.M,S=self.S)
        # synchronize to global memory
        self.A_buffer.sync_to_device()
        self.x0_buffer.sync_to_device()
        self.src_buffer.sync_to_device()
        # Phase 1: reload first
        self.k_reloader.call(self.A_buffer)
        # Phase 2: start senders and receivers
        sw_run = self.k_swGen.start(self.sw_buffer, self.switchPeriod, self.timeSteps)
        ob_run = self.k_observer.start(self.ob_buffer, self.RMS_ID, self.MEAN_ID, self.timeSteps)
        src_run = self.k_srcGen.start(self.src_buffer, self.sourcePeriod, self.timeSteps)
        result_grab = self.k_resultBack.start(self.result_buffer, self.timeSteps)
        # Phase 3: start controller
        ctrl_run = self.k_controller.start(self.x0_buffer, self.timeSteps)
        # Join all 'threads'
        sw_run.wait()
        src_run.wait()
        ctrl_run.wait()
        ob_run.wait()
        result_grab.wait()
        # get result: waveform is not necessary.
        # self.result_buffer.sync_from_device()
        self.ob_buffer.sync_from_device()
        # stop time
        ee = time.time()
        time_cost = ee - ss
        self.accumulated_time += time_cost
        self.simulation_count += 1
        #observe = np.zeros((1,2),dtype=np.float32)
        #observe[:] = self.ob_buffer[0,:]
        return self.ob_buffer.copy()

    def set_param(self,pname,pvalue):
        self.matrixGen.set_device(pname,pvalue)


