import torch
import numpy as np
import cv2
import dataset
import models
import utils

videos = [i for i in range(10)]
ds = dataset.SSDataset(64, 1, videos)#, transform=dataset.RandomCrop(256))

taa1 = models.TAA(0.15, True, True)
taa2 = models.TAA(0.25, True, True)
taa3 = models.TAA(0.05, True, True)

evaluator = utils.DefaultEvaulator()
evaluator.AddPlot(taa1.name())
evaluator.AddPlot(taa2.name())
evaluator.AddPlot(taa3.name())
evaluator.AddPlot('ss1')
evaluator.AddPlot('ss16')

jitter_dist = []

iter = 60
for i in range(0,0+iter):
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
    ss16_img = dataset.ImageNumpyToTorch(dataset.LoadTargetImage(16, item.video, item.frame)).unsqueeze(0).cuda()

    evaluator.Evaluate(im64, taa_res1, taa1.name())
    evaluator.Evaluate(im64, taa_res2, taa2.name())
    evaluator.Evaluate(im64, taa_res3, taa3.name())
    evaluator.Evaluate(im64, im10, 'ss1')
    #evaluator.Evaluate(im64, ss16_img, 'ss16')

    #print(i, item.jitter, psnr_list4[i])
    diff = torch.abs(taa_res1 - im64)

    taa_res = taa_res1.squeeze().cpu().detach()
    taa_res_np = dataset.ImageTorchToNumpy(taa_res)

    #window_name = "Image"
    #cv2.imshow(window_name, taa_res_np)
    #cv2.waitKey(0)
#cv2.destroyAllWindows()

evaluator.Plot()
