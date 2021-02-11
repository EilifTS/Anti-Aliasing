import torch
import torch.nn as nn
import torch.nn.functional as F
import cv2
import dataset

def GridToUV(grid):
    return (grid + 1.0) * 0.5

def UVToGrid(uv):
    return (uv - 0.5) * 2.0

def catmullSample(texture, uv):
    bs, _, width, height = texture.shape
    alpha = torch.ones(size=(bs, 1, width, height)).cuda()
    color = F.grid_sample(texture, UVToGrid(uv), mode='bilinear')

    return torch.cat((color, alpha), dim=1)

def BiCubicGridSample(texture, grid):
    _, _, height, width = texture.shape
    window_size = torch.tensor([width, height]).cuda()
    rec_window_size = torch.tensor([1.0 / width, 1.0 / height]).cuda()
    uv = GridToUV(grid)
    position = uv * window_size
    center_position = torch.floor(position - 0.5) + 0.5
    f = position - center_position
    f2 = f * f
    f3 = f2 * f

    w0 = -0.5 * f3 + f2 - 0.5 * f
    w1 = 1.5 * f3 - 2.5 * f2 + 1.0
    w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f
    w3 = 0.5 * f3 - 0.5 * f2

    w12 = w1 + w2
    tc12 = rec_window_size * (center_position + w2 / w12)
    center_color = catmullSample(texture, tc12)
    tc0 = rec_window_size * (center_position - 1.0)
    tc3 = rec_window_size * (center_position + 2.0)
    
    sp1 = torch.stack((tc12[:,:,:,0], tc0[:,:,:,1]),dim=3)
    sp2 = torch.stack((tc0[:,:,:,0], tc12[:,:,:,1]),dim=3)
    sp3 = torch.stack((tc3[:,:,:,0], tc12[:,:,:,1]),dim=3)
    sp4 = torch.stack((tc12[:,:,:,0], tc3[:,:,:,1]),dim=3)
    c1 = catmullSample(texture, sp1) * (w12[:,:,:,0] * w0[:,:,:,1])
    c2 = catmullSample(texture, sp2) * (w0[:,:,:,0] * w12[:,:,:,1])
    c3 = catmullSample(texture, sp3) * (w3[:,:,:,0] * w12[:,:,:,1])
    c4 = catmullSample(texture, sp4) * (w12[:,:,:,0] * w3[:,:,:,1])
    c5 = center_color * (w12[:,:,:,0] * w12[:,:,:,1])
    out = c1 + c2 + c3 + c4 + c5
    out = out[:,:3,:,:] / out[:,-1,:,:]

    # More accurate version that uses more samples
    #sp1 = torch.stack((tc0[:,:,:,0], tc0[:,:,:,1]),dim=3)
    #sp2 = torch.stack((tc12[:,:,:,0], tc0[:,:,:,1]),dim=3)
    #sp3 = torch.stack((tc3[:,:,:,0], tc0[:,:,:,1]),dim=3)
    #sp4 = torch.stack((tc0[:,:,:,0], tc12[:,:,:,1]),dim=3)
    #sp5 = torch.stack((tc12[:,:,:,0], tc12[:,:,:,1]),dim=3)
    #sp6 = torch.stack((tc3[:,:,:,0], tc12[:,:,:,1]),dim=3)
    #sp7 = torch.stack((tc0[:,:,:,0], tc3[:,:,:,1]),dim=3)
    #sp8 = torch.stack((tc12[:,:,:,0], tc3[:,:,:,1]),dim=3)
    #sp9 = torch.stack((tc3[:,:,:,0], tc3[:,:,:,1]),dim=3)
    #c1 = catmullSample(texture, sp1) * (w0[:,:,:,0] * w0[:,:,:,1])
    #c2 = catmullSample(texture, sp2) * (w12[:,:,:,0] * w0[:,:,:,1])
    #c3 = catmullSample(texture, sp3) * (w3[:,:,:,0] * w0[:,:,:,1])
    #c4 = catmullSample(texture, sp4) * (w0[:,:,:,0] * w12[:,:,:,1])
    #c5 = catmullSample(texture, sp5) * (w12[:,:,:,0] * w12[:,:,:,1])
    #c6 = catmullSample(texture, sp6) * (w3[:,:,:,0] * w12[:,:,:,1])
    #c7 = catmullSample(texture, sp7) * (w0[:,:,:,0] * w3[:,:,:,1])
    #c8 = catmullSample(texture, sp8) * (w12[:,:,:,0] * w3[:,:,:,1])
    #c9 = catmullSample(texture, sp9) * (w3[:,:,:,0] * w3[:,:,:,1])
    #out = c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9
   
    return torch.clamp(out[:,:3,:,:], 0, 1)

