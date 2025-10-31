# GPU Direct Storage on Bare Metal

## Requirements

### Shared Requirements
**Tested only on Ubuntu 20.04 so far.**
* Set IOMMU to Passthrough mode.
    * Edit */etc/default/grub* and add `iommu=pt` to the `GRUB_CMDLINE_LINUX_DEFAULT` setting.
    * Remember to update GRUB and then reboot the system.
    * You may also need to set `amd_iommu`/`intel_iommu` to `off`.

### Supported Local Filesystems
| Filesystem Name | Specific Configuration |
| --- | --- |
| EXT4 | Mounted with the `data=ordered` option. |
| XFS | |

Other filesystems may work, but have not been tested. These two filesystems
are the only ones officially supported by NVIDIA.

### Supported Remote Filesystems
TBD - Yet to begin development on Remote Filesystem PoC.

### Required Packages
| Package Name | Supported Versions | Install Instructions |
| --- | --- | --- |
| MLNX_OFED | >= 24.04-0.6.6 | Follow steps below. |
| clang | >= 1:10.0.0 |   |
| libclang-dev | >= 1:10.0.0 |   |
| llvm-dev | >= 1:10.0.0 |   |

### Nvidia Specific Requirements
| Package Name | Supported Versions | Install Instructions |
| --- | --- | --- |
| NVIDIA Graphics Driver | >= 555 | Open-Source Driver Required. Follow steps below. |
| CUDA | >= 12.5 | Follow steps below. |
| libcufile | >= 1.10 | Some package managers name libcufile with the CUDA version (e.g. libcufile-12-5) |

* Nvidia GPU capable of running GDS.
    * CUDA Compatibility Score of >= 6.0.
    * Workstation/Data Center GPU. Consumer GPUs are not supported.
* *[Optional]* Disable PCIe Access Control Services (ACS) in BIOS settings.
    * Best performance is achieved through disabling this setting.

### AMD Specific Requirements
| Package Name | Supported Versions | Install Instructions |
| --- | --- | --- |
| AMDGPU-DKMS | >= 6.7.0 | Follow steps below. |
| ROCm/HIP | >= 6.1.2 | Follow steps below. |

* AMD GPU capable of running ROCs.
    * Unknown hardware requirements at this time. Currently AMD consumer cards OK.
* Enable Resizeable BAR in BIOS settings.
    * Tested only on AMD CPUs.
    * Assumes "Above 4G Decoding" is also Enabled.
    * Also known as Large BAR or AMD Smart Access Memory (SAM).

## Installing Dependencies

Some of these dependencies are not straightforward to install. The steps here
are included to help accelerate setting up ROCs, but please use the official
documentation if you encounter any issues.

### MLNX_OFED
**Needed for both AMD and NVIDIA.**
> [!CAUTION]
> Secure Boot Enabled Systems\
> When installing MLNX_OFED, you must ensure:\
>     1. The MLNX_OFED kernel modules are signed AND the used signature is stored in the system Secure Boot keyring;\
>     2. OR disabling Secure Boot.\
> Failure to do so will result in your system FAILING TO BOOT if your boot/root partition is on an NVMe drive.

MLNX_OFED is required to drive NVFS when performing GPU Direct IO on a local
NVMe driver or NVMeoF.

