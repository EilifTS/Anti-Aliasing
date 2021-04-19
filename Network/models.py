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
    
    mv = UVToGrid(uv)

    color = F.grid_sample(texture, mv, mode='bilinear', align_corners=False, padding_mode='border')

    
    bs, channels, width, height = color.shape
    alpha = torch.ones(size=(bs, 1, width, height)).cuda()

    #color[mask] = 0.0

    return torch.cat((color, alpha), dim=1)

def BiCubicGridSample(texture, grid):
    N, channels, height, width = texture.shape
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

    # Reshape for multiplication
    _, h_grid, w_grid, _ = grid.shape
    w0 = w0.unsqueeze(1)
    w12 = w12.unsqueeze(1)
    w3 = w3.unsqueeze(1)
    w0 = w0.expand((N, channels + 1, h_grid, w_grid, 2))
    w12 = w12.expand((N, channels + 1, h_grid, w_grid, 2))
    w3 = w3.expand((N, channels + 1, h_grid, w_grid, 2))


    c1 = catmullSample(texture, sp1) * (w12[:,:,:,:,0] *  w0[:,:,:,:,1])
    c2 = catmullSample(texture, sp2) * ( w0[:,:,:,:,0] * w12[:,:,:,:,1])
    c3 = catmullSample(texture, sp3) * ( w3[:,:,:,:,0] * w12[:,:,:,:,1])
    c4 = catmullSample(texture, sp4) * (w12[:,:,:,:,0] *  w3[:,:,:,:,1])
    c5 = center_color * (w12[:,:,:,:,0] * w12[:,:,:,:,1])
    out = c1 + c2 + c3 + c4 + c5
    rec = out[:,-1:,:,:]
    rec = rec.expand((N,channels,h_grid,w_grid))
    out = out[:,:channels,:,:] / rec

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
   
    return out[:,:channels,:,:]

def catmullSampleHD(texture, uv):
    
    mv = UVToGrid(uv)

    color = F.grid_sample(texture, mv, mode='bilinear', align_corners=False, padding_mode='border')

    return color

def BiCubicGridSampleHD(texture, grid):
    N, channels, height, width = texture.shape
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
    tc0 = rec_window_size * (center_position - 1.0)
    tc3 = rec_window_size * (center_position + 2.0)

    # Reshape for multiplication
    _, h_grid, w_grid, _ = grid.shape
    w0 = w0.unsqueeze(1)
    w12 = w12.unsqueeze(1)
    w3 = w3.unsqueeze(1)
    w0 = w0.expand((N, channels, h_grid, w_grid, 2))
    w12 = w12.expand((N, channels, h_grid, w_grid, 2))
    w3 = w3.expand((N, channels, h_grid, w_grid, 2))

    # More accurate version that uses more samples
    sp1 = torch.stack((tc0[:,:,:,0], tc0[:,:,:,1]),dim=3)
    sp2 = torch.stack((tc12[:,:,:,0], tc0[:,:,:,1]),dim=3)
    sp3 = torch.stack((tc3[:,:,:,0], tc0[:,:,:,1]),dim=3)
    sp4 = torch.stack((tc0[:,:,:,0], tc12[:,:,:,1]),dim=3)
    sp5 = torch.stack((tc12[:,:,:,0], tc12[:,:,:,1]),dim=3)
    sp6 = torch.stack((tc3[:,:,:,0], tc12[:,:,:,1]),dim=3)
    sp7 = torch.stack((tc0[:,:,:,0], tc3[:,:,:,1]),dim=3)
    sp8 = torch.stack((tc12[:,:,:,0], tc3[:,:,:,1]),dim=3)
    sp9 = torch.stack((tc3[:,:,:,0], tc3[:,:,:,1]),dim=3)
    c1 = catmullSampleHD(texture, sp1) * (w0[:,:,:,:,0] * w0[:,:,:,:,1])
    c2 = catmullSampleHD(texture, sp2) * (w12[:,:,:,:,0] * w0[:,:,:,:,1])
    c3 = catmullSampleHD(texture, sp3) * (w3[:,:,:,:,0] * w0[:,:,:,:,1])
    c4 = catmullSampleHD(texture, sp4) * (w0[:,:,:,:,0] * w12[:,:,:,:,1])
    c5 = catmullSampleHD(texture, sp5) * (w12[:,:,:,:,0] * w12[:,:,:,:,1])
    c6 = catmullSampleHD(texture, sp6) * (w3[:,:,:,:,0] * w12[:,:,:,:,1])
    c7 = catmullSampleHD(texture, sp7) * (w0[:,:,:,:,0] * w3[:,:,:,:,1])
    c8 = catmullSampleHD(texture, sp8) * (w12[:,:,:,:,0] * w3[:,:,:,:,1])
    c9 = catmullSampleHD(texture, sp9) * (w3[:,:,:,:,0] * w3[:,:,:,:,1])
    out = c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9
   
    return out

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

