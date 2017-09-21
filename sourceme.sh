#!/bin/bash

SCRIPTDIR=`dirname "$(readlink -f "$BASH_SOURCE")"`
export PATH=$PATH:$SCRIPTDIR/bin:$SCRIPTDIR/tools/cb-testing:$SCRIPTDIR/tools/service-launcher:$SCRIPTDIR/tools/poll-generator
export PYTHONPATH=$PYTHONPATH:$SCRIPTDIR/tools/cb-testing/lib
