#!/bin/bash
watch -n 2 "aws ec2 describe-fpga-images --fpga-image-ids=$(cat *_afi_id.txt | awk 'NR==2{print $2}' | tr -cd 'a-z0-9-')"