1. Download the [MLNX_OFED package](https://network.nvidia.com/products/infiniband-drivers/linux/mlnx_ofed/)
    and extract the files.
    ```bash
    $ sudo tar -xzf MLNX_OFED_LINUX-24.04-0.6.6.0-ubuntu20.04-x86_64.tgz -C /usr/local/etc/
    ```
1. Setup the local MLNX_OFED repository.
    ```bash
    $ sudo wget -qO /etc/apt/trusted.gpg.d/mlnx_drivers.asc http://www.mellanox.com/downloads/ofed/RPM-GPG-KEY-Mellanox

    # Register the local MLNX_OFED repository in the package manager by creating a new file under */etc/apt/sources.list.d* with the following contents.

    $ sudo tee /etc/apt/sources.list.d/mlnx_ofed.list << EOF
    # For installing the MLNX_OFED driver to enable GPU DirectStorage for NVMe.
    deb file:/usr/local/etc/MLNX_OFED_LINUX-24.04-0.6.6.0-ubuntu20.04-x86_64/DEBS ./
    EOF

    $ sudo apt update
    ```

1. Run the MLNX_OFED installer directly.\
    We require this for a fine-tuned install with specific options.
    ```bash
    $ cd /usr/local/etc/MLNX_OFED_LINUX-24.04-0.6.6.0-ubuntu20.04-x86_64
    $ sudo ./mlnxofedinstall -vvv --with-nvmf --with-nfsrdma --enable-gds --add-kernel-support --dkms
    ```

1. If installing on a computer with Secure Boot enabled, check that the
   kernel modules are signed and that signature is present in the Secure Boot
   keyring.\
   Example for Ubuntu:
   ```bash
   $ modinfo ./mlx_compat.ko.zst
     ...
     signer:         rildixon-wrkstn Secure Boot Module Signature key
     sig_key:        54:95:B0:05:38:30:69:37:EC:98:12:15:6F:F2:2E:3F:C2:80:19:6E
     ...
   
   $ sudo mokutil --list-enrolled
     ...
     [key N]
     SHA1 Fingerprint: f2:6a:1a:c7:00:43:61:46:cf:ad:0a:1f:0c:19:d4:8e:a2:d0:1d:d9
     ...
     Issuer: CN=rildixon-wrkstn Secure Boot Module Signature key
     ...
   ```
   If a matching signature is not enrolled, look up your OS distro for how to 
   sign kernel modules/enroll a signing key into the Secure Boot keyring.
   *If your distro supports DKMS, note that DKMS does have some built-in*
   *signing capabilities.*

   If you are on a system that installs kernel modules directly, you may need to
   [enroll the Mellanox signing key](https://docs.nvidia.com/networking/display/mlnxofedv24070610/uefi+secure+boot)
   into the Secure Boot keyring.

1. Update the installed kernel module list and reboot the system.
    ```bash
    $ sudo update-initramfs -u -k `uname -r`
    $ sudo reboot
    ```

### NVIDIA Open Source Graphics Driver
**Needed for both AMD and NVIDIA.**

> [!WARNING]
> As of 10/15/2024, we have a hard dependency on the Nvidia driver being installed and loaded.
> Unfortunately, to load the Nvidia driver, a Nvidia GPU must be physically present.

If you current have the proprietary NVIDIA graphics driver installed, it
needs to be removed prior to installing the open source driver.

To check which is installed, run `modinfo nvidia | grep license` and compare
the license.
* NIVIDA == Proprietary
* Dual MIT/GPL == Open Source

1. Install the CUDA Repository setup script.
    ```bash
    $ wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.1-1_all.deb
    $ sudo dpkg -i cuda-keyring_1.1-1_all.deb
    $ sudo apt update
    ```
1. Install the open source NVIDIA graphics driver.
    ```bash
    $ sudo apt install nvidia-driver-555-open
    ```
1. Install the CUDA driver of the same version.\
    **This step must be completed separately from installing the graphics driver.**
    ```bash
    $ sudo apt install cuda-drivers-555
    ```

### CUDA
**NVIDIA Only**
1. If not already done so, install the CUDA repository setup script.
    ```bash
    $ wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.1-1_all.deb
    $ sudo dpkg -i cuda-keyring_1.1-1_all.deb
    $ sudo apt update
    ```
1. Install the CUDA Toolkit.
    ```bash
    $ sudo apt install cuda-toolkit-12-5
    ```
1. Reboot the system.

### Legacy NVFS Install
**This is included for posterity. You should probably be following the ROCs-NVFS steps.**\
**This only supports NVIDIA devices.**
1. Install GDS with the same version as CUDA.
    ```bash
    $ sudo apt install nvidia-gds-12-5
    ```
    This will install both libcufile, and the NVFS kernel module.
1. Reboot the system.
1. Verify that GDS has been properly setup by running the `gdscheck.py` utility.
    ```bash
    $ /usr/local/cuda/gds/tools/gdscheck.py -p
    ```

### AMDGPU-DKMS Driver
**AMD Only**\
These steps use a version of the AMDGPU-DKMS driver that have been published to
a package repository.

1. Install the Radeon Repository setup script.
    ```bash
    $ AMDGPU_INSTALL=$(curl -s https://repo.radeon.com/amdgpu-install/latest/ubuntu/$(lsb_release -cs)/ | grep -oP '(?<=href=")[^"]+' | grep amdgpu-install)
    $ wget https://repo.radeon.com/amdgpu-install/latest/ubuntu/$(lsb_release -cs)/$AMDGPU_INSTALL
    $ sudo apt install ./$AMDGPU_INSTALL
    ```
1. Install the AMDGPU-DKMS package.
    ```bash
    $ sudo apt update
    $ sudo apt install amdgpu-dkms
    ```
1. Add yourself to the Video & Render groups.
    ```bash
    $ groups
    rildixon adm cdrom sudo dip video plugdev lxd render

    $ sudo usermod -a -G video,render $USER
    ```
1. Reboot the system.

### HIP
**AMD Only**
> [!IMPORTANT]
> These steps assume you have completed the Radeon Repository setup from the previous step.

[HIP Installation Reference](https://rocm.docs.amd.com/projects/HIP/en/latest/install/install.html)

1. Install the AMD HIP package.\
   (DEBIAN_FRONTEND is to automate the install of the "tzdata" dependency.)
    ```bash
    $ sudo DEBIAN_FRONTEND=noninteractive apt install hip-dev rocm-device-libs
    ```

> [!NOTE]
> While HIP_PLATFORM=nvidia is required to be defined when compiling for Nvidia, there is no
> apparent requirement on needing HIP installed to compile ROCs on Nvidia (as of 15/10/2024).

## Installing ROCs

> [!IMPORTANT]
> All steps here assume that the ROCs code repository have been copied over to
the target system.
