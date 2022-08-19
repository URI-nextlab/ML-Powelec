from FPGA_Spice_Accelerator import FPGA_Spice_Accelerator
import matplotlib.pyplot as plt
import numpy as np
import gym

class half_llc_env(gym.Env):
    metadata = {'render.modes' : ['human']}
    def __init__(self,config):
        self.training_step = 0
        self.ostar = 300
        self.vname = ['Lr','Cr']
        self.vrange_low = [1e-6,1e-9]
        self.vrange_high = [110e-6,110e-9]
        self.netlist = config.ge
        self.simulator = FPGA_Spice_Accelerator('spice.awsxclbin')
        self.simulator.set_simulator(netlist=self.netlist,timeSteps=10000,h=1e-7,swPeriod=100,srcPeriod=1,M=48,S=2,RMS_ID=20,MEAN_ID=7)
        self.action_meaning = [-1,0,1]
        self.parameters = list([.5,.5])
        self.preset = list([.6,.4])
        self.observation_space = gym.spaces.Box(
            low=np.array([0,0,11,-np.inf,-np.inf]),
            high=np.array([1,1,13,np.inf,np.inf])
        )
        self.action_space = gym.spaces.Tuple([gym.spaces.Discrete(len(self.action_meaning))] * len(self.parameters))

    def observe(self,circuit_observation):
        dc_out = circuit_observation[0,0]
        rms_current = np.sqrt(circuit_observation[0,1])
        new_observation = self.parameters.copy()
        new_observation.append(self.ostar)
        new_observation.append(dc_out)
        new_observation.append(rms_current)
        self.observation = np.array(new_observation)
        return dc_out,rms_current

    def run_sim(self):
        for i in range(len(self.parameters)):
            value = self.exponatial_map(self.parameters[i],self.vrange_low[i],self.vrange_high[i])
            self.simulator.set_param(self.vname[i],value)
        circuit_observation = self.simulator.run()
        return circuit_observation

    def exponatial_map(self, k, vmin=1e-6,vmax = 100e-6):
        num = int(k / 0.02)
        uval = vmin * (1.15 ** num)
        return uval

    def inrange_reward(self, target, error, value):
        delta = target * error
        denom = (delta ** 2) - ((value - target) ** 2)
        nom = (delta ** 2) + ((value - target) ** 2)

        return np.min([denom/nom,.01])

    def lower_reward(self, up_bond, value):
        return (up_bond-value)/(up_bond+value)

    def upper_reward(self, low_bond, value):
        return (value-low_bond)/(low_bond+value)

    def reset(self, rand = True, target = 12):
        if rand:
            self.ostar = np.random.randint(11,14)
        else:
            self.ostar = target
        self.accumulated_reward = 0
        self.training_step = 0
        for i in range(len(self.parameters)):
            self.parameters[i] = self.preset[i]
        circuit_observation = self.run_sim()
        self.observe(circuit_observation)
        return self.observation

    def step(self, action):
        self.training_step += 1
        action = list(np.reshape(np.array(action),(np.array(action).shape[0],)))
        for i in range(len(self.parameters)):
            a = action[i]
            if self.action_meaning[a] == -1:
                self.parameters[i] -= 0.02
            elif self.action_meaning[a] == 1:
                self.parameters[i] += 0.04
        self.parameters = np.clip(self.parameters,0,1).tolist()
        circuit_observation = self.run_sim()

        dc_out,rms_current = self.observe(circuit_observation)
        voltage_gain_reward = self.inrange_reward(self.ostar,.05,dc_out)
        reward = voltage_gain_reward
        if voltage_gain_reward >= 0.001:
            pout = (self.ostar ** 2) / 0.48
            pin = 200 * rms_current
            eff = pout / pin
            if eff >= (self.best_so_far[str(self.ostar)] * self.best_radias[str(self.ostar)]):
                if eff > self.best_so_far[str(self.ostar)]:
                    self.best_so_far[str(self.ostar)] = eff
                    self.best_radias[str(self.ostar)] = 0.95
                    reward += 100
                else:
                    self.best_radias[str(self.ostar)] *= 1.01
                    if self.best_radias[str(self.ostar)] > 0.98:
                        self.best_radias[str(self.ostar)] = 0.98
                        reward += 10
                    else:
                        reward += 5
                reward += 10 * (eff ** 3)
                done = True
            else:
                if self.training_step > 100:
                    reward += eff ** 3
                    done = True
                else:
                    reward += eff ** 3
                    done = False
        else:
            done = False

        if self.training_step > 200:
            eff = 0
            done = True

        for param in self.parameters:
            if param == 0 or param == 1:
                reward -= 100
                eff = 0
                done = True
        if done == True:
            print('!!! Finish at:', self.training_step, 'Reward = ', "%.2f" % (reward), 'eff = ', "%.2f" % eff)
            print('!!! Observation = ', ["%.2f" % value for value in self.observation])
            print('Simulation time: %f @ %f' % (self.simulator.accumulated_time, (self.simulator.accumulated_time/self.simulator.simulation_count)))
            print(" ")
        elif self.training_step % 50 == 0:
            print('    %% Traning Step:', self.training_step, 'Reward = ', reward)
            print('    %% Observation = ', ["%.2f" % value for value in self.observation])
            print(" ")

        self.accumulated_reward += reward
        return self.observation, reward, done, {}
