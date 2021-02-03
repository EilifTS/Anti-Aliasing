import pytorch_ssim
import torch
from torch.autograd import Variable
from torch import optim 
import torch.nn.functional as F
from vgg_perceptual_loss import VGGPerceptualLoss
import cv2
import numpy as np
import dataset

im = dataset.LoadMotionVector(4, 0, 4)
#for i in im:
#    print(i)

window_name = "Image"
cv2.imshow(window_name, im[:,:,0])
cv2.waitKey(0)
cv2.destroyAllWindows()