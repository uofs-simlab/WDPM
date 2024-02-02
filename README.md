# The multi-GPU Wetland DEM Ponding Model (WDPM) [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10609000.svg)](https://doi.org/10.5281/zenodo.10609000)

## Background
- WDPM simulates how runoff water is distributed across the Canadian Prairies
- WDPM has been generalized to take advantage of multiple GPUs
- A speedup of 2.39 has been observed using 4 GPUs with a real dataset
- By using leading computing paradigms, WDPM can perform difficult simulations

## Folders and files  

**[/](/)** - Contains files describing the program  

- [README.md](/README.md) - this file  
- [WDPM_Multiple_GPU_EMS.pdf](/WDPM_Multiple_GPU_EMS.pdf) - the manuscript  

**[/Code](/Code)** - Contains program source code files:

- [wdpm_mulGPU.c](/Code/wdpm_mulGPU.c) - WDPM main line. Can be executed from the command line.
- [runoff.cl](/Code/runoff.cl) - OpenCL kernel that does the water smoothing

**[/Data](/Data)** - Experimental data in the paper:

- The file names describe the experiment: the experiment group, module (add, substract, or drain), devices (GPU or CPU, and the number of devices), and systems (the input files)

## Installation

### OpenCL configuration
The WDPM requires OpenCL drivers in order to work properly. To download and install OpenCL for different platform, please check the following list:  
- [Intel CPU](https://software.seek.intel.com/intel-opencl)
- [AMD CPU](https://www.amd.com/en/support)
- [NVIDIA GPU](https://www.nvidia.com/download/index.aspx?lang=en-us)
- [AMD GPU](https://www.amd.com/en/support)

### Compiling the code
The gcc compiler, version 11.4.0 or later, is recommended. Use the following command to compile the program:  

```
gcc wdpm_mulGPU.c -o WDPM -lOpenCL
```

### Running the code
The add, subtract, and drain modules take the following input arguments:

#### Add module
| Argument | Type | Notes |
| -------- | ---- | ----- |
| DEM file name | string | |  
| Water file name | string | Optional - use NULL to omit |  
| Output file name | string | | 
| Scratch file name | string | Optional - use NULL to omit | 
| Depth of water to add (mm) | real | |
| Water runoff fraction | real | |    
| Elevation tolerance (mm) | real | |  
| Parallel control | integer | Specify 0 for serial CPU and 1 for OpenCL |  
| OpenCL options | integer | Specify 0 for OpenCL GPU and 1 for OpenCL CPU | 
| Zero depth threshold (mm) | real | |  
| Maximum number of iterations | integer | Optional - use 0 to omit |  

#### Subtract module
| Argument | Type | Notes |
| -------- | ---- | ----- |
| DEM file name | string | |  
| Water file name | string | |  
| Output file name | string | |  
| Scratch file name | string | Optional - use NULL to omit |  
| Depth of water to remove (mm) | real | |  
| Elevation tolerance (mm) | real | |  
| Parallel control | integer | Specify 0 for serial CPU and 1 for OpenCL | 
| OpenCL options | integer | Specify 0 for OpenCL GPU and 1 for OpenCL CPU | 
| Zero depth threshold (mm) | real | |  
| Maximum number of iterations | integer | Optional - use 0 to omit |  

#### Drain module
| Argument | Type | Notes |
| -------- | ---- | ----- |
| DEM file name | string | |  
| Water file name | string | Optional - use NULL to omit |
| Output file name | string | |  
| Scratch file name | string | Optional - use NULL to omit |  
| Elevation tolerance (mm) | real | |  
| Drain tolerance (mm) | real | |  
| Parallel control | integer | Specify 0 for serial CPU and 1 for OpenCL |
| OpenCL options | integer | Specify 0 for OpenCL GPU and 1 for OpenCL CPU | 
| Zero depth threshold (mm) | real | |  
| Maximum number of iterations | integer | Optional - use 0 to omit | 

#### Sample run using the add module
```
./WDPM add basin5.asc NULL water.asc NULL 10.0 1.0 1.0 1 0 0.005 0
```
