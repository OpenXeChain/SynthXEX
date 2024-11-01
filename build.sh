#!/bin/bash

# TEMP BUILD SCRIPT

gcc src/common/common.c src/getdata/getdata.c src/setdata/optheaders.c src/setdata/populateheaders.c src/setdata/pagedescriptors.c src/placer/placer.c src/write/writexex.c src/write/headerhash.c src/main.c -o synthxex -lnettle -Wno-multichar -g -fsanitize=address
