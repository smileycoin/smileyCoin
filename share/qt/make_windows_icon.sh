#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/auroracoin.png
ICON_DST=../../src/qt/res/icons/auroracoin.ico
convert ${ICON_SRC} -resize 16x16 auroracoin-16.png
convert ${ICON_SRC} -resize 32x32 auroracoin-32.png
convert ${ICON_SRC} -resize 48x48 auroracoin-48.png
convert auroracoin-16.png auroracoin-32.png auroracoin-48.png ${ICON_DST}

