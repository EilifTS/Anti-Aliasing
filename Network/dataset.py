import cv2
import numpy as np
import torch
from torch.utils.data import Dataset, DataLoader
import torch.autograd.profiler as profiler

ss_path =               '../DatasetGenerator/data/spp{0}/video{1}/spp{0}_v{1}_f{2}.png'
image_path =            '../DatasetGenerator/data/us{0}/images/video{1}/image_us{0}_v{1}_f{2}.png'
motion_vector_path =    '../DatasetGenerator/data/us{0}/motion_vectors/video{1}/motion_vectors_us{0}_v{1}_f{2}.png'
depth_path =            '../DatasetGenerator/data/us{0}/depth/video{1}/depth_us{0}_v{1}_f{2}.png'
jitter_path =           '../DatasetGenerator/data/us{0}/jitter/video{1}/jitter_us{0}_v{1}_f{2}.txt'

def LoadTargetImage(ss_factor, video_index, frame_index):
    return cv2.imread(ss_path.format(ss_factor, video_index, frame_index))

def LoadInputImage(ds_factor, video_index, frame_index):
    return cv2.imread(image_path.format(ds_factor, video_index, frame_index))

# Motion vectors are stored as 4 8bit ints representing two 16bit floats
def LoadMotionVector(ds_factor, video_index, frame_index):
    im = cv2.imread(motion_vector_path.format(ds_factor, video_index, frame_index), 0)
    height, width = im.shape

    # Change from 8-bit ints to 16-bit ints and split into the 4 components of the floats
    i1 = np.array(im[:,0::4], dtype=np.int16)
    i2 = np.array(im[:,1::4], dtype=np.int16)
    i3 = np.array(im[:,2::4], dtype=np.int16)
    i4 = np.array(im[:,3::4], dtype=np.int16)
    i1 = np.left_shift(i1, 8*0)
    i2 = np.left_shift(i2, 8*1)
    i3 = np.left_shift(i3, 8*0)
    i4 = np.left_shift(i4, 8*1)

    # Combine into two image
    im2 = np.bitwise_or(i1, i2)
    im3 = np.bitwise_or(i3, i4)

    # Combine the two images into a single 2 component image
    im4 = np.dstack((im2, im3))
    im4 = im4.view(np.float16) # Return as float

    return im4 

# Depth is stored as 4 8bit ints representing a 32bit float
def LoadDepthBuffer(ds_factor, video_index, frame_index):
    im = cv2.imread(depth_path.format(ds_factor, video_index, frame_index), 0)
    height, width = im.shape

    # Change from 8-bit ints to 32-bit ints and split into the 4 components of the float
    i1 = np.array(im[:,0::4], dtype=np.int32)
    i2 = np.array(im[:,1::4], dtype=np.int32)
    i3 = np.array(im[:,2::4], dtype=np.int32)
    i4 = np.array(im[:,3::4], dtype=np.int32)
    i1 = np.left_shift(i1, 8*0)
    i2 = np.left_shift(i2, 8*1)
    i3 = np.left_shift(i3, 8*2)
    i4 = np.left_shift(i4, 8*3)

    # Combine into one image
    im2 = np.bitwise_or(i1, 
          np.bitwise_or(i2,
          np.bitwise_or(i3, i4)))
    return im.view(np.float32) # Return as float

def LoadJitter(ds_factor, video_index, frame_index):
    f = open(jitter_path.format(ds_factor, video_index, frame_index), "r")
    text = f.read()
    f.close()
    text_split = text.split(" ")
    jitter = np.array( [float(text_split[0]), float(text_split[1])] )
    return jitter

def ImageNumpyToTorch(image):
    image = np.swapaxes(image, 2, 0)
    image = np.swapaxes(image, 2, 1)
    image = torch.from_numpy(image)
    image = image.float()
    image = image / 255.0
    return image.contiguous()

def ImageTorchToNumpy(image):
    image = image * 255.0
    image = image.byte()
    image = image.numpy()
    image = np.swapaxes(image, 2, 1)
    image = np.swapaxes(image, 2, 0)
    return image

def MVNumpyToTorch(mv):
    mv = torch.from_numpy(mv)
    mv = mv.float() # Grid sample requires same format as image :(
    
    # MVs are stored as offsets, but absolute coordinates are needed by grid_sample
    height, width, channels = mv.shape
    x_ten = ((torch.arange(width*height, dtype=torch.float) % width + 0.5) / width - 0.5) * 2.0
    y_ten = ((torch.arange(width*height, dtype=torch.float) % height + 0.5) / height - 0.5) * 2.0
    x_ten = torch.reshape(x_ten, (height, width))
    y_ten = torch.reshape(y_ten, (width, height))
    y_ten = torch.t(y_ten)
    pixel_pos = torch.dstack((x_ten, y_ten))

    return pixel_pos + mv * 2.0

def MVTorchToNumpy(mv):
    mv = mv.numpy()
    return mv

def DepthNumpyToTorch(depth):
    depth = torch.from_numpy(depth)
    return depth.contiguous()

def DepthTorchToNumpy(depth):
    return depth.numpy()

