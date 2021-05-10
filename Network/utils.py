import torch
import torch.nn.functional as F
import metrics
import models
import dataset
import math
import cv2
import numpy as np
import matplotlib.pyplot as plt
import time
import skvideo.measure

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

def TrainEpoch(model, dataloader, optimizer, loss_function):
    torch.seed() # To randomize
    model.train()
    dataloader_iter = iter(dataloader)
    num_iterations = len(dataloader)
    losses = np.zeros(num_iterations)
    start_time = time.time()
    for i, item in enumerate(dataloader_iter):
        #item.ToCuda()

        # Forward
        res = model.forward(item)
        target = item.target_images
        loss = loss_function(target, res)

        
        # Backward
        optimizer.zero_grad()
        for j in range(len(loss)-1):
            loss[j].backward(retain_graph=True)
        loss[-1].backward(retain_graph=False)
        #for name, param in model.named_parameters():
        #    print(name, param.grad.norm())

        optimizer.step()
        current_loss = torch.mean(loss).item()
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

val_iteration_str = 'Current iteration: {0} \t Loss: {1:.6f} \t ETA: {2:.2f}s   '
val_iteration_str2 = 'Current iteration: {0} \t PSNR: {1:.6f} \t SSIM: {2:.6f} \t ETA: {3:.2f}s   '
val_finish_str = 'avg_loss: {0:.6f} \t avg_PSNR {1:.6f} \t avg_SSIM {2:.6f} \t total_time {3:.2f}s   '

def ValidateModel(model, dataloader1, dataloader2, loss_function):
    torch.manual_seed(17) # To get consistent results
    model.eval()
    
    with torch.no_grad():
        dataloader_iter1 = iter(dataloader1)
        dataloader_iter2 = iter(dataloader2)
        num_iterations = len(dataloader1)
        num_iterations2 = len(dataloader2)
        losses = np.zeros(num_iterations)
        psnrs = np.zeros(num_iterations2)
        ssims = np.zeros(num_iterations2)
        psnr = metrics.PSNR()
        ssim = metrics.SSIM()
        start_time = time.time()
        for i, item in enumerate(dataloader_iter1):
            # Forward
            res = model.forward(item)
            target = item.target_images
            loss = torch.mean(loss_function(target, res))
            current_loss = loss.item()
            losses[i] = current_loss

            # Print info
            current_time = time.time()
            avg_time = (current_time - start_time) / (i + 1)
            remaining_time = avg_time * (num_iterations - (i + 1))
            print(val_iteration_str.format(i, current_loss, remaining_time), end='\r')

        start_time2 = time.time()
        history = None
        for i, item in enumerate(dataloader_iter2):
            if(i % 60 == 0):
                history = None
            item.ToCuda()
            # Forward
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0], item.motion_vectors[0], item.jitters[0], history)
            psnrs[i] = psnr(item.target_images[0], res).item()
            ssims[i] = ssim(item.target_images[0], res).item()

            # Print info
            current_time = time.time()
            avg_time = (current_time - start_time2) / (i + 1)
            remaining_time = avg_time * (num_iterations2 - (i + 1))
            print(val_iteration_str2.format(i, psnrs[i], ssims[i], remaining_time), end='\r')

        # Print validation summary
        avg_loss = np.average(losses)
        avg_psnr = np.average(psnrs)
        avg_ssim = np.average(ssims)
        print('Validation finished \t ' + val_finish_str.format(avg_loss, avg_psnr, avg_ssim, time.time()-start_time))
    return avg_loss, avg_psnr, avg_ssim

def GetZeroItem(num_frames, factor):
    W, H = 1920 // factor, 1080 // factor
    item = dataset.SSDatasetItem(-1, -1, -1, -1, -1, -1)
    item.input_images = [torch.zeros(size=(1,3,H,W)) for i in range(num_frames)]
    item.motion_vectors = [torch.zeros(size=(1,H,W,2)) for i in range(num_frames)]
    item.depth_buffers = [torch.zeros(size=(1, 1,H,W)) for i in range(num_frames)]
    item.jitters = [torch.zeros(size=(1,2)) for i in range(num_frames)]
    return item

