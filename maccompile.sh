#!/bin/sh
/usr/local/bin/g++-6 -O4 -g svdDynamic.c RayTracer.c utils.c -lm -o RayTracer -fopenmp -std=gnu++11

