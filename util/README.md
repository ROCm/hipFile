# [INTERNAL] AIS Development Utilities

## clear-journal.sh

Clears systemd's journal.

### History

## rsync_amdgpu_dkms.sh

Helper script used for syncing changes from the AMDGPU-DKMS source code.

Requirements:
- AMDGPU-DKMS already installed on the target system.
- sudo privileges on target system.
- Copy of brahma/ec/linux Gerrit repository for tracking changes.
    - Use git branch `amd-staging-dkms-#.#` or `amd-mainline-dkms-#.#`

### Usage
```
rsync_amdgpu_dkms.sh target_machine [amdgpu_src_path]

target-machine         An SSH-styled address URI (e.g. [USERNAME@]HOSTNAME[:PORT]).
amdgpu_src_path        Path of the amdgpu-dkms source code.
                       If not specified, will attempt to use a default path from 
                       DEFAULT_AMDGPU_SRC_PATH.
```