def JitterAlign(img, jitter, factor, mode='constant'):
    N, C, H, W = img.shape
    jitter_index = torch.floor(jitter*factor).int()
    img = F.pad(img, (factor,factor,factor,factor), mode=mode)
    out = []
    for b in range(N):
        i, j = jitter_index[b,1], jitter_index[b,0]
        out.append(img[b,:,factor-i:-factor-i,factor-j:-factor-j])
    img = torch.stack(out)
    return img

def JitterAlignedInterpolation(img, jitter, factor, mode='bilinear'):
    N, C, H, W = img.shape
    theta = []
    for i in range(N):
        theta.append(torch.tensor(
            [   [1,0,2.0*(0.5-jitter[i,0]) / W],
                [0,1,2.0*(0.5-jitter[i,1]) / H]], device='cuda'))
    theta = torch.stack(theta)
    grid = F.affine_grid(theta, (N,C,H*factor,W*factor), align_corners=False).float()
    #return BiCubicGridSample(img, grid)
    return F.grid_sample(img, grid, mode=mode, align_corners=False, padding_mode='border')

def IdentityGrid(shape):
    N, H, W, C = shape
    theta = torch.tensor(
        [   [1.0,0.0,0.0],
            [0.0,1.0,0.0]], device='cuda')
    theta = theta.unsqueeze(0).expand(N, -1, -1)
    grid = F.affine_grid(theta, (N,C,H,W), align_corners=False).float()
    return grid

def DepthToLinear(depth):
    far, near = 100.0, 0.1
    depth = near * far / (far - depth * (far - near))
    return (depth - near) / (far - near)

def RGBToYCoCg(image):
    Y = 0.25*image[:,0:1,:,:] + 0.5*image[:,1:2,:,:] + 0.25*image[:,2:3,:,:]
    Co = 0.5*image[:,0:1,:,:] + 0.0*image[:,1:2,:,:] - 0.5*image[:,2:3,:,:]
    Cg = -0.25*image[:,0:1,:,:] + 0.5*image[:,1:2,:,:] - 0.25*image[:,2:3,:,:]
    return torch.cat((Y,Co,Cg), dim=1)

def YCoCgToRGB(image):
    R = image[:,0:1,:,:] + image[:,1:2,:,:] - image[:,2:3,:,:]
    G = image[:,0:1,:,:] + 0.0*image[:,1:2,:,:] + image[:,2:3,:,:]
    B = image[:,0:1,:,:] - image[:,1:2,:,:] - image[:,2:3,:,:]
    return torch.cat((R,G,B), dim=1)