def ValidateFBModel(model, dataloader1, dataloader2, loss_function):
    torch.manual_seed(17) # To get consistent results
    model.eval()
    
    with torch.no_grad():
        dataloader_iter1 = iter(dataloader1)
        dataloader_iter2 = iter(dataloader2)
        num_iterations = len(dataloader1)
        num_iterations2 = len(dataloader2)
        losses = np.zeros(num_iterations)
        psnrs = np.zeros(num_iterations2)
        ssims = np.zeros(num_iterations2)
        psnr = metrics.PSNR()
        ssim = metrics.SSIM()
        start_time = time.time()
        for i, item in enumerate(dataloader_iter1):
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
            print(val_iteration_str.format(i, current_loss, remaining_time), end='\r')

        start_time2 = time.time()
        item = None
        for i, x in enumerate(dataloader_iter2):
            if(i % 60 == 0): # Clear history at the start of each video
                item = GetZeroItem(5, model.factor)
            # Forward
            item.input_images = item.input_images[1:]
            item.motion_vectors = item.motion_vectors[1:]
            item.depth_buffers = item.depth_buffers[1:]
            item.jitters = item.jitters[1:]
            item.input_images.append(x.input_images[0])
            item.motion_vectors.append(x.motion_vectors[0])
            item.depth_buffers.append(x.depth_buffers[0])
            item.jitters.append(x.jitters[0])
            res = model.forward(item)

            cuda_target = x.target_images[0].cuda()
            psnrs[i] = psnr(cuda_target, res).item()
            ssims[i] = ssim(cuda_target, res).item()

            # Print info
            current_time = time.time()
            avg_time = (current_time - start_time2) / (i + 1)
            remaining_time = avg_time * (num_iterations2 - (i + 1))
            print(val_iteration_str2.format(i, psnrs[i], ssims[i], remaining_time), end='\r')


        # Print validation summary
        avg_loss = np.average(losses)
        avg_psnr = np.average(psnrs)
        avg_ssim = np.average(ssims)
        print('Validation finished \t ' + val_finish_str.format(avg_loss, avg_psnr, avg_ssim, time.time()-start_time))
    return avg_loss, avg_psnr, avg_ssim

def TestFBModel(model, dataloader):
    print("Testing model")
    model.eval()

    with torch.no_grad():
        dli = iter(dataloader)
        item = None

        psnr = metrics.PSNR()
        ssim = metrics.SSIM()
        psnr_list = np.zeros(len(dli))
        ssim_list = np.zeros(len(dli))
        for i, x in enumerate(dli):
            if(i % 60 == 0): # Clear history at the start of each video
                item = GetZeroItem(5, model.factor)
            item.input_images = item.input_images[1:]
            item.motion_vectors = item.motion_vectors[1:]
            item.depth_buffers = item.depth_buffers[1:]
            item.jitters = item.jitters[1:]
            item.input_images.append(x.input_images[0])
            item.motion_vectors.append(x.motion_vectors[0])
            item.depth_buffers.append(x.depth_buffers[0])
            item.jitters.append(x.jitters[0])
            res = model.forward(item)
            
            cuda_target = x.target_images[0].cuda()
            psnr_list[i] = psnr(res, cuda_target)
            ssim_list[i] = ssim(res, cuda_target)
            # Print info
            test_str = '{0} / {1} \t PSNR: {2:.2f} \t SSIM: {3:.4f} \t '
            print(test_str.format(i, len(dli), psnr_list[i], ssim_list[i]), end="\r")
        
        np.save("ModelPSNR", psnr_list)
        np.save("ModelSSIM", ssim_list)
        # Print averages
        print("")
        print("PSNR average:", np.average(psnr_list))
        print("SSIM average:", np.average(ssim_list))
        # Plot results
        plt.figure()
        plt.xlabel("Frame")
        plt.ylabel("PSNR")
        plt.plot(range(1, len(dli) + 1), psnr_list)
        plt.figure()
        plt.xlabel("Frame")
        plt.ylabel("SSIM")
        plt.plot(range(1, len(dli) + 1), ssim_list)
        plt.show()

