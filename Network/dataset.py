import cv2
import numpy as np
import torch
from torch.utils.data import Dataset, DataLoader
import os
import PIL
import torch.autograd.profiler as profiler

ss_path_png =               '../DatasetGenerator/data/spp{0}/video{1}/spp{0}_v{1}_f{2}.png'
image_path_png =            '../DatasetGenerator/data/us{0}/images/video{1}/image_us{0}_v{1}_f{2}.png'
motion_vector_path_png =    '../DatasetGenerator/data/us{0}/motion_vectors/video{1}/motion_vectors_us{0}_v{1}_f{2}.png'
depth_path_png =            '../DatasetGenerator/data/us{0}/depth/video{1}/depth_us{0}_v{1}_f{2}.png'
jitter_path_png =           '../DatasetGenerator/data/us{0}/jitter/video{1}/jitter_us{0}_v{1}_f{2}.txt'

ss_path =               'data/spp{0}/video{1}/crop{3}/spp{0}_v{1}_f{2}.bmp'
image_path =            'data/us{0}/images/video{1}/crop{3}/image_us{0}_v{1}_f{2}.bmp'
motion_vector_path =    'data/us{0}/motion_vectors/video{1}/crop{3}/motion_vectors_us{0}_v{1}_f{2}.bmp'
depth_path =            'data/us{0}/depth/video{1}/crop{3}/depth_us{0}_v{1}_f{2}.bmp'
jitter_path =           'data/us{0}/jitter/video{1}/jitter_us{0}_v{1}_f{2}.txt'

def LoadTargetImage(ss_factor, video_index, frame_index, crop_index = None):
    if(crop_index == None):
        return cv2.imread(ss_path_png.format(ss_factor, video_index, frame_index))
    else:
        path = ss_path.format(ss_factor, video_index, frame_index, crop_index)
        return cv2.imread(ss_path.format(ss_factor, video_index, frame_index, crop_index))

def LoadInputImage(ds_factor, video_index, frame_index, crop_index = None):
    if(crop_index == None):
        return cv2.imread(image_path_png.format(ds_factor, video_index, frame_index))
    else:
        return cv2.imread(image_path.format(ds_factor, video_index, frame_index, crop_index))


# Motion vectors are stored as 4 8bit ints representing two 16bit floats
def LoadMotionVector(ds_factor, video_index, frame_index, crop_index = None):
    with profiler.record_function("mv_load"):
        if(crop_index == None):
            im = cv2.imread(motion_vector_path_png.format(ds_factor, video_index, frame_index), 0)
        else:
            im = cv2.imread(motion_vector_path.format(ds_factor, video_index, frame_index, crop_index), 0)
    height, width = im.shape
    with profiler.record_function("mv_process"):
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
def LoadDepthBuffer(ds_factor, video_index, frame_index, crop_index = None):
    if(crop_index == None):
        im = cv2.imread(depth_path_png.format(ds_factor, video_index, frame_index), 0)
    else:
        im = cv2.imread(depth_path.format(ds_factor, video_index, frame_index, crop_index), 0)
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

def CreateDir(name):
    try:
        os.mkdir(name)
        #print("Created", name, "directory:")
    except OSError as error:
        return
        #print(name, "directory allready exist")

def ConvertJitter(us_factor, video_index, frame_index):
    f1 = open(jitter_path_png.format(us_factor, video_index, frame_index), "r")
    f2 = open(jitter_path.format(us_factor, video_index, frame_index), "w")
    f2.write(f1.read())
    f1.close()
    f2.close()