class TUS(nn.Module):
    def __init__(self, factor, alpha):
        super(TUS, self).__init__()
        self.factor = factor
        self.alpha = alpha
        self.history = None
        self.zero_up = ZeroUpsampling(self.factor, 3)

    def forward(self, x):
        out = self.zero_up.forward_with_jitter(x.input_images[0], x.jitters[0])
        
        if(self.history != None):
            upsampled_mv = torch.movedim(x.motion_vectors[0], 3, 1)
            upsampled_mv = F.interpolate(upsampled_mv, scale_factor=self.factor, mode='bilinear', align_corners=False)
            upsampled_mv = torch.movedim(upsampled_mv, 1, 3)

            reprojected_history = F.grid_sample(self.history, upsampled_mv, mode='bilinear', align_corners=False)
            out = self.alpha * out + (1-self.alpha) * reprojected_history

        self.history = out
        return out

    def reset_history(self):
        self.history = None

    def name(self):
        name = "TAA_" + str(self.alpha)
        if(self.use_history_camp):
            name += "_clamp"
        if(self.use_bic):
            name += "_cube"
        return name

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
            nn.Upsample(scale_factor=2, mode='bilinear', align_corners=False)
            )
        self.up = nn.Sequential(
            nn.Conv2d(192, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Upsample(scale_factor=2, mode='bilinear', align_corners=False)
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

class FBNet(nn.Module):
    def __init__(self, factor):
        super(FBNet, self).__init__()
        self.factor = factor
        self.main_feature_extractor = FBFeatureExtractor(factor)
        self.other_feature_extractor = FBFeatureExtractor(factor)
        self.reweighting = nn.Sequential(
            nn.Conv2d(20, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 4, 3, stride=1, padding=1),
            nn.Tanh(),
            )
        self.reconstruct = FBUNet()

        
        for p in self.parameters():
            if(p.requires_grad):
                if(p.dim() == 1): # Bias
                    torch.nn.init.zeros_(p)
                else: # Weight 
                    torch.nn.init.xavier_normal_(p)

    def forward(self, x):
        num_frames = 5
        x.ToCuda()
        frames = [x.input_images[i] for i in range(num_frames)]
        depths = [DepthToLinear(x.depth_buffers[i].unsqueeze(1)) for i in range(num_frames)]
        frames = [torch.cat(x, dim=1) for x in zip(frames, depths)]
        del depths

        frames[0] = self.main_feature_extractor(frames[0])
        for i in range(1, num_frames):
            frames[i] = self.other_feature_extractor(frames[i])
        frames = [JitterAlign(frames[i], x.jitters[i], self.factor) for i in range(num_frames)]

        mvs = [x.motion_vectors[i] for i in range(num_frames - 1)]

        for i in range(len(mvs)):
            mvs[i] = torch.movedim(mvs[i], 3, 1)
            mvs[i] = F.interpolate(mvs[i], scale_factor=self.factor, mode='bilinear', align_corners=False)
            mvs[i] = torch.movedim(mvs[i], 1, 3)


        for i in range(1, num_frames):
            for j in range(i - 1, -1, -1):
                frames[i] = F.grid_sample(frames[i], mvs[j], mode='bilinear', align_corners=False)
        del mvs

        feature_weight_input = torch.cat((
            frames[0][:,:4,:,:],
            frames[1][:,:4,:,:], 
            frames[2][:,:4,:,:], 
            frames[3][:,:4,:,:], 
            frames[4][:,:4,:,:]), dim=1)
        feature_weights = (self.reweighting(feature_weight_input) + 1.0) * 5.0
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

    
def pixel_unshuffle(input, downscale_factor):
    '''
    input: batchSize * c * k*w * k*h
    kdownscale_factor: k
    batchSize * c * k*w * k*h -> batchSize * k*k*c * w * h
    '''
    c = input.shape[1]

    kernel = torch.zeros(size=[downscale_factor * downscale_factor * c,
                               1, downscale_factor, downscale_factor],
                         device=input.device)
    for y in range(downscale_factor):
        for x in range(downscale_factor):
            kernel[x + y * downscale_factor::downscale_factor*downscale_factor, 0, y, x] = 1
    return F.conv2d(input, kernel, stride=downscale_factor, groups=c)

class PixelUnshuffle(nn.Module):
    def __init__(self, downscale_factor):
        super(PixelUnshuffle, self).__init__()
        self.downscale_factor = downscale_factor
    def forward(self, input):
        '''
        input: batchSize * c * k*w * k*h
        kdownscale_factor: k
        batchSize * c * k*w * k*h -> batchSize * k*k*c * w * h
        '''

        return pixel_unshuffle(input, self.downscale_factor)


class MasterFeatureExtractor(nn.Module):
    def __init__(self, factor):
        super(MasterFeatureExtractor, self).__init__()
        self.net = nn.Sequential(
            nn.Conv2d(4, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 8, 3, stride=1, padding=1),
            nn.ReLU()
            )

    def forward(self, x):
        return torch.cat((x, 
                       self.net(x)
                       ), dim=1)
       
class MasterUNet(nn.Module):
    def __init__(self):
        super(MasterUNet, self).__init__()
        self.down = nn.MaxPool2d(2)
        self.up = nn.Upsample(scale_factor=2, mode='bilinear')

        self.start = nn.Sequential(
            nn.Conv2d(16, 64, 3, stride=1, padding=1),
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
            nn.Upsample(scale_factor=2, mode='bilinear', align_corners=False)
            )
        self.up = nn.Sequential(
            nn.Conv2d(192, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 64, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Upsample(scale_factor=2, mode='bilinear', align_corners=False)
            )
        self.end = nn.Sequential(
            nn.Conv2d(96, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 4, 3, stride=1, padding=1),
            )
    def forward(self, x):
        x1 = self.start(x)
        x2 = self.down(x1)
        x3 = self.bottom(x2)
        x4 = self.up(torch.cat((x2, x3), dim=1))
        del x2, x3
        return self.end(torch.cat((x1, x4), dim=1))

class MasterNet(nn.Module):
    def __init__(self, factor, num_frames):
        super(MasterNet, self).__init__()
        self.factor = factor
        self.num_frames = num_frames
        self.history_channels = 4
        self.zero_up = ZeroUpsampling(factor, 4)
        self.cnn = nn.Sequential(
            nn.ReplicationPad2d(1),
            nn.Conv2d(8, 64, 3, stride=1, padding=0),
            nn.ReLU(),
            nn.ReplicationPad2d(1),
            nn.Conv2d(64, 64, 3, stride=1, padding=0),
            nn.ReLU(),
            nn.ReplicationPad2d(1),
            nn.Conv2d(64, 2, 3, stride=1, padding=0),
            )

    def sub_forward(self, frame, depth, mv, jitter, history):
        mini_batch, channels, height, width = frame.shape

        # Linearizing depth
        far, near = 100.0, 0.1
        depth = near * far / (far - depth * (far - near))
        depth = (depth - near) / (far - near)

        # Getting frame info
        frame = torch.cat((frame, depth), dim=1)
        
        # Frame upsampling
        frame_bicubic = JitterAlignedInterpolation(frame, jitter, self.factor, mode='bicubic')
        frame = self.zero_up(frame)
        frame = JitterAlign(frame, jitter, self.factor)

        if(history == None): # First frame is handled by its own
            history = frame_bicubic
        else:
            # Upscaling motion vector
            mv = torch.movedim(mv, 3, 1)
            mv = F.interpolate(mv, scale_factor=self.factor, mode='bilinear', align_corners=False)
            mv = torch.movedim(mv, 1, 3)
        
            # History reprojection
            history = F.grid_sample(history, mv, mode='bicubic', align_corners=False, padding_mode='border')
            
            ## Special case for padding
            mask = torch.logical_or(torch.greater(mv, 1.0), torch.less(mv, -1.0))
            mask = torch.logical_or(mask[:,:,:,0], mask[:,:,:,1])
            mask = mask.unsqueeze(1)
            mask = mask.expand(mini_batch, self.history_channels, height*self.factor, width*self.factor)

            history[mask] = frame_bicubic[mask]

        cnn_input = torch.cat((frame, history), dim=1)
        residual = self.cnn(cnn_input)


        # History reweighting
        alpha = torch.clamp(residual[:,0:1,:,:], 0.0, 1.0)

        history = history*(1.0 - alpha) + frame_bicubic * alpha
        history[:,3,:,:] = history[:,3,:,:] + residual[:,1,:,:]
        history = torch.clamp(history, 0.0, 1.0)
 
        # Reconstruction
        return history[:,:3,:,:], history

    def forward(self, x):
        history = None
        out = []
        for i in range(x.seq_length - 1, -1, -1):
            reconstructed, history = self.sub_forward(
                x.input_images[i].cuda(), 
                x.depth_buffers[i].unsqueeze(1).cuda(), 
                x.motion_vectors[i].cuda(), 
                x.jitters[i].cuda(), 
                history)
            if(i in x.target_indices):
                out.append(reconstructed[:,:,:,:])
            
            # Memory optimization
            x.input_images[i] = None 
            x.depth_buffers[i] = None 
            x.motion_vectors[i] = None 
            x.jitters[i] = None
            torch.cuda.empty_cache()
        out.reverse()
        return out

class MasterNet2(nn.Module):
    def __init__(self, factor):
        super(MasterNet2, self).__init__()
        self.factor = factor
        self.history_channels = 4

        self.zero_up = ZeroUpsampling(factor, 4)

        self.down = nn.Sequential(
            #PixelUnshuffle(self.factor),
            #nn.Conv2d(128, 32, 1, stride=1, padding=0),
            nn.Conv2d(8, 32, 4, stride=4),
            )
        self.cnn1 = nn.Sequential(
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            )
        self.cnn2 = nn.Sequential(
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            )
        self.cnn3 = nn.Sequential(
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            )
        self.cnn4 = nn.Sequential(
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            nn.ReLU(),
            nn.Conv2d(32, 32, 3, stride=1, padding=1, padding_mode='replicate'),
            )

        self.up = nn.Sequential(
            nn.PixelShuffle(self.factor),
            )


    def sub_forward(self, frame, depth, mv, jitter, history):
        mini_batch, channels, height, width = frame.shape

        # Linearizing depth
        depth = DepthToLinear(depth)

        # Getting frame info
        #frame_padding = torch.zeros(size=(mini_batch, 32 - 4, height, width), device='cuda')
        frame_ones = torch.cat((frame, torch.ones(size=( mini_batch, 1, height, width), device='cuda')), dim=1)
        
        # Frame upsampling
        #frame = JitterAlignedBilinearInterpolation(frame, jitter, self.factor)
        frame_bilinear = JitterAlignedInterpolation(frame_ones, jitter, self.factor, mode='bilinear')
        frame = torch.cat((frame, depth), dim=1)
        frame = self.zero_up(frame)
        frame = JitterAlign(frame, jitter, self.factor)

        if(history == None): # First frame is handled by its own
            #history = torch.zeros(size=(mini_batch, self.history_channels, height*self.factor, width*self.factor), device='cuda')
            history = frame_bilinear
            #history = torch.rand(size=(mini_batch, self.history_channels, height*self.factor, width*self.factor), device='cuda')
        else:
            # Upscaling motion vector
            mv -= IdentityGrid(mv.shape)
            mv = torch.movedim(mv, 3, 1)
            
            
            # MV-diation
            #_, depth_fronts = F.max_pool2d(1.1-depth, 3, stride=1, padding=1, return_indices=True)
            #f_depth_fronts = torch.flatten(depth_fronts, 2)
            #f_mv = torch.flatten(mv, 2)
            #f_mv1 = torch.gather(f_mv[:,0:1,:], 2, f_depth_fronts)
            #f_mv2 = torch.gather(f_mv[:,1:2,:], 2, f_depth_fronts)
            #mv_dil = torch.cat((f_mv1, f_mv2), dim=1).view((mini_batch, 2, height, width))

            mv = F.interpolate(mv, scale_factor=self.factor, mode='bilinear', align_corners=False)
            #mv_dil = F.interpolate(mv_dil, scale_factor=self.factor, mode='bilinear', align_corners=False)
            
            mv = torch.movedim(mv, 1, 3)
            mv += IdentityGrid(mv.shape)
        
            # History reprojection
            #history = F.grid_sample(history, mv, mode='bilinear', align_corners=False)
            history = F.grid_sample(history, mv, mode='bicubic', align_corners=False, padding_mode='border')
            #history = BiCubicGridSample(history, mv)
            

            ## Special case for padding
            mask = torch.logical_or(torch.greater(mv, 1.0), torch.less(mv, -1.0))
            mask = torch.logical_or(mask[:,:,:,0], mask[:,:,:,1])
            mask = mask.unsqueeze(1)
            mask = mask.expand(mini_batch, 4, height*self.factor, width*self.factor)

            history[mask] = frame_bilinear[mask]


        small_input = torch.cat((frame, history), dim=1)
        small_input = self.down(small_input)
        small_input = small_input + self.cnn1(small_input)
        small_input = small_input + self.cnn2(small_input)
        small_input = small_input + self.cnn3(small_input)
        small_input = small_input + self.cnn4(small_input)
        residual = self.up(small_input)

        # History reweighting
        residual = torch.clamp(residual, 0.0, 1.0)
        alpha = residual[:,0:1,:,:]

        history = history*(1.0 - alpha) + frame_bilinear * alpha
        history_start = history[:,0:3,:,:]
        history_end = history[:,3:4,:,:] * residual[:,1:2,:,:]
        history = torch.cat((history_start, history_end), dim=1)
        history = torch.clamp(history, 0, 1.0)
        # Reconstruction
        return history[:,:3,:,:], history

    def forward(self, x):
        history = None
        out = []

        for i in range(x.seq_length - 1, -1, -1):
            reconstructed, history = self.sub_forward(
                x.input_images[i].cuda(), 
                x.depth_buffers[i].unsqueeze(1).cuda(), 
                x.motion_vectors[i].cuda(), 
                x.jitters[i].cuda(), 
                history)
            if(i in x.target_indices):
                out.append(reconstructed[:,:,:,:])
            
            # Memory optimization
            x.input_images[i] = None 
            x.depth_buffers[i] = None 
            x.motion_vectors[i] = None 
            x.jitters[i] = None
            torch.cuda.empty_cache()
        out.reverse()
        return out

    
class TraditionalModel(nn.Module):
    def __init__(self, factor, mode, jitter_aligned=False):
        super(TraditionalModel, self).__init__()
        self.factor = factor
        self.mode = mode
        self.jitter_aligned = jitter_aligned
        self.up = nn.Upsample(scale_factor=factor, mode='bilinear', align_corners=False)

    def sub_forward(self, frame, depth, mv, jitter, history):
        # Reconstruction
        if(self.jitter_aligned):
            return JitterAlignedInterpolation(frame, jitter, self.factor, self.mode), history
        else:
            return F.interpolate(frame, scale_factor=self.factor, mode=self.mode, align_corners=False), history

    def forward(self, x):
        history = None
        out = []
        reconstructed, history = self.sub_forward(
            x.input_images[0], 
            x.depth_buffers[0].unsqueeze(1), 
            x.motion_vectors[0], 
            x.jitters[0], 
            history)
        out.append(reconstructed[:,:,:,:])
        out.reverse()
        return out