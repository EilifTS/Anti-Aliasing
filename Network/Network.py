import pytorch_ssim
import torch
from torch.autograd import Variable
from torch import optim 
import torch.nn.functional as F
from vgg_perceptual_loss import VGGPerceptualLoss
import cv2
import numpy as np

npImg1 = cv2.imread("einstein.png")

img1 = torch.from_numpy(np.rollaxis(npImg1, 2)).float().unsqueeze(0)/255.0

ZUfilter = F.pad(torch.ones(3,1,1,1),(1,1,1,1))
ZUfilter[1,0,1,1] = 0
ZUfilter[2,0,1,1] = 0
img2LR = F.avg_pool2d(img1, (4,4))
img2ZP = F.conv_transpose2d(img2LR, ZUfilter, stride=4, padding=1)
img2NN = F.interpolate(img2LR, mode='nearest', size=(256, 256))
img2BL = F.interpolate(img2LR, mode='bilinear', size=(256, 256), align_corners=False)
img2BC = F.interpolate(img2LR, mode='bicubic', size=(256, 256), align_corners=False)

npimg2LR = img2LR.numpy()[0,0]
cv2.imwrite("LR" +".png", npimg2LR * 256)
npimg2ZP = img2ZP.numpy()[0,0]
cv2.imwrite("ZP" +".png", npimg2ZP * 256)
npimg2NN = img2NN.numpy()[0,0]
cv2.imwrite("NN" +".png", npimg2NN * 256)
npimg2BL = img2BL.numpy()[0,0]
cv2.imwrite("BL" +".png", npimg2BL * 256)
npimg2BC = img2BC.numpy()[0,0]
cv2.imwrite("BC" +".png", npimg2BC * 256)

#img2 = img2BL
img2 = torch.rand(img1.size())

if torch.cuda.is_available():
    img1 = img1.cuda()
    img2 = img2.cuda()


img1 = Variable( img1, requires_grad = False)
img2 = Variable( img2, requires_grad = True)


# Functional: pytorch_ssim.ssim(img1, img2, window_size = 11, size_average = True)
ssim_value = pytorch_ssim.ssim(img1, img2).data.item()
print("Initial ssim:", ssim_value)

# Module: pytorch_ssim.SSIM(window_size = 11, size_average = True)
ssim_loss = pytorch_ssim.SSIM(window_size = 11)
mse_loss = torch.nn.MSELoss()
l1_loss = torch.nn.L1Loss()
vgg_loss = VGGPerceptualLoss(resize = False)

optimizer = optim.Adam([img2], lr=0.01)

nr = 0
interval = 10
while nr < 1000:
    optimizer.zero_grad()
    #loss_out = mse_loss(img1, img2)
    #loss_out = l1_loss(img1, img2)
    #loss_out = -ssim_loss(img1, img2)
    loss_out = vgg_loss(img1, img2)
    #loss_out = vgg_loss(img1, img2) * 0.1 - ssim_loss(img1, img2)
    print("Total", loss_out.data.item())
    #print("VGG", loss1.data.item(), "MSE", loss2.data.item(), "Total", loss_out.data.item())
    loss_out.backward()
    optimizer.step()

    if(nr % interval == 0):
        img2CPU = torch.Tensor.cpu(torch.Tensor.detach(img2))
        npImg2Temp = img2CPU.numpy()[0,0]
        cv2.imwrite("max" + str(nr // interval) + ".png", npImg2Temp * 256)
    nr += 1
