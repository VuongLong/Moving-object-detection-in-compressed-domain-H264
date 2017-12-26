#pragma once

#include <math.h>
#include "global.h"

extern int bit[100][120];
extern int type[100][120];
extern double mv_x[272][480];
extern double mv_y[272][480];pe

extern void filter(int height, int width, int threshold);
extern void filter_threshold(int height, int width, int threshold);
extern void filter_spatial(int height, int width);
extern void filter_cadidate(int height, int width, int size_min);
extern int calculate_angle(int x, int y);
extern void segmentation(int py, int px, int height, int width, int size_min);
