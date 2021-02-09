import metrics
import numpy as np
import matplotlib.pyplot as plt

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