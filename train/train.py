import torch
import torch.nn as nn
from utils import current_time
from tqdm import tqdm


def train(model, optimizer, train_loader, epoch, total_epochs, batches):
    batch_idx = 0
    loss_sum = 0

    mse_loss = nn.MSELoss()

    model.train()

    pbar = tqdm(total=batches, unit='btch', miniters=50, desc='Loss: {:.8f}'.format(0))
    for ([in1, in2, val, stg], out) in train_loader:
        optimizer.zero_grad()

        output = model(in1, in2, val, stg)
        loss = mse_loss(output[:, 0], out)
        loss.backward()

        loss_sum += loss.item()
        optimizer.step()

        pbar.update()
        batch_idx += 1
        if batch_idx % 100 == 0:
            pbar.set_description_str('Loss: {:.8f}'.format(loss_sum / batch_idx))

    pbar.close()

    return loss_sum / batches


def test(model, test_loader, batches):
    print("")
    model.eval()
    mse_loss = nn.MSELoss()
    pbar = tqdm(total=batches, unit='btch', miniters=50)
    test_loss = 0
    with torch.no_grad():
        for ([in1, in2, val, stg], out) in test_loader:
            output = model(in1, in2, val, stg)
            test_loss += mse_loss(output[:, 0], out)
            pbar.update()
    pbar.close()

    test_loss /= batches

    print('Test set: Average loss: {:.8f}'.format(test_loss))

    return test_loss


def set_lr(optim, new_lr):
    for g in optim.param_groups:
        g['lr'] = new_lr
