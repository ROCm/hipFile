# NVIDIA's CUDA and GPU Direct Storage Install

## Overview

This document gives a step-by-step approach to install NVIDIA's CUDA
with GPU Direct Storage (GDS) in a way that minimizes alterations to
the Linux kernel. To this end we use the [NVIDIA open
source][ref-nvos] linux kernel drivers and the [source
code][ref-nvidia-fs] for the nvidia-fs kernel module.

Note that this document was last updated in September 2025 and
pertains to the 580.65.06 release. This is important to the specifics
of some of the steps below. Also note that this document focuses on
the [p2pdma][ref-gds-p2pdma] approach to GDS and not the older
[nvidia-fs][ref-nvidia-fs] method. Where necessary this is pointed out
in the analysis below.

Note that additional NVIDIA drivers to support `nvidia-fs` mode. You
need to install `mlnx-nvme-dkms` for NVMe and NVMe over Fabrics
support, and `mlnx-nfsrdma-dkms` for NFS over RDMA support. These dkms
packages alter core components of the Linux kernel which is often
problematic for end users.

## Step-by-Step Instructions

Start from a base Ubuntu 24.04 Server cloud image. These instructions
work for both bare-metal and a VM. Note that the easiest way to
generate the initial disk image is to use [this qemu-helper
repo][ref-qemu-minimal] with this command:
```
VM_NAME=batesste-cuda-vm FORCE=true USERNAME=cufile PASS=cufile ./gen-vm
```
Then once the VM is generated you can run it and pass in any NVIDIA
GPU that is GDS compatible using something like this (note
`PCI_HOSTDEV` should be altered to match the target GPU in your
system):
```
VM_NAME=batesste-cuda-vm PCI_HOSTDEV=82:0.0 VCPUS=8 VMEM=8192 NVME=1 ./run-vm
```
Now you can ssh into the VM from the host machine and perform the
following steps.

### Install NVIDIA Open-Source Drivers

These open-source drivers are needed to avoid installing binary
modules into the kernel. Note that the commit SHA used here would need
to change if we plan to install a different version of the CUDA
runtime.

  1. `cd ~/ && git clone https://github.com/NVIDIA/open-gpu-kernel-modules.git`
  1. `cd open-gpu-modules`
  1. `git checkout -b use-this 307159f262`
  1. `make modules -j $(nproc)`
  1. `sudo make modules_install`
  1. `sudo depmod -a`

Note that this only installs these modules for the currently running
Linux kernel. If Ubuntu updates the kernel then these steps need to be
redone.

## Install NVIDIA nvidia-fs Driver

Note that this section is optional and is not required if you only
want to use the p2pdma mode of GDS. Also note the commit SHA used here
would need to potentially change to work with another version of CUDA
or the open-source drivers.

  1. `cd ~/ && git clone https://github.com/NVIDIA/gds-nvidia-fs.git`
  1. `cd gds-nvidia-fs`
  1. `git checkout -b use-this a5adb64443`
  1. `cd gds-nvidia-fs/src`
  1. `NVIDIA_SRC_DIR=/home/cufile/open-gpu-kernel-modules/kernel-open/nvidia/ make -j $(nproc)`
  1. `sudo make install`
  1. `depmod -a`

Note that this only installs this module for the currently running
Linux kernel. If Ubuntu updates the kernel then these steps need to be
redone.

### Install NVIDIA CUDA for Open-Source Drivers

The standard packaged versions of CUDA are not compatible with the
NVIDIA open-source drivers. So we need to download and use the `.run`
installer.

  1. `cd ~/ && wget https://developer.download.nvidia.com/compute/cuda/13.0.0/local_installers/cuda_13.0.0_580.65.06_linux.run`
  1. `sudo sh cuda_13.0.0_580.65.06_linux.run --silent --- --no-kernel-modules`

Note that it *might* be possible to build the open source kernel
modules at a specific label that does enable userspace tooling to be
installed via `apt`. However this is not something that we found easy
to get working. Some code that might be close to working is:

  1. `curl https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb -o cuda-repo-install.deb`
  1. `sudo apt install ./cuda-repo-install.deb`
  1. `sudo apt update && apt install -y cuda-minimal-build libcufile-dev libnvidia-compute gds-tools nvidia-firmware`

### Update .bashrc for CUDA Tools and Libraries

The following commands add the relevant directories to the `PATH` and
`LD_LIBRARY_PATH` environment variables. This is useful for running
tools and compiling code.

  1. `echo -e '#Added for CUDA\nexport PATH=${PATH}:/usr/local/cuda/bin\nexport LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/cuda/lib64' >> .bashrc`
  1. `echo -e '# Add for GDS\nexport PATH=${PATH}:/usr/local/cuda/gds/tools\nexport LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/cuda/targets/x86_64-linux/lib' >> .bashrc`
  1. `source .bashrc`
  1. `echo 'options nvidia NVreg_RegistryDwords="RMForceStaticBar1=1;ForceP2P=0;RmForceDisableIomapWC=1;"' | sudo tee -a /etc/modprobe.d/nvidia-p2pmem.conf`
  1. ``sudo update-initramfs -u -k `uname -r` ``
  1. `sudo reboot now`

