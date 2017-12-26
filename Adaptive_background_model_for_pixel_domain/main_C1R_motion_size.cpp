/**
 * @file main-opencv.cpp
 * @date July 2014 
 * @brief An exemplative main file for the use of ViBe and OpenCV
 */
#include <iostream>
#include <fstream>
#include "opencv2/imgproc.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>


#include "vibe-background-sequential.h"
#include "MeanShift.h"


using namespace cv;
using namespace std;

/** Function Headers */
void processVideo(char* videoFilename);

/**
 * Displays instructions on how to use this program.
 */

void read_jm_res(int height, int width);
void preprocess(uint8_t *image_data, uint8_t *tmp, int height, int width);
void postprocess(uint8_t *image_data, uint8_t *tmp, int height, int width);
void filter(int height, int width, int size_min);
void filter_cadidate(int height, int width, int pSize_min);
int calculate_angle(int x, int y);
void segmentation(int py, int px, int height, int width, int pSize_min);


// long coding
const int max_width = 1000;
const int max_height = 1000;
ifstream motion_file;
ifstream bit_file;
int bit[max_height][max_width];
int res[max_height][max_width];
int type[max_height][max_width];
double mv_x[max_height][max_width];
double mv_y[max_height][max_width];
int qx[1000000];
int qy[1000000];
int mark[max_height][max_width];


int GOP=250;
double alpha = 0;
double beta = 1;
int size_min = 320;
int HWS = 2;
int maxMV = 0;
int maxBit = 0;
void help()
{
    cout
    << "--------------------------------------------------------------------------" << endl
    << "This program shows how to use ViBe with OpenCV                            " << endl
    << "Usage:"                                                                     << endl
    << "./main-opencv <video filename>"                                             << endl
    << "for example: ./main-opencv video.avi"                                       << endl
    << "--------------------------------------------------------------------------" << endl
    << endl;
}


/**
 * Main program. It shows how to use the grayscale version (C1R) and the RGB version (C1R). 
 */
int main(int argc, char* argv[])
{
  /* Print help information. */
  help();

  /* Check for the input parameter correctness. */
  if (argc < 4) {
    cerr <<"Incorrect input" << endl;
    cerr <<"exiting..." << endl;
    return EXIT_FAILURE;
  }
  cout << argc << "\n";
  /* Create GUI windows. */
  namedWindow("Frame");
  namedWindow("Segmentation");
  namedWindow("Bit");
  namedWindow("Motion");

  motion_file.open(argv[2]);
  bit_file.open(argv[3]);

  processVideo(argv[1]);
  
  motion_file.close();
  bit_file.close();
  cout<< "maxBit: " << maxBit <<'\n';
  cout<< "maxMV: " << maxMV <<'\n';

  /* Destroy GUI windows. */
  destroyAllWindows();
  return EXIT_SUCCESS;
}

/**
 * Processes the video. The code of ViBe is included here. 
 *
 * @param videoFilename  The name of the input video file. 
 */
