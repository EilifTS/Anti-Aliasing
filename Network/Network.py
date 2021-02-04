import torch
from torch.autograd import Variable
from torch import optim 
import torch.nn.functional as F
import metrics
import cv2
import numpy as np
import dataset
import matplotlib.pyplot as plt

ds = dataset.SSDataset(32, 4, [0, 1], None)

psnr_metric = metrics.PSNR()
ssim_metric = metrics.SSIM()

psnr_list = []
ssim_list = []

for i in range(120):
    item = ds[i]

    in_im = item.input_image.unsqueeze(0).cuda()
    ta_im = item.target_image.unsqueeze(0).cuda()
    upsampled = F.interpolate(in_im, scale_factor=4, mode='bilinear')

    psnr = psnr_metric.forward(upsampled, ta_im)
    ssim = ssim_metric.forward(upsampled, ta_im)

    psnr_list.append(psnr.item())
    ssim_list.append(ssim.item())
    print(i, psnr.item(), ssim.item())

plt.figure()
plt.plot(range(120), psnr_list)
plt.figure()
plt.plot(range(120), ssim_list)
plt.show()


    #upsampled = upsampled.squeeze()
    #upsampled_np = dataset.ImageTorchToNumpy(upsampled)

    #window_name = "Image"
    #cv2.imshow(window_name, upsampled_np)
    #cv2.waitKey(0)
    #cv2.destroyAllWindows()