import numpy as np
import math
from multiprocessing import Process, Queue, Manager
from SW_Models import ISWD
from SW_Models import ISWS

K = int(96)
S = int(8)
class device:
    def __init__(self, device_type='', device_name='', nodes=[], value=0, ki=0, kv=0):
        self.nodes = nodes
        self.value = value
        self.device_type = device_type
        self.device_name = device_name
        self.ki = ki
        self.kv = kv

def EEstr2num(string):
    exp = 1
    idx = 0
    while (string[idx].isnumeric() == True) or (string[idx] == '.') or (string[idx] == '-'):
        idx = idx + 1
        if idx == len(string):
            break
    nv = float(string[0:idx])
    if idx == len(string):
        return nv

    if string[idx] == 'e':
        exp = int(string[idx + 1:])
    elif string[idx:] == 'm':
        exp = -3
    elif string[idx:] == 'M':
        exp = 6
    elif string[idx:] == 'Meg':
        exp = 6
    elif string[idx:] == 'K':
        exp = 3
    elif string[idx:] == 'k':
        exp = 3
    elif string[idx:] == 'G':
        exp = 9
    elif string[idx:] == 'g':
        exp = 9
    elif string[idx:] == 'µ':
        exp = -6
    elif string[idx:] == 'u':
        exp = -6
    elif string[idx:] == 'n':
        exp = -9
    elif string[idx:] == 'p':
        exp = -12
    elif string[idx:] == 'f':
        exp = -15

    return nv * math.pow(10, exp)


