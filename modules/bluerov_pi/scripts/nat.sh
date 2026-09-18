#!/bin/bash

# configure the network to point to the topside device for internet access
sudo nmcli device modify eth0 ipv4.addresses 192.168.2.2/24 ipv4.gateway 192.168.2.1 ipv4.dns 8.8.8.8 ipv4.method manual
