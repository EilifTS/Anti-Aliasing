# Anti-Aliasing

Anti-Aliasing is a Windows 10 desktop application with implementations of several different anti-aliasing methods.

# Installation
This project consists of three sub projects, two C++ projects and one Python project.
The C++ projects require Micosoft Visual Studio 2019 with the "Desktop development with C++" option and the latest Windows SDK.
In addition is the DirectML NuGet package required.

To run the python project, the following python packages are required: PyTorch, torchvision, CV2, Matplotlib, Numpy, h5py and PIL.

# Hardware requirement
Certain hardware is necessary to run the neural network efficiently.
A GPU which supports native 16bit operations is necessary to avoid a bug where errors from 32bit to 16bit conversions accumulate, but the network is able to run without.
A GPU with tensor cores is necessary to run the network efficiently.
A GPU using the NHWC format is necessary to run the network efficiently.
From NVIDIA should a GTX 1060 or newer work. 

# Description

`Anti-Aliasing/app.cpp`                 : Main file for running the real-time application

`DatasetGenetator/DatasetGenerator.cpp` : Main file for dataset generation

`ELib/graphics/`                        : Everything related to DirectX 12

`ELib/math/`                            : Some helper classes for math

`ELib/window/`                          : Everything related to Windows

`ELib/misc/`                            : Helper functions and classes for various purposes

`Rendering/deep_learning`               : Everything related to DirectML and the execution of DLCTUS

`Rendering/deferred_rendering`          : Classes used for deferred rendering

`Rendering/ray_tracer`                  : Class for using ray tracing

`Rendering/models`                      : .obj files for meshes used

`Rendering/textures`                    : Texture files for mesh textures

`Rendering/aa`                          : Classes for applying anti-aliasing

`Rendering/shaders`                     : Contains all shaders used in this project

`Network`                               : Contains python code related to the project


# Usage

| Keyboard and Mouse Input | Action |
|--- | --- |
| WASD | Movement control |
| Mouse movement | Rotation |
| 1 | Set Anti-aliasing mode off |
| 2 | Set Anti-aliasing mode to FXAA |
| 3 | Set Anti-aliasing mode to TAA |
| 4 | Set Anti-aliasing mode to SSAA |
| 5 | Toggle freeze frame (The right arrow to skip frames) |
| 6 | Toggle ray tracing |
| 7 | Toggle DLCTUS |

*When in TAA mode:*

| Keyboard and Mouse Input | Action |
|--- | --- |
| R | Toggle between bilinear interpolation and bicubic interpolation |
| T | Toggle history rectification |
| Y | Toggle between YCoCg space and RGB space |
| U | Toggle between history clipping and history clamping |


Temporal upsampling is used by changing the constants `upsample_numerator` and `upsample_denominator` in `Anti-Aliasing/app.cpp` at lines 19 and 20

# Deep Learning Pipeline
The deep learning pipeline has several steps:
1. Camera Motion Capture
2. Dataset Generation
3. Dataset Conversion
4. Network Training
5. Network Conversion
6. Network Loading

All steps are not necessary to run the network, as several pretrained networks are included in the project.

## Camera Motion Capture
Camera motion capure is done by starting the main application `Anti-Aliasing`. Then pressing the "Q"-button will record a 60-frame sequence video.
The video is stored as a .txt file in `DatasetGenerator/camera_positions/`, and the file contains world time, camera position and rotation.
100 prerecorded .txt files are allready stored in the folder.

## Dataset Generation
Dataset generation is done by exectuing the `DatasetGenerator` project. The project will iterate over all video files in `DatasetGenerator/camera_positions/` and generate input color, motion vector and depth buffers and save them as .png files to `DatasetGenerator/data`. In addition is the frames jitter offset saved to a .txt file.
A number of control variables are defined at the start of `DatasetGenerator/DataGenerator.cpp` which control the generation process. `super_sample_options` define how many samples are used for the target image, and `upsample_factor_options` define the upsampling factor used for the input images. `mipmap_bias` can be used to add an additional mipmap bias for both input and target images.

## Dataset Conversion
This stage converts the dataset from .png to .hdf5. This is done in Python by running the `Network.py` file, and making sure that the `dataset.ConvertPNGDatasetToH5` function is ran at the start of the file.

## Network Training
Network training is perfromed but the `Network.py`-file. It is important that the right values are set for the `model`, `load_model` and `loss_function` variables before starting the training. The trained network is saved to the `Network/modelMaster/` folder for each epoch of training.
Pretrained networks can be found in `Network/MasterNet2x2/` and `Network/MasterNet4x4/`.

## Network Conversion
This step converts the PyTorch model to a file that is easier read in C++. This is done in the `Network.py`-file by executing the `utils.SaveModelWeights` function after the trained model is loaded. This saves the network as a .bin file to `Network/nn_weights.bin`.

## Network Loading
The last step is to make sure the right model is loaded when the `Anti-Aliasing/main.cpp`-file is ran. This is done by changing the file path for network loading. The variable containing the file path is located in `Rendering/deep_learning/master_net.cpp` and is named `weight_path`.
When this variable is set correctly, and the upsampling factor in `Anti-Aliasing/app.cpp` is set correctly. The network can be ran in real-time by starting the `Anti-Aliasing`-project and pressing `7` on the keyboard.
