#! /usr/bin/env python3
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

"""
SystemD approach if wanting to have this execute on startup:

/etc/systemd/system/setup-nvmeof.service
**************************************************
[Unit]
Description=Configure an NVMeoF Target device.

[Service]
ExecStart=/usr/local/bin/setup-nvmeof.py <YOUR_ARGS_HERE>
Type=oneshot
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
**************************************************
Modify ExecStart as needed.

Then enable the service file in SystemD:
$ sudo systemctl daemon-reload && sudo systemctl enable setup-nvmeof.service

Check the status with
$ sudo systemctl status setup-nvmeof.service

Please note all networking configuration (including RXE if used)
needs to be done prior to running this utility. Otherwise it will
fail to start.

Nice to have enhancements:
* Automatically grab an IP Address to bind to.
* Automatically determine namespace-num if one is not provided.
  (If namespace N exists, generate namespace N+1.)
* Automatically determine port-num if one is not provided.
  (If port-num N exists, generate port-num N+1.)
"""
import argparse
import os
import sys
import subprocess

from logging import Formatter, StreamHandler, FileHandler, INFO, getLogger
from pathlib import Path

logger = getLogger(Path(__file__).stem)

# Types must be expressed in lowercase.
NVMeoF_TRANSPORT_TYPES = [
    "tcp",
    "rdma"
]

NVMET_BASE_PATH = Path("/sys/kernel/config/nvmet")
NVMET_SUBSYSTEM_PATH = NVMET_BASE_PATH / "subsystems"
NVMET_PORT_PATH = NVMET_BASE_PATH / "ports"


def setup_logging():
    """
    Configure a basic logger that prints to STDOUT & a log file.

    The log file used is kept in a standard location: /var/log/setup-nvmeof.log
    If the script is executed directly, the log messages will also be displayed
    to the terminal.
    """
    logger.setLevel(INFO)

    console_format = Formatter("%(levelname)s - %(message)s")
    console_handler = StreamHandler()
    console_handler.setFormatter(console_format)
    logger.addHandler(console_handler)

    log_file_path = Path("/var/log/setup-nvmeof.log")
    log_file_format = Formatter("%(asctime)s - %(levelname)s - %(message)s")
    try:
        if not log_file_path.exists():
            log_file_path.touch(mode=644)
        log_file_handler = FileHandler(log_file_path)
        log_file_handler.setFormatter(log_file_format)
        logger.addHandler(log_file_handler)
    except PermissionError:
        print("Unable to write to the NVMeoF-Setup log file.")
    except OSError:
        print("Unable to create NVMeoF-Setup log file.")


def load_kernel_module(kmod_name: str) -> None:
    """
    Loads a kernel module.

    Assumes that we cannot do anything more if we are unable to load
    the specified kernel module.

    If modprobe has already loaded in the module, modprobe
    becomes a no-op. Thus this is safe to perform without
    further checks in case the modules were loaded elsewhere.
    """
    logger.info(f"Loading kernel module '{kmod_name}'.")
    cmd = ["modprobe", kmod_name]
    result = subprocess.run(
        cmd,
        check=True
    )


def create_NVMet_subsystem(subsystem_name: str, force: bool) -> None:
    """
    Create an NVMet subsystem to encapsulate the target NVMe drives.
    """
    # Check if subsystem already exists.
    # If it already exists, emit a warning, but continue normally.
    subsystem_dir = NVMET_SUBSYSTEM_PATH / subsystem_name
    if subsystem_dir.exists():
        logger.warning(f"NVMet subsystem '{subsystem_name}' already exists.")
        if force:
            logger.warning("Modifying the existing NVMe subsystem.")
        else:
            logger.error("Aborting further NVMeoF setup.")
            sys.exit(1)
    else:
        logger.info(f"Making new subsystem '{subsystem_name}'.")
        os.mkdir(subsystem_dir, 755)

    # Set access to all hosts.
    with open(subsystem_dir / "attr_allow_any_host", "w") as dev_attr:
        logger.info("Enabling any host to connect to this NVMe Subsystem.")
        dev_attr.write("1")


