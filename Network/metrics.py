
import torch
import torchvision
import torch.nn.functional as F
from torch.autograd import Variable
import numpy as np
from math import exp

class VGGPerceptualLoss(torch.nn.Module):
    def __init__(self, resize=True):
        super(VGGPerceptualLoss, self).__init__()
        blocks = []
        blocks.append(torchvision.models.vgg16(pretrained=True).features[:4].eval())
        blocks.append(torchvision.models.vgg16(pretrained=True).features[4:9].eval())
        blocks.append(torchvision.models.vgg16(pretrained=True).features[9:16].eval())
        blocks.append(torchvision.models.vgg16(pretrained=True).features[16:23].eval())
        for bl in blocks:
            for p in bl:
                p.requires_grad = False
                p = p.cuda()
        self.blocks = torch.nn.ModuleList(blocks)
        self.transform = torch.nn.functional.interpolate
        self.mean = torch.nn.Parameter(torch.tensor([0.485, 0.456, 0.406], device='cuda').view(1,3,1,1))
        self.std = torch.nn.Parameter(torch.tensor([0.229, 0.224, 0.225], device='cuda').view(1,3,1,1))
        self.resize = resize

    def forward(self, input, target):
        if input.shape[1] != 3:
            input = input.repeat(1, 3, 1, 1)
            target = target.repeat(1, 3, 1, 1)
        input = (input-self.mean) / self.std
        target = (target-self.mean) / self.std
        if self.resize:
            input = self.transform(input, mode='bilinear', size=(224, 224), align_corners=False)
            target = self.transform(target, mode='bilinear', size=(224, 224), align_corners=False)
        loss = 0.0
        x = input
        y = target
        for block in self.blocks:
            x = block(x)
            y = block(y)
            loss += torch.nn.functional.l1_loss(x, y)
        return loss

class PSNR(torch.nn.Module):
    def __init__(self):
        super(PSNR, self).__init__()

    def forward(self, img1, img2):
        mse = F.mse_loss(img1, img2)
        psnr = 20*torch.log10(1.0 / torch.sqrt(mse))
        return psnr

    def name(self):
        return "PSNR"

    def unit(self):
        return "dB"

def gaussian(window_size, sigma):
    gauss = torch.Tensor([exp(-(x - window_size//2)**2/float(2*sigma**2)) for x in range(window_size)])
    return gauss/gauss.sum()

def create_window(window_size, channel):
    _1D_window = gaussian(window_size, 1.5).unsqueeze(1)
    _2D_window = _1D_window.mm(_1D_window.t()).float().unsqueeze(0).unsqueeze(0)
    window = _2D_window.expand(channel, 1, window_size, window_size).contiguous()
    return window

def _ssim(img1, img2, window, window_size, channel, size_average = True):
    mu1 = F.conv2d(img1, window, padding = window_size//2, groups = channel)
    mu2 = F.conv2d(img2, window, padding = window_size//2, groups = channel)

    mu1_sq = mu1.pow(2)
    mu2_sq = mu2.pow(2)
    mu1_mu2 = mu1*mu2

    sigma1_sq = F.conv2d(img1*img1, window, padding = window_size//2, groups = channel) - mu1_sq
    sigma2_sq = F.conv2d(img2*img2, window, padding = window_size//2, groups = channel) - mu2_sq
    sigma12 = F.conv2d(img1*img2, window, padding = window_size//2, groups = channel) - mu1_mu2
    
    C1 = 0.01**2
    C2 = 0.03**2

    ssim_map = ((2*mu1_mu2 + C1)*(2*sigma12 + C2))/((mu1_sq + mu2_sq + C1)*(sigma1_sq + sigma2_sq + C2))
    if size_average:
        return ssim_map.mean()
    else:
        return ssim_map.mean(1).mean(1).mean(1)

class SSIM(torch.nn.Module):
    def __init__(self, window_size = 11, size_average = True):
        super(SSIM, self).__init__()
        self.window_size = window_size
        self.size_average = size_average
        self.channel = 1
        self.window = create_window(window_size, self.channel)

    def forward(self, img1, img2):
        (_, channel, _, _) = img1.size()

        if channel == self.channel and self.window.data.type() == img1.data.type():
            window = self.window
        else:
            window = create_window(self.window_size, channel)
            
            if img1.is_cuda:
                window = window.cuda(img1.get_device())
            window = window.type_as(img1)
            
            self.window = window
            self.channel = channel


        return _ssim(img1, img2, window, self.window_size, channel, self.size_average)

    def name(self):
        return "SSIM"

    def unit(self):
        return ""

def ssim(img1, img2, window_size = 11, size_average = True):
    (_, channel, _, _) = img1.size()
    window = create_window(window_size, channel)
    
    if img1.is_cuda:
        window = window.cuda(img1.get_device())
    window = window.type_as(img1)
    
    return _ssim(img1, img2, window, window_size, channel, size_average)

class FBLoss(torch.nn.Module):
    def __init__(self):
        super(FBLoss, self).__init__()
        self.ssim = SSIM()
        self.vgg = VGGPerceptualLoss()

    def forward(self, img1, img2):
        return 1.0 - self.ssim(img1[0], img2) + 0.1 * self.vgg(img1[0], img2)

class MasterLoss(torch.nn.Module):
    def __init__(self):
        super(MasterLoss, self).__init__()
        self.ssim = SSIM()

    def forward(self, img1, img2):
        loss = torch.tensor([0.0], device="cuda")
        for i in range(len(img1)):
            #loss += 1.0 - self.ssim(img1[i], img2[i])
            loss += F.mse_loss(img1[i], img2[i])
        return loss / len(img1)

class MasterLoss2(torch.nn.Module):
    def __init__(self, target_indices):
        super(MasterLoss2, self).__init__()
        self.target_indices = target_indices
        self.ssim = SSIM()

    def forward(self, img1, img2):
        loss = torch.tensor([0.0], device="cuda")
        dloss = torch.tensor([0.0], device="cuda")
        weight = torch.tensor([0.0], device="cuda")
        for i in range(len(img1)):
            if(i != 0):
                dt1 = img1[i-1].cuda() - img1[i].cuda()
                dt2 = img2[i-1].cuda() - img2[i].cuda()
                dloss += F.l1_loss(dt1, dt2)
            #loss += F.mse_loss(img1[i].cuda(), img2[i].cuda())
            loss += F.l1_loss(img1[i].cuda(), img2[i].cuda())
            #loss += 1.0 - self.ssim(img1[i].cuda(), img2[i].cuda())
            weight += 1.0
            torch.cuda.empty_cache()
        #print(dloss.item(), loss.item())
        return (loss / weight) + 0.0*(dloss / (weight - 1.0))

class MasterLoss3(torch.nn.Module):
    def __init__(self, target_indices):
        super(MasterLoss3, self).__init__()
        self.target_indices = target_indices
        self.ssim = SSIM()

    def forward(self, img1, img2):
        loss = torch.tensor([0.0], device="cuda")
        dloss = torch.tensor([0.0], device="cuda")
        weight = torch.tensor([0.0], device="cuda")
        for i in range(len(img1)):
            if(i != 0):
                dt1 = img1[i-1].cuda() - img1[i].cuda()
                dt2 = img2[i-1].cuda() - img2[i].cuda()
                dloss += F.mse_loss(dt1, dt2)
            loss += F.mse_loss(img1[i].cuda(), img2[i].cuda())
            weight += 1.0
            torch.cuda.empty_cache()
        return (loss / weight) + (dloss / (weight - 1.0))