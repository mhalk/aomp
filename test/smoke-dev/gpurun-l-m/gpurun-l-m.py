# Python program to check the gpurun -m and -l options.

import subprocess
from subprocess import run
import re
import shutil
import os

def get_gpu_info():
    # Run the rocminfo command
    result = run(['rocminfo'], capture_output=True, text=True, check=True)

    # Initialize lists to store GPU information
    device_count = 0
    gpu_types = []
    gpu_memory = []

    # Split the output into lines
    lines = result.stdout.splitlines()

    # Iterate through the lines to gather information
    for i, line in enumerate(lines):

        # Example:
        #  Marketing Name:          AMD EPYC 9454 48-Core Processor
        #  Marketing Name:          AMD Instinct MI300X
        m = re.match("\s+Marketing Name:\s*(.*)", line)
        if m:
            marketingName = m.group(1)
            gpu_types.append(marketingName.strip())
            device_count += 1

        # Check for "Pool 1" line to find the "Size" line two lines below
        if "Pool 1" in line:
            # print (lines[i+2])
            # Example:
            #  Size:        201310208(0xbffc000) KB
            m = re.match("\s+Size:\s*(\d+)\(.*\)\s+KB", lines[i+2])
            if m:
                size = int(m.group(1))
                gpu_memory.append(int(size))

    return device_count, gpu_types, gpu_memory

#
# Main program.
#

# os.environ['LIBOMPTARGET_KERNEL_TRACE'] = '1'

command_path = shutil.which("numactl")
if command_path is None:
    print("Utility: numactl is not found in the PATH. Exiting.")
    exit(-1)

command_path = shutil.which("rocminfo")
if command_path is None:
    print("Utility: rocminfo is not found in the PATH. Exiting.")
    exit(-2)

device_count, gpu_types, gpu_memory = get_gpu_info()
print(f"Number of Devices: {device_count}")

print ("device_count = ", device_count)
print ("gpu_types = ", gpu_types)
print ("gpu_memory = ", gpu_memory)
for i in range(device_count):
    print(f"Device {i + 1}: Type: {gpu_types[i]}, Memory: {gpu_memory[i]} KB")

for index, element in enumerate(gpu_types):
    if "Instinct" in element:
        break

totalMemory = gpu_memory[index] * 1024 # Convert from Kilobytes to Bytes.
trialMem  = int(totalMemory * .9)      # Start the trial at 90%.
print (f"totalMemory = {totalMemory:,} trialMem = {trialMem:,}")

#
# Test gpurun with the -m and -l options with HSA_XNACK=1/0
# 

os.environ['HSA_XNACK'] = '0'
print(f"Trying {trialMem:,} bytes.")
cmd = f"gpurun -m -v ./gpurun-l-m {trialMem}"
try:
    result = run ([cmd], shell=True)
except:
    print (result.returncode)
    exit (-1) 
print (result.returncode)

print(f"Trying {trialMem:,} bytes.")
cmd = f"gpurun -l -v ./gpurun-l-m {trialMem}"
try:
    result = run ([cmd], shell=True)
except:
    print (result.returncode)
    exit (-2) 
print (result.returncode)

os.environ['HSA_XNACK'] = '1'
print(f"Trying {trialMem:,} bytes.")
cmd = f"gpurun -m -v ./gpurun-l-m {trialMem}"
try:
    result = run ([cmd], shell=True)
except:
    print (result.returncode)
    exit (-3) 
print (result.returncode)

print(f"Trying {trialMem:,} bytes.")
cmd = f"gpurun -l -v ./gpurun-l-m {trialMem}"
try:
    result = run ([cmd], shell=True)
except:
    print (result.returncode)
    exit (-4) 
print (result.returncode)