def create_NVMet_namespace(
    subsystem_name: str,
    namespace_num: int,
    nvme_device: Path,
    force: bool
) -> None:
    """
    Create & Configure a NVMet namespace.

    If the namespace already exists within the subsystem, emit a warning but
    continue as normal.

    subsystem_name - Name for a new NVMe Subsystem.
    namespace_num - Namespace ID # associated under the NVMe Subsystem.
    nvme_device - Path to the NVMe device that will be exposed to the network.
                  (e.g. /dev/nvme0n1, /dev/disk/by-id/nvme-*)
                  Reminder that the /dev/nvme#n# path does not reliably point
                  to the same physical disk. This is generated at boot time
                  during device discovery. Consider using a difference
                  reference that is fixed (PCIe BDF, UUID, Disk ID/SN).
    force - Reconfigure an existing NVMe Nsamespace if it already exists.
    """
    namespace_dir = (
        NVMET_SUBSYSTEM_PATH / subsystem_name / "namespaces" /
        str(namespace_num)
    )
    if namespace_dir.exists():
        logger.warning(
            f"NVMet namespace '{subsystem_name}':'{namespace_num}' already "
            "exists. "
        )
        if force:
            logger.warning("Overwriting the existing NVMe Namespace.")
            with open(namespace_dir / "enable", "w") as dev_attr:
                logger.info(
                    "Disabling the old NVMet Namespace "
                    f"'{subsystem_name}':'{namespace_num}'."
                )
                dev_attr.write("0")
        else:
            logger.error(
                "Aborting further NVMeoF setup. Please be aware previous "
                "setup steps will not be reverted."
            )
            sys.exit(1)
    else:
        logger.info(
            f"Making new namespace '{subsystem_name}':'{namespace_num}'."
        )
        os.mkdir(namespace_dir, 755)

    with open(namespace_dir / "device_path", "w") as dev_attr:
        logger.info(f"Setting NVMe Target drive: {nvme_device}")
        dev_attr.write(str(nvme_device))
    with open(namespace_dir / "enable", "w") as dev_attr:
        logger.info(
            "Enabling the NVMet Namespace "
            f"'{subsystem_name}':'{namespace_num}'."
        )
        dev_attr.write("1")


def create_NVMet_port(
    nvme_port: int,
    net_port: int,
    address: str,
    transport_type: str,
    force: bool
) -> None:
    """
    Create & Configure a new NVMeoF Port.

    If the port already exists, emit a warning and modify the existing port.
    NOTE: That this is different behaviour from creating the NVMe subsystem.
          The port is easier to modify than the subsystem for **reasons**.

    nvme_port - Port number associated with the local NVMe controller.
    net_port - Network port the NVMet drive will listen on for connections.
    address - Address to broadcast the NVMet drive on.
              (Currently only supports IPv4)
    transport_type - Transport type used to establish NVMeoF communication.
    force - Reconfigure an existing NVMe Port if it already exists.
    """

    nvme_port_dir = NVMET_PORT_PATH / str(nvme_port)
    if nvme_port_dir.exists():
        logger.warning(
            f"NVMet Port # {nvme_port} already exists. "
            "Modifying the existing port."
        )
        if force:
            logger.warning("Overwriting the NVMe Port configuration.")
            logger.warning(
                "Disabling the NVMe Port by removing ALL NVMe subsystem "
                "symlinks."
            )
            _delete_NVMet_links(nvme_port_dir / "subsystems")
        else:
            logger.error(
                "Aborting further NVMeoF setup. Please be aware previous "
                "setup steps will not be reverted."
            )
            sys.exit(1)
    else:
        logger.info(f"Creating new NVMet port '{nvme_port}'.")
        os.mkdir(nvme_port_dir, 755)

    with open(nvme_port_dir / "addr_traddr", "w") as dev_attr:
        logger.info(f"Exposing the NVMe device on address '{address}'.")
        dev_attr.write(address)
    with open(nvme_port_dir / "addr_adrfam", "w") as dev_attr:
        # Currently fixed to IPv4. If we need to support other address types
        # we should redesign this script to be more extendible.
        logger.info(f"Setting Address Family as 'ipv4'.")
        dev_attr.write("ipv4")
    with open(nvme_port_dir / "addr_trtype", "w") as dev_attr:
        logger.info(f"Setting NVMeoF Transport method to '{transport_type}'.")
        dev_attr.write(transport_type)
    with open(nvme_port_dir / "addr_trsvcid", "w") as dev_attr:
        logger.info(
            f"Binding NVMe device to '{transport_type}' Port '{net_port}'."
        )
        dev_attr.write(str(net_port))


def _delete_NVMet_links(port_ns_links: Path) -> None:
    """
    Deletes all symlinks in a given directory.
    """
    ns_symlinks = [
        ns_symlink for ns_symlink in port_ns_links.iterdir()
        if ns_symlink.is_symlink()
    ]
    for symlink in ns_symlinks:
        logger.warning(
            f"Removing symlink {symlink} -> {symlink.resolve()}"
        )
        os.remove(symlink)


