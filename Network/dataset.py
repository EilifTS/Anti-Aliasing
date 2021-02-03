import cv2
import numpy as np

image_path =            '../DatasetGenerator/data/us{0}/images/video{1}/image_us{0}_v{1}_f{2}.png'
motion_vector_path =    '../DatasetGenerator/data/us{0}/motion_vectors/video{1}/motion_vectors_us{0}_v{1}_f{2}.png'
depth_path =            '../DatasetGenerator/data/us{0}/depth/video{1}/depth_us{0}_v{1}_f{2}.png'

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
    print(im4.shape)

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