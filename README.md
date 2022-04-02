# ML-Powelec

This is project aims at accelerate the circuit simulation process when using machine learining to train the power converter designer. The algorithm is based on AutoCkt [^AutoCkt][^AutoCktPaper]. An FPGA accelerator is implemneted via xilinx vitis to accelerate the circuit simulation. Half-bridge LLC converter is used as the example. This project contains:
>
>- A pythong script that transfer spice like netlist into accelerator required matrixes and vectors.
>- A Xilinx Vitis project to create the hardware accelerator.
>- The training and forwarding script that used to train the circuit designer. A customed GYM environment that use FPGA accelerator to do the simulation.

# RUN

To run the project, an AWS F1 instance is required. The following python packages (and there dependencies) are required:
>
>- ray
>- ray[rllib]
>- pynq  

These three packages may not be installed correctly with conda. If so, please use `python3 -m pip install <package name>`  

To run the training, run following teriminal commands.
```shell
git clone https://github.com/aws/aws-fpga.git
cd aws-fpga
source vitis_setup.sh
cd ..
git clone https://github.com/URI-nextlab/ML-Powelec.git
cd ML-Powelec/fpga_train
ipython
run train.py
```

[^AutoCkt]: <https://github.com/ksettaluri6/AutoCkt.git>
[^AutoCktPaper]: Settaluri, K., Haj-Ali, A., Huang, Q., Hakhamaneshi, K., & Nikolic, B. (2020). AutoCkt: Deep Reinforcement Learning of Analog Circuit Designs. Proceedings of the 2020 Design, Automation and Test in Europe Conference and Exhibition, DATE 2020, 490–495. <https://doi.org/10.23919/DATE48585.2020.9116200>
