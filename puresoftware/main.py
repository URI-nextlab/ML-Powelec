import code
import MNA
import math
import numpy as np
import copy
import matplotlib.pyplot as plt
from alive_progress import alive_bar
import time
# netlist = [
#          'Vs Vs 0 400',
#          'S1 Vs Vin .1',
#          'Vaux1 Vin Vinp 0',
#          'S2 Vinp 0 .1',
#          'Cr Vinp Vcl 24n',
#          'Lr Vcl Vpp 60u',
#          'L1 Vpp 0 280u',
#          'L2 Vd1 0 968n',
#          'L3 0 Vd2 968n',
#          'D1 Vd1 Vtout1 0',
#          'D2 Vd2 Vtout2 0',
#          'Rds1 Vtout1 Vout .001',
#          'Rds2 Vtout2 Vout .001',
#          'CL Vout 0 1000u',
#          'RL Vout 0 0.48',
#          'K1 L1 L2 1',
#          'K2 L2 L3 1',
#          'K3 L1 L3 1'
#     ]

if __name__ == '__main__':
    cir = MNA.MNA()
    netlist = [
         'Vs Vs 0 400',
         'S1 Vs Vinp 0.1',
         'S2 Vinp 0 0.1',
         'S3 Vs Vinn 0.1',
         'S4 Vinn 0 0.1',
         'Lpri Vinp Vin 8e-6',
         'Lm Vin Vinn 400e-6',
         'L1 Vop Vcm 223e-6',
         'L2 Vcm Von 223e-6',
         'K1 Lm L1 1',
         'K2 Lm L2 1',
         'K3 L1 L2 1',
         'D1 Vop Vto 0',
         'D2 0 Vop 0',
         'D3 Von Vto 0',
         'D4 0 Von 0',
         'D5 Vcm Vmid 0',
         'D6 Vmid Vto 0',
         'Cc Vmid 0 1e-6',
         'Lo Vto Vo 250e-6',
         'Co Vo 0 1e-3',
         'RL Vo 0 0.48'
    ]
    # netlist = [
    #    'Vs Vs 0 10',
    #    'R1 Vs Vout 5',
    #    'D1 Vout 0 5',
    #    'C1 Vout 0 10u',
    # ]
    ss = time.time()
    cir.load_nodes(netlist)
    h = 0.5e-7
    cir.set_time_step(h,1)
    cir.time_domain_model()

    ee1 = time.time()


    cir.init_simulator()
    ee2 = time.time()
    ee3 = time.time()

    print("Time spent on create netlists: %.2f | %.2f | %.2f us!" % ((ee1-ss)*1e6,(ee2-ee1)*1e6,(ee3-ee2)*1e6))
    f = 100e3
    Period = 500
    T = Period / f
    N = int(T / h)
    x = np.zeros([N, 3])
    t_index = np.arange(0, N)
    t_index = t_index * h
    vs = np.zeros([N,1])
    vs = 1 * np.sin(2 * np.pi * f * t_index)
    sw1 = vs < 0.1
    sw2 = vs >=0.1
    vs = 1 * np.sin(2 * np.pi * f * t_index+ np.pi/4)
    sw3 = vs >= 0.1
    sw4 = vs < 0.1
    #cir.list_models()
    print('Required Matrix Dimention %d!' % (len(cir.x0)))
    labels = cir.describe_result()
    print("Matrix label:")
    for name, idx in labels.items():
        print(name + ":\t" + str(idx))
    print(cir.switch_list)
    res = []

    A = cir.export('to_aws.txt')
    with alive_bar(N, force_tty=True) as bar:
        for i in range(N):
            #temp = cir.step()
            #temp = cir.step(vs=['Vs'],vol=[vs[i]])
            #temp = cir.step(sw=['S1'], on=[sw1[i]])
            temp = cir.step(sw=['S1','S2','S3','S4'], on=[sw1[i],sw2[i],sw3[i],sw4[i]])
            #temp = cir.step(sw=['S1','S2','S3','S4'], on=[sw1[i],sw2[i],sw1[i],sw2[i]])
            #temp = cir.step(sw=['S1', 'S2', 'S3', 'S4'], on=[sw1[i],  sw2[i], sw2[i], sw1[i]])
            # x[i] = cir.diode_list['D1']
            x[i, 0] = temp[labels['Vo']]
            x[i, 1] = temp[labels['IVD1']]
            x[i, 2] = temp[labels['IVD2']]
            res.append(temp.copy())
            #print(cir.diode_list['D1'].is_on)
            bar()

    fig = plt.figure(dpi=100)
    #print(np.mean(x[-1000:,0))

    aws = np.loadtxt('res.txt')
    plt.subplot(3, 1, 1)
    plt.plot(t_index * 1e6, x[:, 0],linestyle='-')
    # plt.plot(t_index[1:3000] * 1e6, aws[1:3000, labels['Vout']])
    # plt.legend(['Software', 'Hardware'])
    plt.grid(True)
    #plt.xlim((0,30))
    plt.xlabel('t (us)')
    plt.ylabel('V (V)')
    plt.subplot(3, 1, 2)
    plt.plot(t_index * 1e6, x[:, 1],linestyle='-')
    # plt.plot(t_index[1:3000] * 1e6, aws[1:3000, labels['IVD1']]*.99)
    # plt.legend(['Software', 'Hardware'])

    plt.subplot(3, 1, 3)
    plt.plot(t_index * 1e6, x[:, 2],linestyle='-')
    # plt.plot(t_index[1:3000] * 1e6, aws[1:3000, labels['IVD2']]*.99)
    # plt.legend(['Software', 'Hardware'])
    # # plt.xlabel('t (us)')
    # # plt.ylabel('I (AMP)')

    plt.show()
