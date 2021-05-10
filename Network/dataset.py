import cv2
import numpy as np
import torch
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader
import os
import PIL
import h5py
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
   # with profiler.record_function("mv_load"):
    im = cv2.imread(motion_vector_path.format(ds_factor, video_index, frame_index), 0)
        
    height, width = im.shape
    #with profiler.record_function("mv_process"):
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

def CreateDir(name):
    try:
        os.mkdir(name)
        #print("Created", name, "directory:")
    except OSError as error:
        return

def ConvertJitter(us_factor, video_index, frame_index):
    f1 = open(jitter_path_png.format(us_factor, video_index, frame_index), "r")
    f2 = open(jitter_path.format(us_factor, video_index, frame_index), "w")
    f2.write(f1.read())
    f1.close()
    f2.close()

def ConvertPNGDatasetToBMP(ss_factor, us_factor, video_count, frame_count):
    CreateDir('data')
    if(ss_factor != None):
        CreateDir('data/spp{}'.format(ss_factor))
        for i in range(video_count):
            CreateDir('data/spp{0}/video{1}'.format(ss_factor, i))
            for j in range(frame_count):
                print('Converting target image {0}/{1}     '.format(frame_count*i+j + 1, frame_count*video_count), end="\r")
                im = cv2.imread(ss_path_png.format(ss_factor, i, j))
                cv2.imwrite(ss_path.format(ss_factor, i, j), im)
                
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
                depth_im = cv2.imread(depth_path_png.format(us_factor, i, j), 0)
                mv_im = cv2.imread(motion_vector_path_png.format(us_factor, i, j), 0)
                cv2.imwrite(image_path.format(us_factor, i, j), input_im)
                cv2.imwrite(depth_path.format(us_factor, i, j), depth_im)
                cv2.imwrite(motion_vector_path.format(us_factor, i, j), mv_im)
                ConvertJitter(us_factor, i, j)

