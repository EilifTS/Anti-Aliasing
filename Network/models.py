import torch
import torch.nn as nn
import torch.nn.functional as F

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