def toGrayScale(img):
    return 0.299*img[:,0,:,:] + 0.587*img[:,1,:,:] + 0.114*img[:,2,:,:]

def subTestMasterModel(model, dataloader):
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)

        psnr = metrics.PSNR()
        ssim = metrics.SSIM()
        psnr_list = np.zeros(len(dli))
        ssim_list = np.zeros(len(dli))
        time_list = np.zeros(len(dli))
        history = None

        #strred_dist = np.zeros(shape=(600, 1080, 1920))
        #strred_target = np.zeros(shape=(600, 1080, 1920))
        #tpsnr = metrics.TPSNR()
        #tpsnr_list = np.zeros(len(dli))
        #prev_res = None
        #prev_target = None

        for i, item in enumerate(dli):

            if(i % 60 == 0): # Clear history at the start of each video
                history = None

            # Prepare and time forward pass
            item.ToCuda()
            starter, ender = torch.cuda.Event(enable_timing=True), torch.cuda.Event(enable_timing=True)
            #with torch.cuda.amp.autocast():
            starter.record()
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0], item.motion_vectors[0], item.jitters[0], history)
            ender.record()

            #Get timing info
            torch.cuda.synchronize()
            time_list[i] = starter.elapsed_time(ender)

            #strred_dist[i,:,:] = toGrayScale(res.detach().cpu()).numpy()
            #strred_target[i,:,:] = toGrayScale(item.target_images[0].detach().cpu()).numpy()
            #if(i != 0):
            #    tpsnr_list[i] = tpsnr(res, prev_res, item.target_images[0], prev_target)
            #prev_res = res
            #prev_target = item.target_images[0]

            # Get performance measures
            psnr_list[i] = psnr(res, item.target_images[0])
            ssim_list[i] = ssim(res, item.target_images[0])

            # Print info
            #test_str = '{0} / {1} \t PSNR: {2:.2f} \t TPSNR: {5:.2f} \t SSIM: {3:.4f} \t Time: {4:.4}ms   '
            test_str = '{0} / {1} \t PSNR: {2:.2f} \t SSIM: {3:.4f} \t Time: {4:.4}ms   '
            print(test_str.format(i, len(dli), psnr_list[i], ssim_list[i], time_list[i]), end="\r")
    #_, strred, _ = skvideo.measure.strred(strred_target, strred_dist)
    #print(strred)
    np.save("ModelPSNR", psnr_list)
    np.save("ModelSSIM", ssim_list)
    return psnr_list, ssim_list, time_list
def TestMasterModel(model, dataloader):
    print("Testing model")
    
    psnr_list, ssim_list, time_list = subTestMasterModel(model, dataloader)

    # Print averages
    print("")
    print("PSNR average:", np.average(psnr_list))
    #print("TPSNR average:", np.average(tpsnr_list[1:]))
    print("SSIM average:", np.average(ssim_list))
    print("Time average:", np.average(time_list))
    # Plot results
    plt.figure()
    plt.xlabel("Frame")
    plt.ylabel("PSNR")
    plt.plot(range(1, len(psnr_list) + 1), psnr_list)
    plt.figure()
    plt.xlabel("Frame")
    plt.ylabel("SSIM")
    plt.plot(range(1, len(ssim_list) + 1), ssim_list)
    plt.figure()
    plt.xlabel("Frame")
    plt.ylabel("Time")
    plt.plot(range(1, len(time_list) + 1), time_list)
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