void processVideo(char* videoFilename)
{
  /* Create the capture object. */
  VideoCapture capture(videoFilename);

  if (!capture.isOpened()) {
    /* Error in opening the video input. */
    cerr << "Unable to open video file: " << videoFilename << endl;
    exit(EXIT_FAILURE);
  }

  /* Variables. */
  static int frameNumber = 1; /* The current frame number */
  Mat frame, input_frame;                  /* Current frame. */
  Mat segmentationMap,longvt;        /* Will contain the segmentation map. This is the binary output map. */
  Mat bitMap, motionMap, tmp;        /* Will contain the segmentation map. This is the binary output map. */
  int keyboard = 0;           /* Input from keyboard. Used to stop the program. Enter 'q' to quit. */
  
  // long coding
  
  int height, width;
  bit_file >> width >> height;
  motion_file >> width >> height;
  cout << height << ' ' << width;


  frame = Mat(height, width, CV_8UC1);
  bitMap = Mat(height, width, CV_8UC1);
  motionMap = Mat(height, width, CV_8UC1);
  tmp = Mat(height, width, CV_8UC1);

  moveWindow("Segmentation",width,height*0.5);
  moveWindow("Bit",width,height*2);
  moveWindow("Motion",0,height*1.5);

  int erosion_size = 1;
  Mat element = getStructuringElement(cv::MORPH_CROSS,
                cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                cv::Point(erosion_size, erosion_size) );

  /* Model for ViBe. */
  vibeModel_Sequential_t *model = NULL; /* Model used by ViBe. */

  /* Read input data. ESC or 'q' for quitting. */
  while ((char)keyboard != 'q' && (char)keyboard != 27) {
    /* Read the current frame. */
    if (!capture.read(input_frame)) {
      cerr << "Unable to read next frame." << endl;
      cerr << "Exiting..." << endl;
      break;
      exit(EXIT_FAILURE);
    }
    cout << "frame" << frameNumber <<"\n";
    
    if (frameNumber==1000) break;
  
    read_jm_res(height/4, width/4);

    for (int i=0;i<height;i++)
      for (int j=0;j<width;j++){
        int index = i*width+j;
        
        bitMap.data[index] = bit[i/4][j/4];
        frame.data[index] = (int)alpha * bitMap.data[index];
        if (bit[i/4][j/4]>maxBit) maxBit = bit[i/4][j/4];

        motionMap.data[index] = (int)round(sqrt(mv_x[i][j]*mv_x[i][j]+mv_y[i][j]*mv_y[i][j]));
        frame.data[index]+=(int)beta * motionMap.data[index];
        if ((int)round(sqrt(mv_x[i][j]*mv_x[i][j]+mv_y[i][j]*mv_y[i][j]))>maxMV) maxMV = (int)round(sqrt(mv_x[i][j]*mv_x[i][j]+mv_y[i][j]*mv_y[i][j]));

    }

    preprocess(frame.data, tmp.data, height, width);


    if (frameNumber == 1) {
      segmentationMap = Mat(frame.rows, frame.cols, CV_8UC1);
      longvt = Mat(frame.rows, frame.cols, CV_8UC1);
      model = (vibeModel_Sequential_t*)libvibeModel_Sequential_New();
      libvibeModel_Sequential_AllocInit_8u_C1R(model, frame.data, frame.cols, frame.rows);
    }  

    /* ViBe: Segmentation and updating. */
    libvibeModel_Sequential_Segmentation_8u_C1R(model, frame.data, segmentationMap.data, longvt.data);
    libvibeModel_Sequential_Update_8u_C1R(model, frame.data, segmentationMap.data);

    
    
    for (int i=0;i<height;i++){
      for (int j=0;j<width;j++){
        int index = i*width+j;
        res[i][j] = segmentationMap.data[index];
      }
    }  
    filter(height,width, size_min);
    for (int i=0;i<height;i++){
      for (int j=0;j<width;j++){
        int index = i*width+j;
        segmentationMap.data[index]=res[i][j];
      }
    }    

    postprocess(segmentationMap.data, tmp.data, height, width);   
    //medianBlur(segmentationMap, segmentationMap, 3); /* 3x3 median filtering */

    //erode(segmentationMap, segmentationMap, element);
    //dilate(segmentationMap, segmentationMap, element);
    
    resize(input_frame, input_frame, cv::Size(), 0.25, 0.25);
   // resize(segmentationMap, segmentationMap, cv::Size(), 2, 2);
   // resize(bitMap, bitMap, cv::Size(), 2, 2);
      
    imshow("Frame", input_frame);
    imshow("Segmentation", segmentationMap);
    imshow("Bit", bitMap);
    imshow("Motion", motionMap);


   // resize(bitMap, bitMap, cv::Size(), 0.5, 0.5);
   // resize(segmentationMap, segmentationMap, cv::Size(), 0.5, 0.5);
    resize(input_frame, input_frame, cv::Size(), 4, 4);

    ++frameNumber;

    /* Gets the input from the keyboard. */
    keyboard = waitKey(1);
  }

  /* Delete capture object. */
  capture.release();

  /* Frees the model. */
  libvibeModel_Sequential_Free(model);
}