class SSDatasetItem():
    def __init__(self, ss_factor, us_factor, video, frame, seq_length, target_indices):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.video = video
        self.frame = frame
        self.seq_length = seq_length
        self.target_indices = target_indices
        self.target_images = []
        self.input_images = []
        self.motion_vectors = []
        self.depth_buffers = []
        self.jitters = []

    def Load(self):
        self.target_images =    [LoadTargetImage(self.ss_factor, self.video, self.frame - i)    for i in self.target_indices]
        self.input_images =     [LoadInputImage(self.us_factor, self.video, self.frame - i)     for i in range(self.seq_length)]
        self.motion_vectors =   [LoadMotionVector(self.us_factor, self.video, self.frame - i)   for i in range(self.seq_length)]
        self.depth_buffers =    [LoadDepthBuffer(self.us_factor, self.video, self.frame - i)    for i in range(self.seq_length)]
        self.jitters =          [LoadJitter(self.us_factor, self.video, self.frame - i)         for i in range(self.seq_length)]

    def ToTorch(self):
        self.target_images =     [ImageNumpyToTorch(self.target_images[i])   for i in range(len(self.target_images))]
        self.input_images =      [ImageNumpyToTorch(self.input_images[i])    for i in range(self.seq_length)]
        self.motion_vectors =    [MVNumpyToTorch(self.motion_vectors[i])     for i in range(self.seq_length)]
        self.depth_buffers =     [DepthNumpyToTorch(self.depth_buffers[i])   for i in range(self.seq_length)]
        self.jitters =           [torch.from_numpy(self.jitters[i])          for i in range(self.seq_length)]

    def ToCuda(self):
        for i in range(len(self.target_images)):
            self.target_images[i] = self.target_images[i].cuda()
        for i in range(len(self.input_images)):
            self.input_images[i] = self.input_images[i].cuda()
        for i in range(len(self.motion_vectors)):
            self.motion_vectors[i] = self.motion_vectors[i].cuda()
        for i in range(len(self.depth_buffers)):
            self.depth_buffers[i] = self.depth_buffers[i].cuda()
        for i in range(len(self.jitters)):
            self.jitters[i] = self.jitters[i].cuda()

class RandomCrop():
    def __init__(self, size):
        self.size = size

    def __call__(self, item : SSDatasetItem):
        _, height, width = item.input_images[0].shape
        us = item.us_factor
        x1 = torch.randint(width - self.size // us, (1,))
        x2 = x1 + self.size // us
        y1 = torch.randint(height - self.size // us, (1,))
        y2 = y1 + self.size // us
        for i in range(len(item.target_images)):
            item.target_images[i] = item.target_images[i][:,y1*us:y2*us,x1*us:x2*us]
        for i in range(item.seq_length):
            item.input_images[i] = item.input_images[i][:,y1:y2,x1:x2]
            item.depth_buffers[i] = item.depth_buffers[i][y1:y2,x1:x2]

            item.motion_vectors[i] = item.motion_vectors[i][y1:y2,x1:x2,:]
            item.motion_vectors[i] = (item.motion_vectors[i] + 1.0) * 0.5
            item.motion_vectors[i] = item.motion_vectors[i] * torch.tensor([width, height])
            item.motion_vectors[i] = item.motion_vectors[i] - torch.tensor([x1, y1])
            item.motion_vectors[i] = item.motion_vectors[i] / (self.size // us)
            item.motion_vectors[i] = (item.motion_vectors[i] - 0.5) * 2.0
        return item
        
num_videos = 2
num_frames_per_video = 60

class SSDataset(Dataset):

    def __init__(self, ss_factor, us_factor, videos, seq_length, target_indices, transform=None):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.videos = videos
        self.seq_length = seq_length
        self.target_indices = target_indices
        self.transform = transform

    def max_allowed_frames(self):
        return num_frames_per_video - (self.seq_length - 1)

    def __len__(self):
        return len(self.videos) * self.max_allowed_frames()

    def __getitem__(self, idx):
        if (torch.is_tensor(idx)):
            idx = idx.tolist()
        
        video_index = idx // self.max_allowed_frames()
        frame_index = self.seq_length - 1 + idx % self.max_allowed_frames()
        d = SSDatasetItem(self.ss_factor, self.us_factor, self.videos[video_index], frame_index, self.seq_length, self.target_indices)
        d.Load()
        d.ToTorch()
        if(self.transform):
            d = self.transform(d)
        
        return d

def SSDatasetCollate(batch):
    out = SSDatasetItem(batch[0].ss_factor, batch[0].us_factor, -1, -1, batch[0].seq_length, batch[0].target_indices)
    for i in range(len(out.target_indices)):
        out.target_images.append(batch[0].target_images[i].unsqueeze(0))
    for i in range(out.seq_length):
        out.input_images.append(batch[0].input_images[i].unsqueeze(0))
        out.motion_vectors.append(batch[0].motion_vectors[i].unsqueeze(0))
        out.depth_buffers.append(batch[0].depth_buffers[i].unsqueeze(0))
        out.jitters.append(batch[0].jitters[i].unsqueeze(0))
    for i in range(1, len(batch)):
        for j in range(len(out.target_images)):
            out.target_images[j] =  torch.cat((out.target_images[j],    batch[i].target_images[j].unsqueeze(0)),    dim=0)
        for j in range(out.seq_length):
            out.input_images[j] =   torch.cat((out.input_images[j],     batch[i].input_images[j].unsqueeze(0)),     dim=0)
            out.motion_vectors[j] = torch.cat((out.motion_vectors[j],   batch[i].motion_vectors[j].unsqueeze(0)),   dim=0)
            out.depth_buffers[j] =  torch.cat((out.depth_buffers[j],    batch[i].depth_buffers[j].unsqueeze(0)),    dim=0)
            out.jitters[j] =        torch.cat((out.jitters[j],          batch[i].jitters[j].unsqueeze(0)),          dim=0)
    return out

