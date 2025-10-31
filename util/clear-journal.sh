#!/usr/bin/env bash

set -x

sudo journalctl --rotate
sudo journalctl --vacuum-time=1s
sudo systemctl restart systemd-journald
sudo journalctl --disk-usage