class MNA:
    def __init__(self):
        self.static = None
        self.iAH = None
        self.x = None
        self.x0 = None
        self.I = None
        self.A = None
        self.C = None
        self.B = None
        self.G = None
        self.Csw = 1e-9
        self.Lsw = 1e-9
        self.device_list = {}
        self.node_list = {'GND': -1}
        self.h = 2e-9
        self.td_device_list = {}
        self.td_node_list = {'GND': -1}
        self.voltage_list = {}
        self.current_list = {}
        self.diode_list = {}
        self.switch_list = {}
        self.result = {}

    def assign_nodes(self, node_string, dev):
        for node in node_string:
            if node not in self.node_list:
                if node == '0' or node == 'GND' or node == 'Gnd' or node == 'gnd':
                    dev.nodes.append('GND')
                else:
                    dev.nodes.append(node)
                    self.node_list[node] = len(self.node_list) - 1
            else:
                if node == '0' or node == 'GND' or node == 'Gnd' or node == 'gnd':
                    dev.nodes.append('GND')
                else:
                    dev.nodes.append(node)
        return dev

    def load_nodes(self, netlist):
        M_num = 4 * S + 2
        self.device_list = {}
        self.diode_list = {}
        self.switch_list = {}
        self.node_list = {'GND': -1}
        for line in netlist:
            new_device = device(nodes=[])
            line = line.split()
            new_device.device_name = line[0]
            new_device.device_type = line[0][0]

            if new_device.device_type == 'K':
                new_device.value = EEstr2num(line[3])
                new_device.nodes.append(line[1])
                new_device.nodes.append(line[2])
            elif new_device.device_type == 'V':
                new_device.value = EEstr2num(line[3])
                new_device = self.assign_nodes(line[1:3], new_device)
            elif new_device.device_type == 'I':
                new_device.value = EEstr2num(line[3])
                new_device = self.assign_nodes(line[1:3], new_device)
            elif new_device.device_type == 'D':
                new_device.value = EEstr2num(line[3])
                new_device = self.assign_nodes(line[1:3], new_device)
            elif new_device.device_type == 'S':
                new_device.value = EEstr2num(line[3])
                new_device = self.assign_nodes(line[1:3], new_device)
            elif new_device.device_type == 'F':
                new_device.value = EEstr2num(line[4])
                new_device = self.assign_nodes(line[1:3], new_device)
                new_device.nodes.append(line[3])
            elif new_device.device_type == 'E':
                new_device.value = EEstr2num(line[5])
                new_device = self.assign_nodes(line[1:5], new_device)
            elif new_device.device_type == 'G':
                new_device.value = EEstr2num(line[5])
                new_device = self.assign_nodes(line[1:5], new_device)
            elif new_device.device_type == 'H':
                new_device.value = EEstr2num(line[4])
                new_device = self.assign_nodes(line[1:3], new_device)
                new_device.nodes.append(line[3])
            elif new_device.device_type == 'A':
                new_device.device_type = 'E'
                new_device.device_name = 'E' + new_device.device_name
                new_device.value = EEstr2num(line[4])
                new_device = self.assign_nodes([line[3], 'GND', line[1], line[2]], new_device)
            else:
                new_device.value = EEstr2num(line[3])
                new_device = self.assign_nodes(line[1:3], new_device)

            self.device_list[new_device.device_name] = new_device
            del new_device

        #self.time_domain_model()
        return self.device_list

    def time_domain_model(self):
        self.td_device_list = self.device_list.copy()
        self.td_node_list = self.node_list.copy()
        for name, dev in self.td_device_list.copy().items():
            if dev.device_type == 'C':
                # create mid-point
                midpoint = 'N' + dev.device_name
                self.td_node_list[midpoint] = len(self.td_node_list) - 1
                self.td_device_list['R' + dev.device_name] = \
                    device('R', 'R' + dev.device_name, [midpoint, dev.nodes[1]], self.h / (2 * dev.value))
                self.td_device_list['V' + dev.device_name] = device('V', 'V' + dev.device_name,
                                                                    [dev.nodes[0], midpoint], 0)
                self.td_device_list['ID' + dev.device_name] = \
                    device('ID', 'ID' + dev.device_name, [dev.nodes[1], midpoint], 0, 1, 2 * dev.value / self.h)
                self.td_device_list.pop(name)
            elif dev.device_type == 'L':
                # create mid-point
                midpoint = 'N' + dev.device_name
                self.td_node_list[midpoint] = len(self.td_node_list) - 1
                self.td_device_list['R' + dev.device_name] = \
                    device('R', 'R' + dev.device_name, [midpoint, dev.nodes[1]], 2 * dev.value / self.h)
                self.td_device_list['V' + dev.device_name] = device('V', 'V' + dev.device_name,
                                                                    [dev.nodes[0], midpoint], 0)
                self.td_device_list['ID' + dev.device_name] = \
                    device('ID', 'ID' + dev.device_name, [dev.nodes[1], midpoint], 0, -1, -self.h / (2 * dev.value))
                self.td_device_list.pop(name)
            elif dev.device_type == 'D':
                # create mid-point
                midpoint = 'N' + dev.device_name
                self.td_node_list[midpoint] = len(self.td_node_list) - 1
                self.td_device_list['R' + dev.device_name] = \
                    device('R', 'R' + dev.device_name, [midpoint, dev.nodes[1]], self.h / self.Csw)
                self.td_device_list['V' + dev.device_name] = device('V', 'V' + dev.device_name,
                                                                    [dev.nodes[0], midpoint], dev.value)
                self.td_device_list['ISD' + dev.device_name] = \
                    device('ISD', 'ISD' + dev.device_name, [dev.nodes[1], midpoint], dev.value, -1,
                            self.Csw / self.h)
                self.td_device_list.pop(name)
            elif dev.device_type == 'K':
                L1 = self.device_list[dev.nodes[0]]
                L2 = self.device_list[dev.nodes[1]]
                L1_midpoint = 'N' + L1.device_name
                L2_midpoint = 'N' + L2.device_name
                M = math.sqrt(L1.value * L2.value)
                M12 = dev.value * M / L1.value
                M21 = dev.value * M / L2.value
                self.td_device_list['IDM' + L1.device_name + L2.device_name] = \
                    device('IDM', 'IDM' + L1.device_name + L2.device_name, [L1.nodes[1], L1_midpoint, 'V' + L2.device_name], 0, -M12, 0)
                self.td_device_list['IDM' + L2.device_name + L1.device_name] = \
                    device('IDM', 'IDM' + L2.device_name + L1.device_name, [L2.nodes[1], L2_midpoint, 'V' + L1.device_name], 0, -M21, 0)
                self.td_device_list['F' + L1.device_name + L2.device_name] = \
                    device('F', 'F' + L1.device_name + L2.device_name, [L1.nodes[1], L1_midpoint, 'V' + L2.device_name], M12, 0, 0)
                self.td_device_list['F' + L2.device_name + L1.device_name] = \
                    device('F', 'F' + L2.device_name + L1.device_name, [L2.nodes[1], L2_midpoint, 'V' + L1.device_name], M21, 0, 0)
                self.td_device_list.pop(name)
            elif dev.device_type == 'S':
                midpoint = 'N' + dev.device_name
                self.td_node_list[midpoint] = len(self.td_node_list) - 1
                self.td_device_list['RSW' + dev.device_name] = \
                    device('R', 'RSW' + dev.device_name, [dev.nodes[0],midpoint], dev.value)
                self.td_device_list['R' + dev.device_name] = \
                    device('R', 'R' + dev.device_name, [midpoint, dev.nodes[1]], self.h / self.Csw)
                self.td_device_list['ISD' + dev.device_name] = \
                    device('ISD', 'ISD' + dev.device_name, [dev.nodes[1], midpoint], 0, -1/dev.value, (1 * self.Csw) / self.h)
                self.td_device_list.pop(name)
            elif dev.device_type == 'E':
                self.td_device_list['V' + dev.device_name] = device('V', 'V' + dev.device_name,
                                                                    [dev.nodes[0], dev.nodes[1]], 0)
            elif dev.device_type == 'H':
                self.td_device_list['V' + dev.device_name] = device('V', 'V' + dev.device_name,
                                                                    [dev.nodes[0], dev.nodes[1]], 0)
        for name, dev in self.td_device_list.items():
            if dev.device_type == 'V':
                self.voltage_list[name] = len(self.voltage_list)
            if dev.device_type == 'I':
                self.current_list[name] = dev

    def create_matrix(self):
        number_of_voltages = len(self.voltage_list)
        self.G = np.mat(np.zeros([len(self.td_node_list) - 1, len(self.td_node_list) - 1]))
        self.B = np.mat(np.zeros([len(self.td_node_list) - 1, number_of_voltages]))
        self.C = np.mat(np.zeros([number_of_voltages, number_of_voltages]))
        for name, dev in self.td_device_list.items():
            if dev.device_type == 'R':
                x = self.td_node_list[dev.nodes[0]]
                y = self.td_node_list[dev.nodes[1]]
                if x >= 0 and y >= 0:
                    self.G[x, x] += 1 / dev.value
                    self.G[y, y] += 1 / dev.value
                    self.G[x, y] -= 1 / dev.value
                    self.G[y, x] -= 1 / dev.value
                else:
                    x = np.max([x, y])
                    self.G[x, x] += 1 / dev.value

            elif dev.device_type == 'V':
                x = self.td_node_list[dev.nodes[0]]
                y = self.td_node_list[dev.nodes[1]]
                if x >= 0:
                    self.B[x, self.voltage_list[dev.device_name]] = 1
                if y >= 0:
                    self.B[y, self.voltage_list[dev.device_name]] = -1

    def create_A_matrix(self):
        H1 = np.concatenate((self.G, self.B), axis=1)
        H2 = np.concatenate((self.B.T, self.C), axis=1)
        self.A = np.concatenate((H1, H2), axis=0)
        for name, dev in self.td_device_list.items():
            if dev.device_type == 'F':
                x = self.td_node_list[dev.nodes[0]]
                y = self.td_node_list[dev.nodes[1]]
                i_base = len(self.td_node_list) + self.voltage_list[dev.nodes[2]] - 1
                if x >= 0:
                    self.A[x, i_base] += dev.value
                if y >= 0:
                    self.A[y, i_base] -= dev.value
            elif dev.device_type == 'H': #Hxxxxx N+ N- Vxxxxx Value
                v_bias = len(self.td_node_list) + self.voltage_list['V' + name] - 1
                i_bias = len(self.td_node_list) + self.voltage_list[dev.nodes[2]] - 1
                if v_bias >= 0:
                    if i_bias >= 0:
                        self.A[v_bias, i_bias] -= dev.value
            elif dev.device_type == 'E':
                v_bias = len(self.td_node_list) + self.voltage_list['V' + name] - 1
                x = self.td_node_list[dev.nodes[2]]
                y = self.td_node_list[dev.nodes[3]]
                if x >= 0:
                    self.A[v_bias, x] -= dev.value
                if y >= 0:
                    self.A[v_bias, y] += dev.value
            elif dev.device_type == 'G': #Gxxxxx N+ N- NC+ NC- Value
                i_bias_p = self.td_node_list[dev.nodes[0]]
                i_bias_n = self.td_node_list[dev.nodes[1]]
                x = self.td_node_list[dev.nodes[2]]
                y = self.td_node_list[dev.nodes[3]]
                if i_bias_p >= 0:
                    if x >= 0:
                        self.A[i_bias_p, x] += dev.value
                    if y >= 0:
                        self.A[i_bias_p, y] -= dev.value
                if i_bias_n >= 0:
                    if x >= 0:
                        self.A[i_bias_n, x] -= dev.value
                    if y >= 0:
                        self.A[i_bias_n, y] += dev.value
        self.iA = np.linalg.inv(self.A)
        return self.A

    def create_I_matrix(self):
        self.I = np.matrix(np.zeros(self.A.shape))

        for name, dev in self.td_device_list.items():
            if dev.device_type == 'ID':
                # I factor
                v_name = dev.device_name.replace('ID', 'V')
                x = self.td_node_list[dev.nodes[0]]
                y = self.td_node_list[dev.nodes[1]]
                i_base = len(self.td_node_list) + self.voltage_list[v_name] - 1
                if x >= 0:
                    self.I[x, i_base] -= dev.ki
                    if y >= 0:
                        self.I[x, y] -= dev.kv
                    self.I[x, x] += dev.kv
                if y >= 0:
                    self.I[y, i_base] += dev.ki
                    self.I[y, y] += dev.kv
                    if x >= 0:
                        self.I[y, x] -= dev.kv
            elif dev.device_type == 'IDM':
                x = self.td_node_list[dev.nodes[0]]
                y = self.td_node_list[dev.nodes[1]]
                i_base = len(self.td_node_list) + self.voltage_list[dev.nodes[2]] - 1
                if x >= 0:
                    self.I[x, i_base] -= dev.ki
                if y >= 0:
                    self.I[y, i_base] += dev.ki
        # cannot be done before all devices are modeled
        for name, dev in self.device_list.items():
            if dev.device_type == 'S':
                IDev = self.td_device_list['ISD' + name]
                G_dev = self.td_device_list['RSW' + name]
                self.switch_list[name] = ISWS(name, self.td_node_list, IDev.nodes, G_dev, IDev.ki, IDev.kv, self.iA)
        for name, dev in self.device_list.items():
            if dev.device_type == 'D':
                IDev = self.td_device_list['ISD' + name]
                self.diode_list[name] = ISWD(name, self.td_node_list, IDev.nodes, self.voltage_list, 'V' + name, IDev.ki, IDev.kv, 0, self.iA)
        return self.I

    def set_time_step(self, h, scale = 1):
        self.h = h
        self.Lsw = h * scale
        self.Csw = h * h / self.Lsw

    def set_device(self, name, value):
        if name in self.device_list.keys():
            dev = self.device_list[name]
            dev.value = value
            if dev.device_type == 'C':
                self.td_device_list['R' + dev.device_name].value =  self.h / (2 * dev.value)
                self.td_device_list['ID' + dev.device_name].kv = 2 * dev.value / self.h
            elif dev.device_type == 'L':
                self.td_device_list['R' + dev.device_name].value = 2 * dev.value / self.h
                self.td_device_list['ID' + dev.device_name].kv =  -self.h / (2 * dev.value)
            elif dev.device_type == 'K':
                L1 = self.device_list[dev.nodes[0]]
                L2 = self.device_list[dev.nodes[1]]
                M = math.sqrt(L1.value * L2.value)
                M12 = dev.value * M / L1.value
                M21 = dev.value * M / L2.value
                self.td_device_list['IDM' + L1.device_name + L2.device_name].ki = -M12
                self.td_device_list['IDM' + L2.device_name + L1.device_name].ki = -M21
                self.td_device_list['F' + L1.device_name + L2.device_name].value = M12
                self.td_device_list['F' + L2.device_name + L1.device_name].value = M21
            elif dev.device_type == 'S':
                self.td_device_list['RSW' + dev.device_name].value = dev.value

    def init_simulator(self):
        #self.list_models()
        self.create_matrix()
        self.create_A_matrix()
        self.create_I_matrix()
        #self.list_nodes()
        self.x = np.mat(np.zeros([len(self.td_node_list) - 1 + len(self.voltage_list), 1]))
        self.iAH = self.iA * self.I
        self.get_static_response()

    def describe_result(self):
        name_list = {}
        idx = -1
        for name, dev in self.td_node_list.items():
            name_list[name] = idx
            idx += 1

        for voltage_src,_ in self.voltage_list.items():
            name_list['I' + voltage_src] = idx
            idx += 1
        return name_list

    def list_devices(self):
        print('Original Devices:')
        for name, dev in self.device_list.items():
            if dev.device_type != 'M' and dev.device_type != 'K':
                print('\t%s:\t%s\t%s(%d)\t%s(%d)\tvalue = %f' % (
                    dev.device_type, dev.device_name, dev.nodes[0], self.node_list[dev.nodes[0]],
                    dev.nodes[1], self.node_list[dev.nodes[1]], dev.value))
            else:
                pass

    def list_models(self):
        print('Modeled Devices:')
        for name, dev in self.td_device_list.items():
            if dev.device_type != 'M':
                print('\t%s:\t%s\t%s(%d)\t%s(%d)\tvalue = %f' % (
                    dev.device_type, dev.device_name, dev.nodes[0],
                    self.td_node_list[dev.nodes[0]],
                    dev.nodes[1], self.td_node_list[dev.nodes[1]], dev.value))
            else:
                pass
    def list_nodes(self):
        print('Node List:')
        for name, num in self.td_node_list.items():
            print('\t%s\t->\t%d' % (name,num))

    def update_diodes(self):
        x_diode = np.mat(np.zeros(self.x.shape))
        for name, diode in self.diode_list.items():
            x_diode = x_diode + diode.update(self.x)
        return x_diode

    def update_switches(self, sw, is_on):
        x_sw = np.mat(np.zeros(self.x.shape))
        for switch, on in zip(sw, is_on):
            x_sw = x_sw + self.switch_list[switch].update(self.x, on)
        return x_sw

    def step(self, vs=[], vol=[], cs = [], cur = [],sw=[], on=[]):
        x_ids = self.iAH * self.x
        # src = np.mat(np.zeros([len(self.td_node_list) - 1 + len(self.voltage_list), 1]))
        # for v in self.voltage_list:
        #     if v in vs:
        #         src[self.voltage_list[v] + len(self.td_node_list) - 1] = vol[vs.index(v)]
        #     else:
        #         src[self.voltage_list[v] + len(self.td_node_list) - 1] = self.td_device_list[v].value
        #
        # for name, dev in self.current_list.items():
        #     if name in cs:
        #         i_src = cur[cs.index(name)]
        #     else:
        #         i_src = dev.value
        #     if(self.td_node_list[dev.nodes[0]] >= 0):
        #         src[self.td_node_list[dev.nodes[0]]] -= i_src
        #     if(self.td_node_list[dev.nodes[1]] >= 0):
        #         src[self.td_node_list[dev.nodes[1]]] += i_src
        # x_src = self.iA * src
        x_diode = self.update_diodes()
        x_sw = self.update_switches(sw, on)
        self.x = x_sw + x_ids + x_diode + self.static
        return self.x.copy()

    def get_static_response(self):
        src = np.mat(np.zeros([len(self.td_node_list) - 1 + len(self.voltage_list), 1]))
        for v in self.voltage_list:
            src[self.voltage_list[v] + len(self.td_node_list) - 1] = self.td_device_list[v].value

        for name, dev in self.current_list.items():
            i_src = dev.value
            if self.td_node_list[dev.nodes[0]] >= 0:
                src[self.td_node_list[dev.nodes[0]]] -= i_src
            if self.td_node_list[dev.nodes[1]] >= 0:
                src[self.td_node_list[dev.nodes[1]]] += i_src
        x_src = self.iA * src
        self.x0 = src
        self.static = x_src
        return x_src

    def export(self,filename = None):
        Rows = 2 * K + 4 * S + 4 * S + 2 * S + 4
        R_size = self.iAH.shape[0]
        A = np.matrix(np.zeros([Rows ,K],np.float32))
        A[0:R_size,0:R_size] = self.iA
        A[(K):(K + R_size),0:R_size] = self.iAH
        if len(self.switch_list) > 0:
            offset = 2 * K
            for s in range(len(self.switch_list)):
                try:
                    A[(offset + s * 4) : (offset + s * 4 + 2),0:R_size] = list(self.switch_list.values())[s].Hoff_valid
                    A[(offset + s * 4 + 2) : (offset + s * 4 + 4),0:R_size] = list(self.switch_list.values())[s].Hon_valid
                except:
                    pass

        if len(self.diode_list) > 0:
            offset = 2 * K + 4 * S
            for s in range(len(self.diode_list)):
                A[(offset + s * 4) : (offset + s * 4 + 2),0:R_size] = list(self.diode_list.values())[s].Hoff_valid
                A[(offset + s * 4 + 2) : (offset + s * 4 + 4),0:R_size]  = list(self.diode_list.values())[s].Hon_valid

            offset = 2 * K + 4 * S + 4 * S
            for s in range(len(self.diode_list)):
                A[(offset + s * 2),0:R_size] = list(self.diode_list.values())[s].JOff.T
                A[(offset + s * 2 + 1),0:R_size] = list(self.diode_list.values())[s].JON.T
        offset = 2 * K + 4 * S + 4 * S + 2 * S
        A[offset,0:R_size] = self.x0.T
        A[(offset + 1),0:R_size] = self.static.T

        if len(self.switch_list) > 0:
            offset = 2 * K + 4 * S + 4 * S + 2 * S + 2
            for s in range(len(self.switch_list)):
                A[offset,s * 4:s * 4 + 2] = list(self.switch_list.values())[s].related_idx_off
                A[offset,s * 4 + 2:s * 4 + 4] = list(self.switch_list.values())[s].related_idx_on
        
        if len(self.diode_list) > 0:
            offset = 2 * K + 4 * S + 4 * S + 2 * S + 3
            for s in range(len(self.diode_list)):
                A[offset,s*4:s*4+2] = list(self.diode_list.values())[s].related_idx_off
                A[offset,s*4+2:s*4+4] = list(self.diode_list.values())[s].related_idx_on

        if filename == None:
            return A
        else:
            np.savetxt(filename,A)
            return A



