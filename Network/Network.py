import torch
from torch.autograd import Variable
from torch import optim 
import torch.nn.functional as F
import cv2
import numpy as np
import matplotlib.pyplot as plt
import dataset
import metrics
import models

videos = [i for i in range(10)]
ds = dataset.SSDataset(64, 1, videos, None)

## Motion vector gen svikter nar en vertex av trekanten er bak kamera

taa1 = models.TAA(0.25, True)
taa2 = models.TAA(0.60, True)
taa3 = models.TAA(0.25, False)
psnr_metric = metrics.PSNR()
ssim_metric = metrics.SSIM()
psnr_list1 = []
ssim_list1 = []
psnr_list2 = []
ssim_list2 = []
psnr_list3 = []
ssim_list3 = []
psnr_list4 = []
ssim_list4 = []

jitter_dist = []

iter = 120
for i in range(iter):
    print(i)
    item = ds[i]

    im64 = item.target_image.unsqueeze(0).cuda()
    im10 = item.input_image.unsqueeze(0).cuda()
    mv = item.motion_vector.unsqueeze(0).cuda()

    if(i % 60 == 0):
        taa1.reset_history()
        taa2.reset_history()
        taa3.reset_history()

    taa_res1 = taa1.forward(im10, mv)
    taa_res2 = taa2.forward(im10, mv)
    taa_res3 = taa3.forward(im10, mv)

    psnr_list1.append(psnr_metric.forward(im64, taa_res1).item())
    ssim_list1.append(ssim_metric.forward(im64, taa_res1).item())
    psnr_list2.append(psnr_metric.forward(im64, taa_res2).item())
    ssim_list2.append(ssim_metric.forward(im64, taa_res2).item())
    psnr_list3.append(psnr_metric.forward(im64, taa_res3).item())
    ssim_list3.append(ssim_metric.forward(im64, taa_res3).item())
    psnr_list4.append(psnr_metric.forward(im64, im10).item())
    ssim_list4.append(ssim_metric.forward(im64, im10).item())

    jd = item.jitter - np.ones(2) * 0.5
    jitter_dist.append(np.abs(jd[0]) + np.abs(jd[1]))

    #print(i, item.jitter, psnr_list4[i])
    diff = torch.abs(taa_res1 - im64)

    taa_res = diff.squeeze().cpu().detach()
    taa_res_np = dataset.ImageTorchToNumpy(taa_res)

    window_name = "Image"
    cv2.imshow(window_name, taa_res_np)
    cv2.waitKey(0)
cv2.destroyAllWindows()


plt.figure()
plt.plot(range(iter), psnr_list1, label=str(taa1.alpha) + " clamp")
plt.plot(range(iter), psnr_list2, label=str(taa2.alpha) + " clamp")
plt.plot(range(iter), psnr_list3, label=str(taa3.alpha) + "")
plt.plot(range(iter), psnr_list4, label="1.0")
plt.legend(loc="upper right")
plt.figure()
plt.plot(range(iter), ssim_list1, label=str(taa1.alpha) + " clamp")
plt.plot(range(iter), ssim_list2, label=str(taa2.alpha) + " clamp")
plt.plot(range(iter), ssim_list3, label=str(taa3.alpha) + "")
plt.plot(range(iter), ssim_list4, label="1.0")
plt.legend(loc="upper right")

plt.figure()
plt.plot(jitter_dist, ssim_list4, 'o')
plt.show()
    #upsampled = upsampled.squeeze()
    #upsampled_np = dataset.ImageTorchToNumpy(upsampled)

    #window_name = "Image"
    #cv2.imshow(window_name, upsampled_np)
    #cv2.waitKey(0)
    #cv2.destroyAllWindows()