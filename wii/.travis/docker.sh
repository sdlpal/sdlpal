#!/bin/bash -ex
source /etc/profile.d/devkit-env.sh
export PATH=$DEVKITPPC/bin:$PATH
make -C /src/wii