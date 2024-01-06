# WDPM Multiple GPUs
- WDPM simulates how runoff water is distributed across the Canadian Prairies
- WDPM has been generalized to take advantage of multiple GPUs
- A speedup of 2.39 has been observed using 4 GPUs with a real dataset
- By using leading computing paradigms, WDPM can perform difficult simulations


## Folders and files  

**/** - contains files describing the program  

- README.md - this file  
- WDPM_Multiple_GPU_EMS.pdf - the manuscript  

**Code** - contains program source code files:

- wdpm_mulGPU.c - WDPM main line. Can be executed from the command line.
- runoff.cl - OpenCL kernel that does the water smoothing

**Data** - experimental data in the paper:

- the file names describe the experiment: the experiment group, module (add, substract, or drain), devices (GPU or CPU, and the number of devices), systems (the input files)


## Program manual

The WDPM requires OpenCL drivers in order to work properly. The program   

```
cd existing_repo
git remote add origin https://git.cs.usask.ca/numerical_simulations_lab/gpu/wdpm.git
git branch -M main
git push -uf origin main
```