def VisualizeFBModel(model, dataloader):
    model.eval()
    with torch.no_grad():
        dli = iter(dataloader)
        item = None
        for i, x in enumerate(dli):
            if(i % 60 == 0): # Clear history at the start of each video
                item = GetZeroItem(5, 4)
            # Forward
            item.input_images.pop()
            item.motion_vectors.pop()
            item.depth_buffers.pop()
            item.jitters.pop()
            item.input_images.insert(0,x.input_images[0])
            item.motion_vectors.insert(0,x.motion_vectors[0])
            item.depth_buffers.insert(0,x.depth_buffers[0])
            item.jitters.insert(0,x.jitters[0])
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

            if(i % 60 == 0): # Clear history at the start of each video
                history = None
            item.ToCuda()
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0], item.motion_vectors[0], item.jitters[0], history)
            res = res
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
        history1 = None
        history2 = None
        i = 0
        for i, item in enumerate(dli):
            if(i % 60 == 0): # Clear history at the start of each video
                history1 = None
                history2 = None
            item.ToCuda()
            res1, history1 = model.sub_forward(item.input_images[0].clone(), item.depth_buffers[0].unsqueeze(1).clone(), item.motion_vectors[0].clone(), item.jitters[0].clone(), history1, False)
            res2, history2 = model.sub_forward(item.input_images[0], item.depth_buffers[0].unsqueeze(1), item.motion_vectors[0], item.jitters[0], history2, True)
            diff1 = torch.mean(torch.abs(res1[:,0:3,:,:] - item.target_images[0]),dim=1)
            diff2 = torch.mean(torch.abs(res2[:,0:3,:,:] - item.target_images[0]),dim=1)
            mins = torch.argmin(torch.stack((diff1, diff2)), dim=0).float()
            
            res = torch.abs(res1)
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
            res = torch.abs(res2)
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
            res = item.target_images[0]
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
        cv2.destroyAllWindows()

def PlotLosses(train_loss, val_epochs, val_loss, val_psnr, val_ssim):
    plt.figure()
    plt.plot(range(len(train_loss)), train_loss)
    plt.plot(val_epochs, val_loss)
    plt.grid()
    min_val = min(min(train_loss), min(val_loss))
    max_val = max(max(train_loss), max(val_loss))
    plt.yticks(np.arange(min_val,max_val, (max_val - min_val) * 0.05))
    plt.figure()
    plt.plot(val_epochs, val_psnr)
    plt.figure()
    plt.plot(val_epochs, val_ssim)
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
    axes.set_xlim([0, factor])
    axes.set_ylim([0, factor])
    axes.set_aspect('equal', adjustable='box')
    plt.xticks(np.arange(0,factor + 1, 1))
    plt.yticks(np.arange(0,factor + 1, 1))
    plt.grid()
    plt.scatter(jitters[:,0], jitters[:,1])
    for i in range(num_jitters):
        plt.annotate(str(i), (jitters[i,0] + 0.05, jitters[i,1] + 0.05))
    plt.show()

def AddGradientHooks(model):
    for name, layer in model.named_children():
        layer.__name__ = name
        def func(m, g1, g2):
            for g in list(g1):
                print(m.__name__, g.norm())
        layer.register_backward_hook(func)
            
def SaveCroppedImages(img):
    crops = [(0,0), (100,200)]
    crop_size = 64
    for i, (cx, cy) in enumerate(crops):
        cimg = img[:,:,cy:cy+crop_size,cx:cx+crop_size]
        cimg = cimg
        cimg = dataset.ImageTorchToNumpy(cimg.squeeze().cpu().detach())
        cv2.imwrite("crop_img" + str(i) + ".png", cimg)

        # Red rectangle
        img[:,0,cy:cy+crop_size,cx] = 1
        img[:,1,cy:cy+crop_size,cx] = 0
        img[:,2,cy:cy+crop_size,cx] = 0
        img[:,0,cy:cy+crop_size,cx+crop_size] = 1
        img[:,1,cy:cy+crop_size,cx+crop_size] = 0
        img[:,2,cy:cy+crop_size,cx+crop_size] = 0
        img[:,0,cy,cx:cx+crop_size] = 1
        img[:,1,cy,cx:cx+crop_size] = 0
        img[:,2,cy,cx:cx+crop_size] = 0
        img[:,0,cy+crop_size,cx:cx+crop_size] = 1
        img[:,1,cy+crop_size,cx:cx+crop_size] = 0
        img[:,2,cy+crop_size,cx:cx+crop_size] = 0
    img = dataset.ImageTorchToNumpy(img.squeeze().cpu().detach())
    cv2.imwrite("crop_img_outlines.png", img)

