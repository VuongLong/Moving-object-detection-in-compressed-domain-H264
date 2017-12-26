#include "HMI.h"

int dir_x[8] = { -1,0,1,1,1,0,-1,-1 };
int dir_y[8] = { -1,-1,-1,0,1,1,1,0 };
int qx[1000000];
int qy[1000000];
int mark[272][480];


void filter(int height, int width, int threshold) {
	filter_threshold(height, width, threshold);
	filter_spatial(height, width);
	filter_cadidate(height, width, 0);
}

void filter_threshold(int height, int width, int threshold) {
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {

			if (bit[i][j] < threshold) bit[i][j] = 0;

			if (mb_type[i][j] == 1) {
				int count = 0;
				if ((i - 1 >= 0) && (bit[i - 1][j] > 0)) count++;
				if ((j - 1 >= 0) && (bit[i][j - 1] > 0)) count++;
				if ((i - 1 >= 0) && (j + 1 < width) && (bit[i - 1][j + 1] > 0)) count++;
				if (count == 3) {
					bit[i][j] = round((bit[i - 1][j] + bit[i][j - 1] + bit[i - 1][j + 1]) / 3);
				}
			}
		}
}

//if (#(FG neighbours)>4 => FG
void filter_spatial(int height, int width) {
	int y[8] = { -1,-1,-1,0,0,1,1,1 };
	int x[8] = { -1,0,1,-1,1,-1,0,1 };

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			if (bit[i][j] == 0) {
				int count = 0;
				int amount = 0;
				for (int k = 0; k < 8; k++) {
					int yy = i + y[k];
					int xx = j + x[k];
					if ((xx >= 0) && (xx < width) && (yy >= 0) && (yy < height) && (bit[yy][xx] > 0)) {
						count++;
						amount += bit[yy][xx];
					}
				}
				if (count > 4) bit[i][j] = round(amount / count);
			}
		}
}

void filter_cadidate(int height, int width, int size_min) {
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) mark[i][j] = 0;

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			if (mark[i][j] == 0 && bit[i][j]>0) segmentation(i, j, height, width, size_min);
	
}

void segmentation(int py, int px, int height, int width, int size_min) {
	int dau = 1;
	int cuoi = 1;
	qx[1] = px;
	qy[1] = py;
	mark[py][px] = 1;
	/////////////////
	int sa[9] = { 0,0,0,0,0,0,0,0,0 };
	float area = 0;
	float perimeter = 0;
	float length = 0;
	/////////////////
	while (dau <= cuoi) {
		int x = qx[dau];
		int y = qy[dau];

		/*
		extract feature for cadidate
		*/
		area++;
		// co 1 phia == 0 thi tang chu vi len
		int ok = 0;
		for (int uv = 0; uv < 8; uv++) {
			int xxx = x + dir_x[uv];
			int yyy = x + dir_y[uv];
			if (xxx >= 0 && yyy >= 0 && xxx < width && yyy < height && bit[yyy][xxx] == 0)
				ok = 1;
		}
		perimeter += ok;
		/////////////////////
		double le = 0;
		for (int u = 0; u<4; u++)
			for (int v = 0; v < 4; v++) {
				sa[calculate_angle(mv_x[y *4+ u][x*4 + v], mv_y[y *4+ u][x*4 + v])]++;
				le += mv_x[y *4+ u][x*4 + v]* mv_x[y *4+ u][x*4 + v] * mv_y[y *4+ u][x*4 + v] * mv_y[y *4+ u][x*4 + v];
			}
		length += trunc(sqrt(le / 16));
		dau++;
		for (int i = 0; i < 8; i++) {
			int xx = x + dir_x[i];
			int yy = y + dir_y[i];
			if (xx >= 0 && yy >= 0 && xx < width && yy < height && mark[yy][xx] == 0 && bit[yy][xx]>0) {
				mark[yy][xx] = 1;
				cuoi++;
				qx[cuoi] = xx;
				qy[cuoi] = yy;
			}
		}
	}

	// check condition
	int size = 1;
	int pattern = 1;
	int density = 1;
	length /= area;
	// check size
	if (size_min > 0 && cuoi < size_min) size = 0;
	
	// check pattern
	int max1, max2;
	if (sa[1] > sa[2]) {
		max1 = 1;
		max2 = 2;
	}
	else {
		max1 = 2;
		max2 = 1;
	}
	for (int i=3;i<9;i++)
		if (sa[i] > sa[max1]) {
			max2 = max1;
			max1 = i;
		}
		else if (sa[i] > sa[max2]) max2 = i;
	float Dominant = sa[max1];
	if (Dominant / area < 2.8) pattern = 0;
	////////////////

	// check density
	if (perimeter >= area) density = 0;

	//result
	if (size==0) 
		for (int i = 1; i <= cuoi; i++) bit[qy[i]][qx[i]] = 0;
	else {
		if (pattern == 1) {
			dau = 1;
			/////////////////
			while (dau <= cuoi) {
				int x = qx[dau];
				int y = qy[dau];
				dau++;
				for (int i = 0; i < 8; i++) {
					int xx = x + dir_x[i];
					int yy = y + dir_y[i];
					if (xx >= 0 && yy >= 0 && xx < width && yy < height && mark[yy][xx] == 0) {
						double le = 0;
						int okkk = 16;
						for (int u = 0; u<4; u++)
							for (int v = 0; v < 4; v++) {
								int dir = calculate_angle(mv_x[yy*4 + u][xx*4+ v], mv_y[yy*4 + u][xx*4+ v]);
								//if ((float)sa[max1] / area > 2.4 && dir != max1) okkk--;
								//else if (dir != max1 && dir != max2) okkk--;
								if (dir != max1) okkk--;
								le += mv_x[yy*4 + u][xx*4+ v] * mv_x[yy*4 + u][xx*4+ v] * mv_y[yy*4 + u][xx*4+ v] * mv_y[yy*4 + u][xx*4+ v];
							}
						le = trunc(sqrt(le / 16));
						if (okkk > 0 && le > length*0.7 && le < length*1.3) {
							mark[yy][xx] = 1;
							cuoi++;
							qx[cuoi] = xx;
							qy[cuoi] = yy;
						}
					}
					
				}
			}
			/////////////////
			for (int i = 1; i <= cuoi; i++) bit[qy[i]][qx[i]] = 2;
		}		
		else if (density == 1 )
			for (int i = 1; i <= cuoi; i++) bit[qy[i]][qx[i]] = 1;
		else 
			for (int i = 1; i <= cuoi; i++) bit[qy[i]][qx[i]] = 0;
	}
}

int calculate_angle(int x, int y) {
	if (x == 0 && y == 0) return 0;
	if (x > 0) {
		if (y > x) return 2;
		else if (y > 0) return 1;
		else if (y > -x) return 8;
		else return 7;
	}
	else {
		if (y > -x) return 3;
		else if (y > 0) return 4;
		else if (y < x) return 6;
		else return 5;
	}
	return 0;
}