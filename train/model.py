import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
from feature_transformer import DoubleFeatureTransformerSlice


USE_PSQT = True
USE_S = True
USE_L3 = True
USE_L4 = True

PSQT_COEFF = 512

L1_CLAMP = 5

K_SIZE = 16
P_SIZE = 768
if USE_S:
    S_SIZE = 6
else:
    S_SIZE = 1

if USE_PSQT:
    PADDING = 32 - S_SIZE   # 32 - size of avx512 registry
else:
    PADDING = 0

HIDDEN_SIZE = 1280
HIDDEN2_SIZE = 6
HIDDEN3_SIZE = 32

L1_INPUT_SIZE = K_SIZE * P_SIZE
if USE_PSQT:
    L1_OUTPUT_SIZE = HIDDEN_SIZE + S_SIZE
    L1_OUTPUT_SIZE_P = L1_OUTPUT_SIZE + PADDING
else:
    L1_OUTPUT_SIZE = HIDDEN_SIZE
    L1_OUTPUT_SIZE_P = L1_OUTPUT_SIZE

L2_INPUT_SIZE = HIDDEN_SIZE

if USE_L3:
    L2_OUTPUT_SIZE = HIDDEN2_SIZE * S_SIZE
    L3_INPUT_SIZE = HIDDEN2_SIZE
    if USE_L4:
        L3_OUTPUT_SIZE = HIDDEN3_SIZE * S_SIZE
        L4_INPUT_SIZE = HIDDEN3_SIZE
        L4_OUTPUT_SIZE = S_SIZE
    else:
        L3_OUTPUT_SIZE = S_SIZE
else:
    L2_OUTPUT_SIZE = S_SIZE


class Net(nn.Module):
    def __init__(self, eval_divider):
        super(Net, self).__init__()
        self.layer1 = DoubleFeatureTransformerSlice(L1_INPUT_SIZE, L1_OUTPUT_SIZE)
        self.layer2 = nn.Linear(L2_INPUT_SIZE, L2_OUTPUT_SIZE)
        if USE_L3:
            self.layer3 = nn.Linear(L3_INPUT_SIZE, L3_OUTPUT_SIZE)
        if USE_L4:
            self.layer4 = nn.Linear(L4_INPUT_SIZE, L4_OUTPUT_SIZE)

        if not USE_PSQT:
            return

        self.psqt_coeff = eval_divider / PSQT_COEFF

        pieces = [110.0, 370.0, 364.0, 589.0, 1138.0, -110.0, -370.0, -364.0, -589.0, -1138.0, 0.0, 0.0]

        for i in range(S_SIZE):
            self.layer1.bias.data[HIDDEN_SIZE + i] = 0.0
            for k in range(K_SIZE):
                for p in range(12):
                    for sq in range(64):
                        self.layer1.weight.data[k * P_SIZE + p * 64 + sq, HIDDEN_SIZE + i] = pieces[p] / PSQT_COEFF

    def forward(self, x, y, v, s):
        x, y = self.layer1(x, v, y, v)

        s_f = s.flatten() + torch.arange(0,S_SIZE*s.shape[0],S_SIZE, device=next(self.parameters()).device)

        if USE_PSQT:
            x1, x2 = torch.split(x, x.shape[1]-S_SIZE, dim=1)
            y1, y2 = torch.split(y, y.shape[1]-S_SIZE, dim=1)
            c = torch.cat((x1, y1), dim=1)
        else:
            c = torch.cat((x, y), dim=1)

        c = torch.clamp(c, 0.0, 1.0)

        c_spl = torch.split(c, HIDDEN_SIZE // 2, dim=1)
        c = torch.cat((c_spl[0] * c_spl[1], c_spl[2] * c_spl[3]), dim=1)

        c = self.layer2(c)

        if USE_L3:
            c = c.reshape((-1, S_SIZE, HIDDEN2_SIZE)).view(-1, HIDDEN2_SIZE)[s_f]
            c = torch.clamp(c, 0.0, 1.0)
            c = torch.pow(c, 2)
            c = self.layer3(c)
        if USE_L4:
            c = c.reshape((-1, S_SIZE, HIDDEN3_SIZE)).view(-1, HIDDEN3_SIZE)[s_f]
            c = torch.clamp(c, 0.0, 1.0)
            c = self.layer4(c)

        if USE_PSQT:
            c = c + (x2 - y2) * (0.5 / self.psqt_coeff)

        if USE_S:
            c = c.gather(1, s)

        return torch.sigmoid(c)

    def clamp_l1(self):
        self.layer1.weight.data.clamp_(-L1_CLAMP, L1_CLAMP)

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
        if USE_L3:
            net_size = net_size + L3_OUTPUT_SIZE + L3_INPUT_SIZE * L3_OUTPUT_SIZE
        if USE_L4:
            net_size = net_size + L4_OUTPUT_SIZE + L4_INPUT_SIZE * L4_OUTPUT_SIZE

        counter = 0

        net = np.zeros(net_size, dtype=np.float32)

        wlist = self.state_dict()['layer1.bias'].cpu().numpy().tolist()
        for w in wlist:
            net[counter] = w
            counter += 1
        for i in range(PADDING):
            net[counter] = 0
            counter += 1

        wlist = self.state_dict()['layer1.weight'].cpu().numpy().tolist()
        for ws in wlist:
            for w in ws:
                net[counter] = w
                counter += 1
            for i in range(PADDING):
                net[counter] = 0
                counter += 1

        wlist = self.state_dict()['layer2.bias'].cpu().numpy().tolist()
        for w in wlist:
            net[counter] = w
            counter += 1

        wlist = self.state_dict()['layer2.weight'].cpu().numpy().tolist()
        for ws in wlist:
            for w in ws:
                net[counter] = w
                counter += 1

        if USE_L3:
            wlist = self.state_dict()['layer3.bias'].cpu().numpy().tolist()
            for w in wlist:
                net[counter] = w
                counter += 1

            wlist = self.state_dict()['layer3.weight'].cpu().numpy().tolist()
            for ws in wlist:
                for w in ws:
                    net[counter] = w
                    counter += 1

        if USE_L4:
            wlist = self.state_dict()['layer4.bias'].cpu().numpy().tolist()
            for w in wlist:
                net[counter] = w
                counter += 1

            wlist = self.state_dict()['layer4.weight'].cpu().numpy().tolist()
            for ws in wlist:
                for w in ws:
                    net[counter] = w
                    counter += 1

        # print("Save! net_size: {}  counter: {}".format(net_size, counter))
        net.tofile(nn_name)

    def load_model(self, model_name, optimizer):
        print("\nLoading model from file: " + model_name)
        checkpoint = torch.load(model_name)
        initial_epoch = checkpoint['epoch'] + 1
        self.load_state_dict(checkpoint['model'])
        optimizer.load_state_dict(checkpoint['optimizer'])
        return initial_epoch