class TAA(nn.Module):

    def __init__(self, alpha, hist_clamp, use_bic):
        super(TAA, self).__init__()
        self.alpha = alpha
        self.use_history_camp = hist_clamp
        self.use_bic = use_bic
        self.history = ""
        self.max_pool = nn.MaxPool2d(3, stride=1, padding=1)

    def forward(self, x, mv):
        out = x
        if(self.history != ""):
            reprojected_history = F.grid_sample(self.history, mv, mode='bilinear')
            if(self.use_bic):
                reprojected_history = BiCubicGridSample(self.history, mv)
            #History clamping
            if(self.use_history_camp):
                max_nh = self.max_pool(out)
                min_nh = -self.max_pool(-out)

                # Torch clamping is wonky
                reprojected_history, _ = torch.max(torch.stack((reprojected_history, min_nh)), dim=0)
                reprojected_history, _ = torch.min(torch.stack((reprojected_history, max_nh)), dim=0)
                #reprojected_history = torch.clamp(reprojected_history, min_nh, max_nh)

            out = self.alpha * out + (1-self.alpha) * reprojected_history
        self.history = out
        return out

    def reset_history(self):
        self.history = ""

    def name(self):
        name = "TAA_" + str(self.alpha)
        if(self.use_history_camp):
            name += "_clamp"
        if(self.use_bic):
            name += "_cube"
        return name

# U-Net helper classes
class FBFeatureExtractor(nn.Module):
    def __init__(self, factor):
        super(FBFeatureExtractor, self).__init__()
        self.net = nn.Sequential(
            nn.Conv2d(4, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 8, 3, stride=1, padding=1),
            nn.ReLU()
            )
        self.upsampler = ZeroUpsampling(factor, 12)

    def forward(self, x):
        return self.upsampler(
            torch.cat((x, 
                       self.net(x)
                       ), dim=1)
            )

class FBUNet(nn.Module):
    def __init__(self):
        super(FBUNet, self).__init__()
        self.down = nn.MaxPool2d(2)
        self.up = nn.Upsample(scale_factor=2, mode='bilinear')

        self.start = nn.Sequential(
            nn.Conv2d(60, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 32, 3, stride=1, padding=1),
            nn.ReLU()
            )
        self.down = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Conv2d(32, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 64, 3, stride=1, padding=1),
            nn.ReLU()
            )
        self.bottom = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Conv2d(64, 128, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(128, 128, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Upsample(scale_factor=2, mode='bilinear')
            )
        self.up = nn.Sequential(
            nn.Conv2d(192, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Upsample(scale_factor=2, mode='bilinear')
            )
        self.end = nn.Sequential(
            nn.Conv2d(96, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 3, 3, stride=1, padding=1),
            nn.ReLU()
            )
    def forward(self, x):
        x1 = self.start(x)
        x2 = self.down(x1)
        x3 = self.bottom(x2)
        x4 = self.up(torch.cat((x2, x3), dim=1))
        del x2, x3
        return self.end(torch.cat((x1, x4), dim=1))

class ZeroUpsampling(nn.Module):
    def __init__(self, factor, channels):
        super(ZeroUpsampling, self).__init__()
        self.factor = factor
        self.channels = channels
        self.w = torch.zeros(size=(self.factor, self.factor))
        self.w[0, 0] = 1
        self.w = self.w.expand(channels, 1, self.factor, self.factor).cuda()

    def forward(self, x):
        return F.conv_transpose2d(x, self.w, stride=self.factor, groups=self.channels)

class FBNet(nn.Module):
    def __init__(self, factor):
        super(FBNet, self).__init__()
        self.factor = factor
        self.feature_extractor = FBFeatureExtractor(factor)
        self.reweighting = nn.Sequential(
            nn.Conv2d(20, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 4, 3, stride=1, padding=1),
            nn.Tanh(),
            )
        self.reconstruct = FBUNet()

    def forward(self, x):
        num_frames = 5
        frames = [x.input_images[i] for i in range(num_frames)]
        depths = [x.depth_buffers[i].unsqueeze(1) for i in range(num_frames)]
        frames = [torch.cat(x, dim=1) for x in zip(frames, depths)]
        del depths

        frames = [self.feature_extractor(f) for f in frames]
        
        mvs = [x.motion_vectors[i] for i in range(num_frames - 1)]

        for i in range(len(mvs)):
            mvs[i] = torch.movedim(mvs[i], 3, 1)
            mvs[i] = F.upsample(mvs[i], scale_factor=self.factor, mode='bilinear')
            mvs[i] = torch.movedim(mvs[i], 1, 3)


        for i in range(1, num_frames):
            for j in range(i - 1, -1, -1):
                frames[i] = F.grid_sample(frames[i], mvs[j], mode='bilinear')
        del mvs

        feature_weight_input = torch.cat((
            frames[0][:,:4,:,:],
            frames[1][:,:4,:,:], 
            frames[2][:,:4,:,:], 
            frames[3][:,:4,:,:], 
            frames[4][:,:4,:,:]), dim=1)
        feature_weights = (self.reweighting(feature_weight_input) + 1.0) * 10.0
        del feature_weight_input

        frames[1] = frames[1] * feature_weights[:,0:1,:,:]
        frames[2] = frames[2] * feature_weights[:,1:2,:,:]
        frames[3] = frames[3] * feature_weights[:,2:3,:,:]
        frames[4] = frames[4] * feature_weights[:,3:4,:,:]
        del feature_weights

        reconstruction_input = torch.cat((
            frames[0], 
            frames[1], 
            frames[2], 
            frames[3], 
            frames[4]), dim=1)
        del frames

        return self.reconstruct(reconstruction_input)