// long coding
// HMI
const int connected = 8;
int dir_x[8] = { 0 ,1,0,-1,-1, 1, 1, -1};
int dir_y[8] = { -1,0,1, 0, 1, 1,-1, -1};

void read_jm_res(int height, int width){
  int x;
  motion_file >> x;
  bit_file >> x;
  for (int i=0;i<height;i++)
    for (int j=0;j<width;j++){
      bit_file >> bit[i][j] >> type[i][j];
    }
  for (int i=0;i<height*4;i++)
    for (int j=0;j<width*4;j++){
      motion_file >> mv_x[i][j] >> mv_y[i][j];
    }
}

void preprocess(uint8_t *image_data, uint8_t *tmp, int height, int width){
  
  for (int i=0;i<height;i++){
    for (int j=0;j<width;j++){
      int index = j + i * width;
      tmp[index] = image_data[index];
    }
  }

  int WS = HWS*2+1;
  int SWS = WS*WS-1;
  for (int i=HWS;i<height-HWS;i++){
    for (int j=HWS;j<width-HWS;j++){
        int sum = 0;
        for (int u=-HWS;u<=HWS;u++)
        for (int v=-HWS;v<=HWS;v++){
          int y = i-u;
          int x = j-v;
          if (u!=0 || v!=0){
            int index = x + y * width;
            sum+=tmp[index];
          }
        }
        sum/=SWS;
        int index = j + i * width;
        if (image_data[index]>sum) image_data[index]=sum;
      }
  }
}

void postprocess(uint8_t *image_data, uint8_t *tmp, int height, int width){

  for (int i=0;i<height;i++){
    for (int j=0;j<width;j++){
      int index = j + i * width;
      tmp[index] = image_data[index];
    }
  }

  int WS = HWS*2+1;
  int SWS = WS*WS-1;
  for (int i=HWS;i<height-HWS;i++){
    for (int j=HWS;j<width-HWS;j++){
        int sum = 0;
        for (int u=-HWS;u<=HWS;u++)
        for (int v=-HWS;v<=HWS;v++){
          int y = i-u;
          int x = j-v;
          if (u!=0 || v!=0){
            int index = x + y * width;
            if (tmp[index]>0) sum++;
          }
        }
        int index = j + i * width;
        if (sum<SWS/2+1) image_data[index]=0;
      }
  }
}


void filter(int height, int width, int size) {
  filter_cadidate(height, width, size);
}


void filter_cadidate(int height, int width, int pSize_min) {
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++) mark[i][j] = 0;

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      if (mark[i][j] == 0 && res[i][j]>0) segmentation(i, j, height, width, pSize_min);
  
}

void segmentation(int py, int px, int height, int width, int pSize_min) {
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
      if (xxx >= 0 && yyy >= 0 && xxx < width && yyy < height && res[yyy][xxx] == 0)
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
    for (int i = 0; i < connected; i++) {
      int xx = x + dir_x[i];
      int yy = y + dir_y[i];
      if (xx >= 0 && yy >= 0 && xx < width && yy < height && mark[yy][xx] == 0 && res[yy][xx]>0) {
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
  if (pSize_min > 0 && cuoi < pSize_min) size = 0;
  
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
    for (int i = 1; i <= cuoi; i++) res[qy[i]][qx[i]] = 0;
  /*
  else
   {
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
      for (int i = 1; i <= cuoi; i++) res[qy[i]][qx[i]] = 2;
    }   
    else if (density == 1 )
      for (int i = 1; i <= cuoi; i++) res[qy[i]][qx[i]] = 1;
    else 
      for (int i = 1; i <= cuoi; i++) res[qy[i]][qx[i]] = 0;
  }
  */
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
