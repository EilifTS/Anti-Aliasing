import cv2
import numpy as np
import torch
from torch.utils.data import Dataset, DataLoader

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
    i2 = np.array(im[:,0::4], dtype=np.int16)
    i3 = np.array(im[:,0::4], dtype=np.int16)
    i4 = np.array(im[:,0::4], dtype=np.int16)
    i1 = np.left_shift(i1, 8*1)
    i2 = np.left_shift(i2, 8*0)
    i3 = np.left_shift(i3, 8*1)
    i4 = np.left_shift(i4, 8*0)

    # Combine into two image
    im2 = np.bitwise_or(i1, i2)
    im3 = np.bitwise_or(i3, i4)

    # Combine the two images into a single 2 component image
    im4 = np.dstack((im2, im3))

    return im4.view(np.float16) # Return as float

# Depth is stored as 4 8bit ints representing a 32bit float
def LoadDepthBuffer(ds_factor, video_index, frame_index):
    im = cv2.imread(depth_path.format(ds_factor, video_index, frame_index), 0)
    height, width = im.shape

    # Change from 8-bit ints to 32-bit ints and split into the 4 components of the float
    i1 = np.array(im[:,0::4], dtype=np.int32)
    i2 = np.array(im[:,0::4], dtype=np.int32)
    i3 = np.array(im[:,0::4], dtype=np.int32)
    i4 = np.array(im[:,0::4], dtype=np.int32)
    i1 = np.left_shift(i1, 8*3)
    i2 = np.left_shift(i2, 8*2)
    i3 = np.left_shift(i3, 8*1)
    i4 = np.left_shift(i4, 8*0)

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

class SSDatasetItem():
    def __init__(self, ss_factor, us_factor, video, frame):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.video = video
        self.frame = frame
        self.target_image = ""
        self.input_image = ""
        self.motion_vector = ""
        self.depth_buffer = ""
        self.jitter = ""

    def Load(self):
        self.target_image = LoadTargetImage(self.ss_factor, self.video, self.frame)
        self.input_image = LoadInputImage(self.us_factor, self.video, self.frame)
        self.motion_vector = LoadMotionVector(self.us_factor, self.video, self.frame)
        self.depth_buffer = LoadDepthBuffer(self.us_factor, self.video, self.frame)
        self.jitter = LoadJitter(self.us_factor, self.video, self.frame)

    def ToTorch(self):
        self.target_image = ImageNumpyToTorch(self.target_image)
        self.input_image = ImageNumpyToTorch(self.input_image)

        
num_videos = 2
num_frames_per_video = 60

class SSDataset(Dataset):

    def __init__(self, ss_factor, us_factor, videos, transforms):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.videos = videos
        self.transforms = transforms

    def __len__(self):
        return len(videos) * num_frames_per_video

    def __getitem__(self, idx):
        if (torch.is_tensor(idx)):
            idx = idx.tolist()

        video_index = idx // num_frames_per_video
        frame_index = idx % num_frames_per_video
        d = SSDatasetItem(self.ss_factor, self.us_factor, self.videos[video_index], frame_index)
        d.Load()
        d.ToTorch()

        return d