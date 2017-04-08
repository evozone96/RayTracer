#!/bin/sh
g++ -O2 -g svdDynamic.c RayTracer.c utils.c -lm -o RayTracer -fopenmp -std=gnu++11