def SaveTestImage(model, loader, image_nr, model_name, epoch):
    with torch.no_grad(): 
        dli = iter(loader)
        history = None
        res = None
        for i in range(image_nr):
            item = next(dli)
            item.ToCuda()
            res, history = model.sub_forward(item.input_images[0], item.depth_buffers[0], item.motion_vectors[0], item.jitters[0], history)
        res = res.squeeze().cpu().detach()
        res = dataset.ImageTorchToNumpy(res)
        cv2.imwrite(model_name + "/temp_img" + str(epoch) + ".png", res)

def SaveTestImageFB(model, loader, image_nr, model_name, epoch):
    with torch.no_grad(): 
        dli = iter(loader)
        item = GetZeroItem(5,model.factor)
        res = None
        for i in range(image_nr):
            x = next(dli)
            item.input_images = item.input_images[1:]
            item.motion_vectors = item.motion_vectors[1:]
            item.depth_buffers = item.depth_buffers[1:]
            item.jitters = item.jitters[1:]
            item.input_images.append(x.input_images[0])
            item.motion_vectors.append(x.motion_vectors[0])
            item.depth_buffers.append(x.depth_buffers[0])
            item.jitters.append(x.jitters[0])
            if(i == image_nr - 1):
                res = model.forward(item)
        res = res.squeeze().cpu().detach()
        res = dataset.ImageTorchToNumpy(res)
        cv2.imwrite(model_name + "/temp_img" + str(epoch) + ".png", res) 

import struct
def SaveModelWeights(model):
    f = open("nn_weights.bin", "wb")
    print("Saving model weights")
    num_named = 0
    for name, param in model.named_parameters():
        num_named += 1
    print("Number of named parameters:", num_named)
    f.write(num_named.to_bytes(4, byteorder='little', signed=False))
    for name, param in model.named_parameters():
        # Write name
        name_len = len(name)
        print(name_len, name)
        f.write(name_len.to_bytes(4, byteorder='little', signed=False))
        f.write(name.encode('ascii'))

        # Write parameters
        print(param.data)
        param_list = param.data.flatten().tolist()
        param_float_count = len(param_list)
        f.write(param_float_count.to_bytes(4, byteorder='little', signed=False))
        f.write(struct.pack('f'*len(param_list), *param_list))
    f.close()

def TestMultiple(dataloader):
    file_list = []
    
    # 4x4
    file_list.append(("JAU4x4", "JAU"))
    file_list.append(("TUS1004x4", "TUS"))
    file_list.append(("XNet4x4", "Xiao et. al."))
    file_list.append(("MasterNet4x4", "DLTUS(4, 1)"))

    # 2x2
    #file_list.append(("JAU2x2", "JAU"))
    #file_list.append(("TUS502x2", "TUS"))
    #file_list.append(("XNet2x2", "Xiao et. al."))
    #file_list.append(("MasterNet2x2", "DLTUS(2, 1)"))

    # Biased 2x2
    #file_list.append(("JAU2x2Biased", "JAU"))
    #file_list.append(("TUS502x2Biased", "TUS"))
    #file_list.append(("XNet2x2Biased", "Xiao et. al."))
    #file_list.append(("MasterNet2x2Biased", "DLTUS(2, 2)"))

    plt.figure()
    for file, name in file_list:
        psnr_list = np.load("Results/" + file + "PSNR.npy")
        plt.xlabel("Frame")
        plt.ylabel("PSNR")
        plt.plot(range(1, len(psnr_list) + 1), psnr_list, label=name)
        print(file, "average PSNR:", np.average(psnr_list))

    plt.grid(axis="y")
    #plt.title("PSNR on Dataset 1 with 2x2 Upsampling")
    plt.title("PSNR on Dataset 1 with 4x4 Upsampling")
    plt.legend()

    plt.figure()
    for file, name in file_list:
        ssim_list = np.load("Results/" + file + "SSIM.npy")
        plt.xlabel("Frame")
        plt.ylabel("SSIM")
        plt.plot(range(1, len(ssim_list) + 1), ssim_list, label=name)
        print(file, "average SSIM:", np.average(ssim_list))

    plt.grid(axis="y")
    #plt.title("SSIM on Dataset 1 with 2x2 Upsampling")
    plt.title("SSIM on Dataset 1 with 4x4 Upsampling")
    plt.legend()
    plt.show()
