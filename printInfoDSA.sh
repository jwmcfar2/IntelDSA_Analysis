#!/bin/bash

lspci | grep 0b25 | awk '{print $1}' | xargs -I {} lspci -vvv -s {}