def ConvertPNGDatasetToH5(ss_factor, us_factor, video_count, frame_count):
    CreateDir('datah5')
    f = h5py.File('datah5/dataset.hdf5', 'w')
    ds_targets = f.create_dataset('spp' + str(ss_factor), (video_count * frame_count, 1080, 1920, 3), dtype='uint8')
    if(ss_factor != None):
        for i in range(video_count):
            for j in range(frame_count):
                print('Converting target image {0}/{1}     '.format(frame_count*i+j + 1, frame_count*video_count), end="\r")
                im = LoadTargetImage(ss_factor, i, j)
                ds_targets[i * frame_count + j,:,:,:] = im

    for factor in us_factor:
        ds_input = f.create_dataset('us' + str(factor)+'image', (video_count * frame_count, 1080 // factor, 1920 // factor, 3), dtype='uint8')
        ds_depth = f.create_dataset('us' + str(factor)+'depth', (video_count * frame_count, 1080 // factor, 1920 // factor, 1), dtype='float32')
        ds_mv = f.create_dataset('us' + str(factor)+'mv', (video_count * frame_count, 1080 // factor, 1920 // factor, 2), dtype='float16')
        ds_jitter = f.create_dataset('us' + str(factor)+'jitter', (video_count * frame_count, 2), dtype='float32')
        
        print("First iteration might be slower than the rest")
        for i in range(video_count):
            for j in range(frame_count):
                print('Converting input image image {0}/{1}     '.format(frame_count*i+j + 1, frame_count*video_count), end="\r")
                data = LoadInputImage(factor, i, j)
                ds_input[i * frame_count + j,:,:,:] = data
                data = np.expand_dims(LoadDepthBuffer(factor, i, j), axis=2)
                ds_depth[i * frame_count + j,:,:] = data
                data = LoadMotionVector(factor, i, j)
                ds_mv[i * frame_count + j,:,:,:] = data
                data = LoadJitter(factor, i, j)
                ds_jitter[i * frame_count + j,:] = data

    f.close()

def LoadTargetImageH5(f, ss_factor, start_index, frame_count):
    name = 'spp' + str(ss_factor)
    return f[name][start_index:start_index+frame_count,:,:,:]
def LoadTargetImageH5Crop(f, ss_factor, start_index, frame_count, crop_x, crop_y, crop_size):
    name = 'spp' + str(ss_factor)
    return f[name][start_index:start_index+frame_count,crop_y:crop_y+crop_size,crop_x:crop_x+crop_size,:]
def LoadInputImageH5(f, us_factor, start_index, frame_count):
    name = 'us' + str(us_factor) + 'image'
    return f[name][start_index:start_index+frame_count,:,:,:]
def LoadInputImageH5Crop(f, us_factor, start_index, frame_count, crop_x, crop_y, crop_size):
    name = 'us' + str(us_factor) + 'image'
    return f[name][start_index:start_index+frame_count,crop_y:crop_y+crop_size,crop_x:crop_x+crop_size,:]
def LoadDepthH5(f, us_factor, start_index, frame_count):
    name = 'us' + str(us_factor) + 'depth'
    return f[name][start_index:start_index+frame_count,:,:,:]
def LoadDepthH5Crop(f, us_factor, start_index, frame_count, crop_x, crop_y, crop_size):
    name = 'us' + str(us_factor) + 'depth'
    return f[name][start_index:start_index+frame_count,crop_y:crop_y+crop_size,crop_x:crop_x+crop_size,:]
def LoadMVH5(f, us_factor, start_index, frame_count):
    name = 'us' + str(us_factor) + 'mv'
    _, H, W, _ = f[name].shape
    data = f[name][start_index:start_index+frame_count,:,:,:]
    return data * np.array([W, H])
def LoadMVH5Crop(f, us_factor, start_index, frame_count, crop_x, crop_y, crop_size):
    name = 'us' + str(us_factor) + 'mv'
    _, H, W, _ = f[name].shape
    data = f[name][start_index:start_index+frame_count,crop_y:crop_y+crop_size,crop_x:crop_x+crop_size,:]
    return data * np.array([W, H])
def LoadJitterH5(f, us_factor, start_index, frame_count):
    name = 'us' + str(us_factor) + 'jitter'
    return f[name][start_index:start_index+frame_count,:]

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

def IdentityGrid(shape):
    N = 1
    H, W, C = shape
    theta = torch.tensor(
        [   [1.0,0.0,0.0],
            [0.0,1.0,0.0]])
    theta = theta.unsqueeze(0).expand(N, -1, -1)
    grid = F.affine_grid(theta, (N,C,H,W), align_corners=False).float()
    return grid[0,:,:,:]

def MVNumpyToTorch(mv):
    mv = torch.from_numpy(mv)
    mv = mv.float() # Grid sample requires same format as image :(

    H, W, _ = mv.shape
    mv = mv / torch.tensor([W, H])

    # MVs are stored as offsets, but absolute coordinates are needed by grid_sample
    mv = IdentityGrid(mv.shape) + mv * 2.0
    return mv.contiguous()

def MVTorchToNumpy(mv):
    mv = mv.numpy()
    return mv

def DepthNumpyToTorch(depth):
    depth = np.swapaxes(depth, 2, 0)
    depth = np.swapaxes(depth, 2, 1)
    depth = torch.from_numpy(depth)
    return depth.contiguous()

def DepthTorchToNumpy(depth):
    depth = depth.numpy()
    depth = np.swapaxes(depth, 2, 1)
    depth = np.swapaxes(depth, 2, 0)
    return depth


num_frames_per_video = 60
class SSDatasetItem():
    def __init__(self, ss_factor, us_factor, video, frame, seq_length, target_count):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.video = video
        self.frame = frame
        self.seq_length = seq_length
        self.target_count = target_count
        self.target_images = []
        self.input_images = []
        self.motion_vectors = []
        self.depth_buffers = []
        self.jitters = []

    def Load(self, transform):
        load_targets = None
        load_images = None
        load_depth = None
        load_mv = None
        load_jitter = None
        start_index = self.video * num_frames_per_video + self.frame - self.seq_length + 1
        frame_count = self.seq_length
        target_count = self.target_count

        # Load data
        f = h5py.File('datah5/dataset.hdf5', 'r')
        if(transform != None):
            x, y, s = transform.create_crop()
            us = transform.us
            load_targets = LoadTargetImageH5Crop(f, self.ss_factor, start_index + frame_count - target_count, target_count, x * us, y * us, s * us)
            load_images = LoadInputImageH5Crop(f, self.us_factor, start_index, frame_count, x, y, s)
            load_depth = LoadDepthH5Crop(f, self.us_factor, start_index, frame_count, x, y, s)
            load_mv = LoadMVH5Crop(f, self.us_factor, start_index, frame_count, x, y, s)
            load_jitter = LoadJitterH5(f, self.us_factor, start_index, frame_count)
        else:
            load_targets = LoadTargetImageH5(f, self.ss_factor, start_index + frame_count - target_count, target_count)
            load_images = LoadInputImageH5(f, self.us_factor, start_index, frame_count)
            load_depth = LoadDepthH5(f, self.us_factor, start_index, frame_count)
            load_mv = LoadMVH5(f, self.us_factor, start_index, frame_count)
            load_jitter = LoadJitterH5(f, self.us_factor, start_index, frame_count)


        #with profiler.profile(record_shapes=True) as prof:
        
        # Restructure data
        for i in range(target_count):
            #with profiler.record_function("target"):
            self.target_images.append(load_targets[i,:,:,:])
            self.target_images[i] = ImageNumpyToTorch(self.target_images[i])

        for i in range(self.seq_length):
            #with profiler.record_function("input"):
            self.input_images.append(load_images[i,:,:,:])
            self.input_images[i] = ImageNumpyToTorch(self.input_images[i])

            #with profiler.record_function("mv"):
            self.motion_vectors.append(load_mv[i,:,:,:])
            self.motion_vectors[i] = MVNumpyToTorch(self.motion_vectors[i])
            #with profiler.record_function("depth"):
            self.depth_buffers.append(load_depth[i,:,:,:])
            self.depth_buffers[i] = DepthNumpyToTorch(self.depth_buffers[i])
            #with profiler.record_function("jitter"):
            self.jitters.append(load_jitter[i,:])
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
        x = torch.randint(self.width - self.size // self.us, (1,))
        y = torch.randint(self.height - self.size // self.us, (1,))
        return x, y, self.size // self.us

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

class SSDataset(Dataset):

    def __init__(self, ss_factor, us_factor, videos, seq_length, target_count, transform=None):
        self.ss_factor = ss_factor
        self.us_factor = us_factor
        self.videos = videos
        self.seq_length = seq_length
        self.target_count = target_count
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
        d = SSDatasetItem(self.ss_factor, self.us_factor, self.videos[video_index], frame_index, self.seq_length, self.target_count)
        d.Load(self.transform)

        return d

def SSDatasetCollate(batch):
    out = SSDatasetItem(batch[0].ss_factor, batch[0].us_factor, batch[0].video, batch[0].frame, batch[0].seq_length, batch[0].target_count)
    for i in range(out.target_count):
        out.target_images.append(batch[0].target_images[i].unsqueeze(0))
    for i in range(out.seq_length):
        out.input_images.append(batch[0].input_images[i].unsqueeze(0))
        out.motion_vectors.append(batch[0].motion_vectors[i].unsqueeze(0))
        out.depth_buffers.append(batch[0].depth_buffers[i].unsqueeze(0))
        out.jitters.append(batch[0].jitters[i].unsqueeze(0))
    for i in range(1, len(batch)):
        for j in range(out.target_count):
            out.target_images[j] =  torch.cat((out.target_images[j],    batch[i].target_images[j].unsqueeze(0)),    dim=0)
        for j in range(out.seq_length):
            out.input_images[j] =   torch.cat((out.input_images[j],     batch[i].input_images[j].unsqueeze(0)),     dim=0)
            out.motion_vectors[j] = torch.cat((out.motion_vectors[j],   batch[i].motion_vectors[j].unsqueeze(0)),   dim=0)
            out.depth_buffers[j] =  torch.cat((out.depth_buffers[j],    batch[i].depth_buffers[j].unsqueeze(0)),    dim=0)
            out.jitters[j] =        torch.cat((out.jitters[j],          batch[i].jitters[j].unsqueeze(0)),          dim=0)
    return out