def ConvertPNGDatasetToBMP(ss_factor, us_factor, video_count, frame_count, crop_count):
    # Preprocessing using crops
    crop_gen = RandomCrop(256, 1920, 1080, us_factor)
    crops = []
    print("Creating crop positions")
    for i in range(video_count):
        crops.append([])
        for j in range(crop_count):
            crops[i].append(crop_gen.create_crop())

    CreateDir('data')
    if(ss_factor != None):
        CreateDir('data/spp{}'.format(ss_factor))
        for i in range(video_count):
            CreateDir('data/spp{0}/video{1}'.format(ss_factor, i))
            for j in range(frame_count):
                print('Converting target image {0}/{1}     '.format(frame_count*i+j + 1, frame_count*video_count), end="\r")
                im = cv2.imread(ss_path_png.format(ss_factor, i, j))
                for k in range(crop_count):
                    CreateDir('data/spp{0}/video{1}/crop{2}'.format(ss_factor, i, k))
                    x1, x2, y1, y2 = crops[i][k]
                    cv2.imwrite(ss_path.format(ss_factor, i, j, k), im[y1*us_factor:y2*us_factor,x1*us_factor:x2*us_factor,:])
    print('')
    if(us_factor != None):
        CreateDir('data/us{}'.format(us_factor))
        CreateDir('data/us{}/images'.format(us_factor))
        CreateDir('data/us{}/depth'.format(us_factor))
        CreateDir('data/us{}/motion_vectors'.format(us_factor))
        CreateDir('data/us{}/jitter'.format(us_factor))
        for i in range(video_count):
            CreateDir('data/us{0}/images/video{1}'.format(us_factor, i))
            CreateDir('data/us{0}/depth/video{1}'.format(us_factor, i))
            CreateDir('data/us{0}/motion_vectors/video{1}'.format(us_factor, i))
            CreateDir('data/us{0}/jitter/video{1}'.format(us_factor, i))
            for j in range(frame_count):
                print('Converting input image image {0}/{1}     '.format(frame_count*i+j + 1, frame_count*video_count), end="\r")
                input_im = cv2.imread(image_path_png.format(us_factor, i, j))
                depth_im = cv2.imread(depth_path_png.format(us_factor, i, j))
                mv_im = cv2.imread(motion_vector_path_png.format(us_factor, i, j))
                for k in range(crop_count):
                    CreateDir('data/us{0}/images/video{1}/crop{2}'.format(us_factor, i, k))
                    CreateDir('data/us{0}/depth/video{1}/crop{2}'.format(us_factor, i, k))
                    CreateDir('data/us{0}/motion_vectors/video{1}/crop{2}'.format(us_factor, i, k))
                    x1, x2, y1, y2 = crops[i][k]
                    cv2.imwrite(image_path.format(us_factor, i, j, k), input_im[y1:y2,x1:x2,:])
                    cv2.imwrite(depth_path.format(us_factor, i, j, k), depth_im[y1:y2,x1*4:x2*4])
                    cv2.imwrite(motion_vector_path.format(us_factor, i, j, k), mv_im[y1:y2,x1*4:x2*4])
                ConvertJitter(us_factor, i, j)

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

def MVToPixelOffset(mv):
    mini_batch, height, width, channels = mv.shape
    x_ten = ((torch.arange(width*height, dtype=torch.float, device='cuda') % width + 0.5) / width - 0.5) * 2.0
    y_ten = ((torch.arange(width*height, dtype=torch.float, device='cuda') % height + 0.5) / height - 0.5) * 2.0
    x_ten = torch.reshape(x_ten, (height, width))
    y_ten = torch.reshape(y_ten, (width, height))
    y_ten = torch.t(y_ten)
    pixel_pos = torch.dstack((x_ten, y_ten))

    mv_t = (mv - pixel_pos)*0.5
    mv_t = mv_t * torch.tensor([height, width], device='cuda')

    
    mv_t = torch.movedim(mv_t, 3, 1)

    return mv_t

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

    mv = pixel_pos + mv * 2.0
    return mv.contiguous()

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

    def Load(self, transform, pre_cropped):
        crop_index = None
        if(pre_cropped):
            crop_index = torch.randint(0, 100, (1,)).item()
        x1, x2, y1, y2 = 0, 0, 0, 0
        if(transform != None):
            x1, x2, y1, y2 = transform.create_crop()
        #with profiler.profile(record_shapes=True) as prof:
            
        for i in self.target_indices:
            #with profiler.record_function("target"):
            self.target_images.append(LoadTargetImage(self.ss_factor, self.video, self.frame - i, crop_index))
            self.target_images[i] = ImageNumpyToTorch(self.target_images[i])
            if(transform != None):
                self.target_images[i] = transform.tf_target_image(self.target_images[i], x1, x2, y1, y2)

        for i in range(self.seq_length):
            #with profiler.record_function("input"):
            self.input_images.append(LoadInputImage(self.us_factor, self.video, self.frame - i, crop_index))
            self.input_images[i] = ImageNumpyToTorch(self.input_images[i])
            if(transform != None):
                self.input_images[i] = transform.tf_input_image(self.input_images[i], x1, x2, y1, y2)

            #with profiler.record_function("mv"):
            self.motion_vectors.append(LoadMotionVector(self.us_factor, self.video, self.frame - i, crop_index))
            self.motion_vectors[i] = MVNumpyToTorch(self.motion_vectors[i])
            if(transform != None):
                self.motion_vectors[i] = transform.tf_motion_vector(self.motion_vectors[i], x1, x2, y1, y2)
            #with profiler.record_function("depth"):
            self.depth_buffers.append(LoadDepthBuffer(self.us_factor, self.video, self.frame - i, crop_index))
            self.depth_buffers[i] = DepthNumpyToTorch(self.depth_buffers[i])
            if(transform != None):
                self.depth_buffers[i] = transform.tf_depth_buffer(self.depth_buffers[i], x1, x2, y1, y2)
            #with profiler.record_function("jitter"):
            self.jitters.append(LoadJitter(self.us_factor, self.video, self.frame - i))
            self.jitters[i] = torch.from_numpy(self.jitters[i])
        #print(prof.key_averages().table(sort_by="cpu_time_total", row_limit=10))

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

    def ToHalf(self):
        for i in range(len(self.target_images)):
            self.target_images[i] = self.target_images[i].half()
        for i in range(len(self.input_images)):
            self.input_images[i] = self.input_images[i].half()
        for i in range(len(self.motion_vectors)):
            self.motion_vectors[i] = self.motion_vectors[i].half()
        for i in range(len(self.depth_buffers)):
            self.depth_buffers[i] = self.depth_buffers[i].half()
        for i in range(len(self.jitters)):
            self.jitters[i] = self.jitters[i].half()

