#!/bin/bash

mogrify \
  -trim +repage \
  -background none \
  -gravity north \
  -splice 0x20 \
  -gravity south \
  -splice 0x20 \
  -gravity west \
  -splice 20x0 \
  -gravity center \
  -resize 128x128 \
  -extent 128x128 \
  *.png
