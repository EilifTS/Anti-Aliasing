import torch
import metrics
import numpy as np
import matplotlib.pyplot as plt
import time

class Evaluator():
    def __init__(self, metrics):
        self.metrics = metrics
        self.lines = {}

    def AddPlot(self, plot_name):
        self.lines[plot_name] = [[] for i in range(len(self.metrics))]

    def Evaluate(self, i1, i2, plot_name):
        for i, metric in enumerate(self.metrics):
            self.lines[plot_name][i].append(metric.forward(i1, i2).item())
    
    def Plot(self):
        print(self.lines)
        for i, metric in enumerate(self.metrics):
            plt.figure()
            for k, v in self.lines.items():
                line = v[i]
                plt.plot(range(len(line)), line, label=k)
            plt.xlabel("Frame")
            ylabel = metric.name()
            if(metric.unit() != ""):
                ylabel += '({})'.format(metric.unit())
            plt.ylabel(ylabel)
            plt.legend(loc="upper left")
        plt.show()

def DefaultEvaulator():
    return Evaluator([metrics.PSNR(), metrics.SSIM()])

iteration_str = 'Current iteration: {0} \t Loss: {1:.6f} \t ETA: {2:.2f}s   '
finish_str = 'avg_loss: {0:.4f} \t min_loss {1:.4f} \t max_loss {2:.4f} \t total_time {3:.2f}s   '

def TrainEpoch(model, dataloader, optimizer, loss_function):
    torch.seed() # To randomize
    model.train()
    dataloader_iter = iter(dataloader)
    num_iterations = len(dataloader)
    losses = np.zeros(num_iterations)
    start_time = time.time()

    for i, item in enumerate(dataloader_iter):
        item.ToCuda()

        # Forward
        res = model.forward(item)
        target = item.target_images[0]
        loss = loss_function(target, res)

        # Backward
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        current_loss = loss.item()
        losses[i] = current_loss

        # Print info
        current_time = time.time()
        avg_time = (current_time - start_time) / (i + 1)
        remaining_time = avg_time * (num_iterations - (i + 1))
        print(iteration_str.format(i, current_loss, remaining_time), end='\r')

    # Print epoch summary
    avg_loss = np.average(losses)
    min_loss = np.min(losses)
    max_loss = np.max(losses)
    print('Training finished \t ' + finish_str.format(avg_loss, min_loss, max_loss, time.time()-start_time))
    return avg_loss

def ValidateModel(model, dataloader, loss_function):
    torch.manual_seed(17) # To get consistent results
    model.eval()
    
    with torch.no_grad():
        dataloader_iter = iter(dataloader)
        num_iterations = len(dataloader)
        losses = np.zeros(num_iterations)
        start_time = time.time()
        for i, item in enumerate(dataloader_iter):
            item.ToCuda()

            # Forward
            res = model.forward(item)
            target = item.target_images[0]
            loss = loss_function(target, res)
            current_loss = loss.item()
            losses[i] = current_loss

            # Print info
            current_time = time.time()
            avg_time = (current_time - start_time) / (i + 1)
            remaining_time = avg_time * (num_iterations - (i + 1))
            print(iteration_str.format(i, current_loss, remaining_time), end='\r')

        # Print validation summary
        avg_loss = np.average(losses)
        min_loss = np.min(losses)
        max_loss = np.max(losses)
        print('Validation finished \t ' + finish_str.format(avg_loss, min_loss, max_loss, time.time()-start_time))
    return avg_loss