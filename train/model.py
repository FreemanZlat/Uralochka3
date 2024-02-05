import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
from feature_transformer import DoubleFeatureTransformerSlice


K_SIZE = 16
P_SIZE = 768
S_SIZE = 6

# PADDING = 0
PADDING = 32 - S_SIZE   # 32 - size of avx512 registry

HIDDEN_SIZE = 1024

L1_INPUT_SIZE = K_SIZE * P_SIZE
# L1_OUTPUT_SIZE = HIDDEN_SIZE
# L1_OUTPUT_SIZE_P = L1_OUTPUT_SIZE
L1_OUTPUT_SIZE = HIDDEN_SIZE + S_SIZE
L1_OUTPUT_SIZE_P = L1_OUTPUT_SIZE + PADDING

L2_INPUT_SIZE = HIDDEN_SIZE * 2
L2_OUTPUT_SIZE = S_SIZE

QUANTIZATION_COEFF_L1 = 64
QUANTIZATION_COEFF_L2 = 512


# class NetOld(nn.Module):
#     def __init__(self):
#         super(NetOld, self).__init__()
#         self.layer1 = nn.Linear(L1_INPUT_SIZE, HIDDEN_SIZE)
#         self.layer2 = nn.Linear(HIDDEN_SIZE*2, 1)
#
#     def forward(self, x, y):
#         x = self.layer1(x)
#         x = F.relu(x)
#         y = self.layer1(y)
#         y = F.relu(y)
#         c = torch.cat((x, y), dim=1)
#         #c = F.relu(c)
#         #c = F.mish(c)
#         c = self.layer2(c)
#         return torch.sigmoid(c)


class Net(nn.Module):
    def __init__(self, eval_divider):
        super(Net, self).__init__()
        self.layer1 = DoubleFeatureTransformerSlice(L1_INPUT_SIZE, L1_OUTPUT_SIZE)
        self.layer2 = nn.Linear(L2_INPUT_SIZE, L2_OUTPUT_SIZE)

        self.psqt_coeff = eval_divider / QUANTIZATION_COEFF_L1

        pieces = [110.0, 370.0, 364.0, 589.0, 1138.0, -110.0, -370.0, -364.0, -589.0, -1138.0, 0.0, 0.0]

        for i in range(S_SIZE):
            self.layer1.bias.data[HIDDEN_SIZE + i] = 0.0
            for k in range(K_SIZE):
                for p in range(12):
                    for sq in range(64):
                        self.layer1.weight.data[k * P_SIZE + p * 64 + sq, HIDDEN_SIZE + i] = pieces[p] *\
                                                                                             self.psqt_coeff /\
                                                                                             eval_divider

    def forward(self, x, y, v, s):
        x, y = self.layer1(x, v, y, v)

        # x = F.relu(x)
        # y = F.relu(y)
        # c = torch.cat((x, y), dim=1)
        # c = self.layer2(c)
        # c = c.gather(1, s)

        x1, x2 = torch.split(x, x.shape[1]-S_SIZE, dim=1)
        x1 = F.relu(x1)
        y1, y2 = torch.split(y, y.shape[1]-S_SIZE, dim=1)
        y1 = F.relu(y1)
        c = torch.cat((x1, y1), dim=1)
        c = self.layer2(c) + (x2 - y2) * (0.5 / self.psqt_coeff)
        c = c.gather(1, s)

        return torch.sigmoid(c)

    def save_model(self, model_name, nn_name, optimizer, epoch):
        if model_name is not None:
            torch.save({'epoch': epoch,
                        'model': self.state_dict(),
                        'optimizer': optimizer.state_dict()},
                       model_name)

        if nn_name is None:
            return

        net_size = L1_OUTPUT_SIZE_P + L1_INPUT_SIZE * L1_OUTPUT_SIZE_P + \
                   L2_OUTPUT_SIZE + L2_INPUT_SIZE * L2_OUTPUT_SIZE
        net = np.zeros(net_size, dtype=np.int16)
        counter = 0

        wlist = self.state_dict()['layer1.bias'].cpu().numpy().tolist()
        for w in wlist:
            net[counter] = round(w * QUANTIZATION_COEFF_L1)
            counter += 1
        for i in range(PADDING):
            net[counter] = 0
            counter += 1

        wlist = self.state_dict()['layer1.weight'].cpu().numpy().tolist()
        for ws in wlist:
            for w in ws:
                net[counter] = round(w * QUANTIZATION_COEFF_L1)
                counter += 1
            for i in range(PADDING):
                net[counter] = 0
                counter += 1

        wlist = self.state_dict()['layer2.bias'].cpu().numpy().tolist()
        for w in wlist:
            net[counter] = round(w * QUANTIZATION_COEFF_L1 * QUANTIZATION_COEFF_L2)
            counter += 1

        wlist = self.state_dict()['layer2.weight'].cpu().numpy().tolist()
        for ws in wlist:
            for w in ws:
                net[counter] = round(w * QUANTIZATION_COEFF_L2)
                counter += 1

        net.tofile(nn_name)

    def load_model(self, model_name, optimizer):
        print("\nLoading model from file: " + model_name)
        checkpoint = torch.load(model_name)
        initial_epoch = checkpoint['epoch'] + 1
        self.load_state_dict(checkpoint['model'])
        optimizer.load_state_dict(checkpoint['optimizer'])
        return initial_epoch


# def convert(old_net, new_net):
#     print("Convert!")
#     print("old L1 bias size:", old_net.layer1.bias.data.size())
#     print("new L1 bias size:", new_net.layer1.bias.data.size())
#     print("old L1 weights size:", old_net.layer1.weight.data.size())
#     print("new L1 weights size:", new_net.layer1.weight.data.size())
#     for i in range(HIDDEN_SIZE):
#         new_net.layer1.bias.data[i] = old_net.layer1.bias.data[i]
#         for j in range(L1_INPUT_SIZE):
#             new_net.layer1.weight.data[j, i] = old_net.layer1.weight.data[i, j]
#
#     print("old L2 bias size:", old_net.layer2.bias.data.size())
#     print("new L2 bias size:", new_net.layer2.bias.data.size())
#     print("old L2 weights size:", old_net.layer2.weight.data.size())
#     print("new L2 weights size:", new_net.layer2.weight.data.size())
#     for i in range(L2_OUTPUT_SIZE):
#         new_net.layer2.bias.data[i] = old_net.layer2.bias.data[0]
#         for j in range(L2_INPUT_SIZE):
#             new_net.layer2.weight.data[i, j] = old_net.layer2.weight.data[0, j]
#
#     return 0