## Testing Install

Note the use of `CUFILE_USE_PCIP2PDMA` in the command below. It is
also possible to specify this via a [cufile.json][ref-cufile-json]
configuration file.
```
$ CUFILE_USE_PCIP2PDMA=true /usr/local/cuda/gds/tools/gdscheck.py -p
 GDS release version: 1.15.0.42
 libcufile version: 2.12
 Platform: x86_64
 ============
 ENVIRONMENT:
 ============
 =====================
 DRIVER CONFIGURATION:
 =====================
 NVMe P2PDMA        : Supported
 NVMe               : Unsupported
 NVMeOF             : Unsupported
 SCSI               : Unsupported
 ScaleFlux CSD      : Unsupported
 NVMesh             : Unsupported
 DDN EXAScaler      : Unsupported
 IBM Spectrum Scale : Unsupported
 NFS                : Unsupported
 BeeGFS             : Unsupported
 ScaTeFS            : Unsupported
 WekaFS             : Unsupported
 Userspace RDMA     : Unsupported
 --Mellanox PeerDirect : Disabled
 --rdma library        : Not Loaded (libcufile_rdma.so)
 --rdma devices        : Not configured
 --rdma_device_status  : Up: 0 Down: 0
 =====================
 CUFILE CONFIGURATION:
 =====================
 properties.use_pci_p2pdma : true
 properties.use_compat_mode : true
 properties.force_compat_mode : false
 properties.gds_rdma_write_support : true
 properties.use_poll_mode : false
 properties.poll_mode_max_size_kb : 4
 properties.max_batch_io_size : 128
 properties.max_batch_io_timeout_msecs : 5
 properties.max_direct_io_size_kb : 1024
 properties.max_device_cache_size_kb : 131072
 properties.per_buffer_cache_size_kb : 1024
 properties.max_device_pinned_mem_size_kb : 18014398509481980
 properties.posix_pool_slab_size_kb : 4 1024 16384
 properties.posix_pool_slab_count : 128 64 64
 properties.rdma_peer_affinity_policy : RoundRobin
 properties.rdma_dynamic_routing : 0
 fs.generic.posix_unaligned_writes : false
 fs.lustre.posix_gds_min_kb: 0
 fs.beegfs.posix_gds_min_kb: 0
 fs.scatefs.posix_gds_min_kb: 0
 fs.weka.rdma_write_support: false
 fs.gpfs.gds_write_support: false
 fs.gpfs.gds_async_support: true
 profile.nvtx : false
 profile.cufile_stats : 0
 miscellaneous.api_check_aggressive : false
 execution.max_io_threads : 0
 execution.max_io_queue_depth : 128
 execution.parallel_io : false
 execution.min_io_threshold_size_kb : 1024
 execution.max_request_parallelism : 0
 properties.force_odirect_mode : false
 properties.prefer_iouring : false
 =========
 GPU INFO:
 =========
 GPU index 0 NVIDIA L40S bar:1 bar size (MiB):65536 supports GDS, IOMMU State: Disabled
 ==============
 PLATFORM INFO:
 ==============
 IOMMU: disabled
 Nvidia Driver Info Status: Supported(Nvidia Open Driver Installed)
 Cuda Driver Version Installed:  13000
 Platform: Standard PC (Q35 + ICH9, 2009), Arch: x86_64(Linux 6.8.0-79-generic)
 Platform verification succeeded
```
## Testing Operation

You can now use the `gdsio` tool to test if the IO path is
operational. In this specific case we use the emulated NVMe SSD
created by the call to ./run-vm.
```
$ sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH PATH=$PATH \
  CUFILE_USE_PCIP2PDMA=true gdsio -f /dev/nvme0n1 \
  -d 0 -m 0 -s 1G -i 64k -x 0 -I 2 -T 30  -a 64k
IoType: RANDREAD XferType: GPUD Threads: 1 DataSetSize: 955328/1048576(KiB)
IOSize: 64(KiB) Throughput: 0.030136 GiB/sec, Avg_Latency: 2025.139948 usecs
ops: 14927 total_time 30.232086 secs
```

[ref-gds-p2pdma]: https://docs.nvidia.com/gpudirect-storage/overview-guide/index.html#primary-components
[ref-gds-nvidia-fs]: https://docs.nvidia.com/gpudirect-storage/overview-guide/index.html#primary-components
[ref-nvos]: https://github.com/NVIDIA/open-gpu-kernel-modules
[ref-nvidia-fs]: https://github.com/NVIDIA/gds-nvidia-fs
[ref-qemu-minimal]: https://github.com/sbates130272/qemu-minimal
[ref-cufile-json]: https://docs.nvidia.com/gpudirect-storage/configuration-guide/index.html
