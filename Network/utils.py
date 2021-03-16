import torch
import torch.nn.functional as F
import metrics
import dataset
import math
import cv2
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
            #print("Average", metric.name(), )
        plt.show()

def DefaultEvaluator():
    return Evaluator([metrics.PSNR(), metrics.SSIM()])

iteration_str = 'Current iteration: {0} \t Loss: {1:.6f} \t ETA: {2:.2f}s   '
finish_str = 'avg_loss: {0:.6f} \t min_loss {1:.6f} \t max_loss {2:.6f} \t total_time {3:.2f}s   '



def TrainEpoch(model, dataloader, loader_corruption, optimizer, loss_function):
    torch.seed() # To randomize
    model.train()
    dataloader_iter = iter(dataloader)
    num_iterations = len(dataloader)
    losses = np.zeros(num_iterations)
    start_time = time.time()
    for i, item in enumerate(dataloader_iter):
        #item.ToCuda()

        # Forward
        res = model.forward(item, loader_corruption=loader_corruption)
        target = item.target_images
        loss = loss_function(target, res)

        # Backward
        optimizer.zero_grad()
        loss.backward()

        #for name, param in model.named_parameters():
        #    print(name, param.grad.norm())

        optimizer.step()
        current_loss = loss.item()
        losses[i] = current_loss

        if(math.isnan(current_loss)):
            print(losses)
            print(item.video)
            print(item.frame)
            res = res[0][:,0:3,:,:]
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res[:,:,:])
            cv2.waitKey(0)
            cv2.destroyAllWindows()
            return

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
            target = item.target_images
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

def TestModel(model, dataloader):
    print("Testing model")
    model.eval()

    with torch.no_grad():
        dli = iter(dataloader)
        evaluator = DefaultEvaluator()
        evaluator.AddPlot("Model")
        for i, item in enumerate(dli):
            print(i, "/", len(dli), "  ", end="\r")
            item.ToCuda()
            res = model.forward(item)
            evaluator.Evaluate(res, item.target_images[0], "Model")
        evaluator.Plot()

def TestMasterModel(model, dataloader):
    print("Testing model")
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)

        psnr = metrics.PSNR()
        ssim = metrics.SSIM()
        psnr_list = np.zeros(len(dli))
        ssim_list = np.zeros(len(dli))
        time_list = np.zeros(len(dli))
        history = None
        for i, item in enumerate(dli):

            if(i % 60 == 0): # Clear history at the start of each video
                history = None

            # Prepare and time forward pass
            item.ToCuda()
            starter, ender = torch.cuda.Event(enable_timing=True), torch.cuda.Event(enable_timing=True)
            #with torch.cuda.amp.autocast():
            starter.record()
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0].unsqueeze(1), item.motion_vectors[0], item.jitters[0], history)
            ender.record()

            #Get timing info
            torch.cuda.synchronize()
            time_list[i] = starter.elapsed_time(ender)

            # Get performance measures
            psnr_list[i] = psnr(res, item.target_images[0])
            ssim_list[i] = ssim(res, item.target_images[0])

            # Print info
            test_str = '{0} / {1} \t PSNR: {2:.2f} \t SSIM: {3:.4f} \t Time: {4:.4}ms   '
            print(test_str.format(i, len(dli), psnr_list[i], ssim_list[i], time_list[i]), end="\r")

        # Print averages
        print("")
        print("PSNR average:", np.average(psnr_list))
        print("SSIM average:", np.average(ssim_list))
        print("Time average:", np.average(time_list))
        # Plot results
        plt.figure()
        plt.xlabel("Frame")
        plt.ylabel("PSNR")
        plt.plot(range(1, len(dli) + 1), psnr_list)
        plt.figure()
        plt.xlabel("Frame")
        plt.ylabel("SSIM")
        plt.plot(range(1, len(dli) + 1), ssim_list)
        plt.figure()
        plt.xlabel("Frame")
        plt.ylabel("Time")
        plt.plot(range(1, len(dli) + 1), time_list)
        plt.show()


def CheckMasterModelSampleEff(model, dataloader, loss_function):
    max_frames = 32
    model.eval()
    losses = np.zeros(max_frames)
    for num_frames in range(1, max_frames + 1):
        print("Validating using", num_frames, "frames")
        model.num_frames = num_frames
        losses[num_frames - 1] = ValidateModel(model, dataloader, loss_function)
        print("")
    plt.figure()
    plt.plot(range(1, max_frames + 1), losses)
    plt.show()

def VisualizeModel(model, dataloader):
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)
        for item in dli:
            item.ToCuda()
            res = model.forward(item)
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
        cv2.destroyAllWindows()

def VisualizeMasterModel(model, dataloader):
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)
        history = None
        i = 0
        for i, item in enumerate(dli):

            #if(i % 60 == 0): # Clear history at the start of each video
            #    history = None
            item.ToCuda()
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0].unsqueeze(1), item.motion_vectors[0], item.jitters[0], history)
            res = res[:,0:3,:,:]
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res[:,:,:])
            cv2.waitKey(0)
        cv2.destroyAllWindows()

def VisualizeDifference(model, dataloader):
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)
        for item in dli:
            item.ToCuda()
            res = model.forward(item)
            res = torch.abs(res - item.target_images[0])
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
        cv2.destroyAllWindows()

def PlotLosses(train_loss, val_loss):
    plt.figure()
    plt.plot(range(len(train_loss)), train_loss)
    plt.plot(range(len(val_loss)), val_loss)
    plt.grid()
    min_val = min(min(train_loss), min(val_loss))
    max_val = max(max(train_loss), max(val_loss))
    plt.yticks(np.arange(min_val,max_val, (max_val - min_val) * 0.05))
    plt.show()

def IllustrateJitterPattern(dataloader, num_jitters, factor):
    dli = iter(dataloader)
    jitters = np.zeros(shape=(num_jitters, 2))
    for i in range(num_jitters):
        item = next(dli)
        jitter = item.jitters[0]
        jitters[i,0] = jitter[0,0] * factor
        jitters[i,1] = jitter[0,1] * factor
    
    plt.figure()
    axes = plt.gca()
    axes.set_xlim([0, 4])
    axes.set_ylim([0, 4])
    axes.set_aspect('equal', adjustable='box')
    plt.xticks(np.arange(0,factor + 1, 1))
    plt.yticks(np.arange(0,factor + 1, 1))
    plt.grid()
    plt.scatter(jitters[:,0], jitters[:,1])
    plt.show()

def AddGradientHooks(model):
    for name, layer in model.named_children():
        layer.__name__ = name
        def func(m, g1, g2):
            for g in list(g1):
                print(m.__name__, g.norm())
        layer.register_backward_hook(func)
            