import numpy as np
import math

class ISWD:
    def __init__(self, device_name, node_list, nodes, voltage_list, v_name, ki, kv, vth, iA):
        self.is_on = False
        self.nodes = []
        self.device_name = device_name
        self.vth = vth
        self.Hon = np.mat(np.zeros(iA.shape))
        self.Hoff = np.mat(np.zeros(iA.shape))
        for node in nodes:
            self.nodes.append(node_list[node])
        self.vidx = len(node_list) + voltage_list[v_name] - 1
        # I factor
        x = self.nodes[0]
        y = self.nodes[1]
        i_base = self.vidx
        self.JON = np.mat(np.zeros((iA.shape[1], 1)))
        self.JOff = np.mat(np.zeros((iA.shape[1], 1)))
        self.JON[self.vidx] = 1
        if self.nodes[1] >= 0:
            self.JOff[self.nodes[1]] = 1
        if self.nodes[0] >= 0:
            self.JOff[self.nodes[0]] = -1
        if x >= 0:
            self.Hon[x, i_base] -= ki
            if y >= 0:
                self.Hoff[x, y] -= kv
            self.Hoff[x, x] += kv
        if y >= 0:
            self.Hon[y, i_base] += ki
            self.Hoff[y, y] += kv
            if x >= 0:
                self.Hoff[y, x] -= kv

        self.Hon = iA * self.Hon
        self.Hoff = iA * self.Hoff

    def update(self, x):
        on_j = self.JON.T * x
        off_j = self.JOff.T * x
        if self.is_on:
            self.is_on = on_j > 0
            return self.Hon * x
        else:
            self.is_on = off_j > 0
            return self.Hoff * x



class ISWS:
    def __init__(self, device_name, node_list, nodes, G_dev, ki, kv, iA):
        self.is_on = False
        self.nodes = []
        self.device_name = device_name
        self.Hon = np.mat(np.zeros(iA.shape))
        self.Hoff = np.mat(np.zeros(iA.shape))
        for node in nodes:
            self.nodes.append(node_list[node])
        self.rp = node_list[G_dev.nodes[0]]
        self.rn = node_list[G_dev.nodes[1]]
        # I factor
        x = self.nodes[0]
        y = self.nodes[1]
        self.last_x = None
        if x >= 0:
            if y >= 0:
                self.Hoff[x, y] -= kv
            self.Hoff[x, x] += kv
        if y >= 0:
            self.Hoff[y, y] += kv
            if x >= 0:
                self.Hoff[y, x] -= kv
        if self.rp >= 0:
            if x >= 0:
                self.Hon[x, self.rp] -= ki
            if y >= 0:
                self.Hon[y, self.rp] += ki
        if self.rn >= 0:
            if x >= 0:
                self.Hon[x, self.rn] += ki
            if y >= 0:
                self.Hon[y, self.rn] -= ki
        self.Hon = iA * self.Hon
        self.Hoff = iA * self.Hoff

    def update(self, x, on):
        if on:
            x_next =  self.Hon * x
        else:
            x_next =  self.Hoff * x
        return x_next