class RandomCrop():
    def __init__(self, size, image_width, image_height, us_factor):
        self.size = size
        self.width = image_width // us_factor
        self.height = image_height // us_factor
        self.us = us_factor

    def create_crop(self):
        x1 = torch.randint(self.width - self.size // self.us, (1,))
        x2 = x1 + self.size // self.us
        y1 = torch.randint(self.height - self.size // self.us, (1,))
        y2 = y1 + self.size // self.us
        return x1, x2, y1, y2

    def tf_target_image(self, target_image, x1, x2, y1, y2):
        return target_image[:,y1*self.us:y2*self.us,x1*self.us:x2*self.us].contiguous()
    def tf_input_image(self, input_image, x1, x2, y1, y2):
        return input_image[:,y1:y2,x1:x2].contiguous()
    def tf_depth_buffer(self, depth_buffer, x1, x2, y1, y2):
        return depth_buffer[y1:y2,x1:x2].contiguous()
    def tf_motion_vector(self, mv, x1, x2, y1, y2):
        mv = mv[y1:y2,x1:x2,:]
        mv = (mv + 1.0) * 0.5
        mv = mv * torch.tensor([self.width, self.height])
        mv = mv - torch.tensor([x1, y1])
        mv = mv / (self.size // self.us)
        mv = (mv - 0.5) * 2.0
        return mv.contiguous()

        
num_videos = 2
num_frames_per_video = 60

class SSDataset(Dataset):

    def __init__(self, ss_factor, us_factor, videos, seq_length, target_indices, transform=None, pre_cropped=False):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.videos = videos
        self.seq_length = seq_length
        self.target_indices = target_indices
        self.transform = transform
        self.pre_cropped = pre_cropped

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
        d.Load(self.transform, self.pre_cropped)

        return d

def SSDatasetCollate(batch):
    out = SSDatasetItem(batch[0].ss_factor, batch[0].us_factor, batch[0].video, batch[0].frame, batch[0].seq_length, batch[0].target_indices)
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

class TargetImageDataset(Dataset):

    def __init__(self, ss_factor, videos, transform=None):
        self.ss_factor = ss_factor
        self.videos = videos
        self.transform = transform

    def max_allowed_frames(self):
        return num_frames_per_video

    def __len__(self):
        return len(self.videos) * self.max_allowed_frames()

    def __getitem__(self, idx):
        if (torch.is_tensor(idx)):
            idx = idx.tolist()
        
        video_index = idx // self.max_allowed_frames()
        frame_index = idx % self.max_allowed_frames()
        target_image = LoadTargetImage(self.ss_factor, video_index, frame_index)
        target_image = ImageNumpyToTorch(target_image)
        if(self.transform):
            target_image = self.transform(target_image)
        
        return target_image