def create_NVMet_link(subsystem_name: str, nvme_port: int) -> None:
    """
    Add a symlink to connect the NVMeoF Port to the NVMe Subsystem.
    """
    subsystem_path = NVMET_SUBSYSTEM_PATH / subsystem_name
    port_subsystem_path = (
        NVMET_PORT_PATH / str(nvme_port) / "subsystems" / subsystem_name
    )
    logger.info(
        f"Creating symlink '{port_subsystem_path}' -> "
        f"'{subsystem_path}'."
    )
    try:
        os.symlink(
            src=subsystem_path,
            dst=port_subsystem_path
        )
    except OSError as exc:
        if "No such device" in exc.strerror:
            # E501 - line too long (80 char limit)
            logger.exception(
                f"\n\n{'*' * 40}\n"
                f"This error typically indicates that the setup script ran \n"
                f"successfully. However, a device that NVMeoF depends on is either \n"  # noqa: E501
                f"missing or mis-configured. For example: \n"
                f"    * The network device as specified by address is not ready.\n"  # noqa: E501
                f"    * If RDMA is specified, the network device is not RDMA capable.\n"  # noqa: E501
                f"    * If RDMA is specified, RXE may not be loaded & configured.\n"  # noqa: E501
                f"Start by verifying the devices NVMeoF depends on are active and \n"  # noqa: E501
                f"correctly configured.\n"
                f"{'*' * 40}\n\n"
            )
        raise


def check_syslog(nvme_port: int, address: str, net_port: int) -> None:
    """
    Checks the syslog for the kernel to emit a message.

    This is not a perfect check as an old message may be read that was not
    attributed to this exact script invocation.
    """
    result = subprocess.run(
        ["dmesg"],
        capture_output=True,
        encoding="utf-8"
    )
    # nvmet_rdma: enabling port 1 (192.168.50.67:4420)
    check_str = f"nvmet_rdma: enabling port {nvme_port} ({address}:{net_port})"
    if check_str in result.stdout:
        logger.info(f"NVMeoF Device Enabled message present in kernel logs.")
    else:
        logger.warning(
            "Unable to find NVMeoF Device enabled message in kernel logs.\n"
            "Please manually confirm that the NVMeoF device is enabled."
        )


parser = argparse.ArgumentParser(
    prog="NVMeoF_Target_Setup",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description=(
        "A helper script to automatically setup NVMeoF on a system.\n"
        "Example: # ./setup-nvmeof.py \\\n"
        "                -s ais-nvmeof -n 1 -d /dev/nvme0n1 \n"
        "                -P 1 -a 192.168.50.67 -p 4420 -t rdma \n\n\n"
    ),
    epilog=(
        "This script must be run as root."
    )
)
# Can we be intelligent enough to automatically grab a given IP address?
parser.add_argument(
    "-a",
    "--address",
    type=str,
    help=(
        "Address to broadcast this NVMeoF Target."
    ),
    required=True
)
parser.add_argument(
    "-d",
    "--device-path",
    type=Path,
    help=(
        "Path to the NVMe disk to use as the NVMeoF Target. "
        "(ex. /dev/nvme0n1)"
    ),
    required=True
)
parser.add_argument(
    "-n",
    "--namespace-num",
    type=int,
    default=1,
    help=(
        "Number to identify this namespace under the Subsystem. "
        "(Similar to LUN.) Defaults to 1."
    )
)
parser.add_argument(
    "-p",
    "--net-port",
    type=int,
    default=4420,
    help=(
        "Network port to bind this NVMeoF Target to. Defaults to 4420."
    ),
)
parser.add_argument(
    "-P",
    "--nvme-port",
    type=int,
    default=1,
    help=(
        "NVMe Controller port to bind this NVMeoF Target to. Defaults to 1."
    )
)
parser.add_argument(
    "-s",
    "--subsystem-name",
    type=str,
    help="NVMe Subsystem Name.",
    required=True
)
parser.add_argument(
    "-t",
    "--transport",
    type=lambda _str: _str.lower(),
    choices=NVMeoF_TRANSPORT_TYPES,
    help="Transport Type for NVMeoF communication.",
    required=True
)
parser.add_argument(
    "--force",
    action="store_true",
    help=(
        "Overwrite existing NVMe configurations, if possible. "
        "Configurations that cannot be edited or removed may cause the "
        "script to fail."
    )
)

if len(sys.argv) == 1:
    parser.print_help()
    sys.exit(1)
if os.getuid() != 0:
    logger.error("Script must be run as root.")
    sys.exit(1)

args = parser.parse_args()
setup_logging()

device_path: Path = args.device_path
if not device_path.exists():
    raise FileNotFoundError(
        f"'{device_path}' does not exist on this system."
    )
if not device_path.is_block_device():
    raise ValueError(
        f"'{device_path}' does not point to a Block Device or a symlink to a "
        "Block Device."
    )
# Could also perform other validation checks...

load_kernel_module("nvmet")
load_kernel_module("nvmet-rdma")
create_NVMet_subsystem(
    args.subsystem_name, args.force
)
create_NVMet_namespace(
    args.subsystem_name, args.namespace_num, device_path, args.force
)
create_NVMet_port(
    args.nvme_port, args.net_port, args.address, args.transport, args.force
)
create_NVMet_link(
    args.subsystem_name, args.nvme_port
)
check_syslog(
    args.nvme_port, args.address, args.net_port
)

logger.info(f"NVMeoF Setup complete.")
