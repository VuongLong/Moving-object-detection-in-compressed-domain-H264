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


using namespace cv;
using namespace std;

/** Function Headers */
void processVideo(char* videoFilename);

/**
 * Displays instructions on how to use this program.
 */

int distance(int height, int width, int y1, int x1, int y2, int x2);
void read_jm_res(int height, int width);
void optimize_parameters(int height, int width);
void filter(int height, int width);
void filter_threshold(int height, int width);
void filter_spatial(int height, int width);
void filter_cadidate(int height, int width, int size_min);
int calculate_angle(int x, int y);
void segmentation(int py, int px, int height, int width, int size_min);


// long coding
ifstream motion_file;
ifstream bit_file;
ofstream threshold_file;
ofstream DF_file;
ofstream DB_file;

double out_threshold = 0;
int out_count = 0;
int threshold_bit = 200;
double precision, recall;
double F1_score = 70;
int bit[100][120];
int res[100][120];
int SM[100][120];
int type[100][120];
double mv_x[272][480];
double mv_y[272][480];
int GOP=250;

int DF[300]={0};
int isTesting = 0;
int test_theshold = 0;
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
 * Main program. It shows how to use the grayscale version (C1R) and the RGB version (C3R). 
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
  namedWindow("Segmentation by ViBe");
  namedWindow("Segmentation by proposed");

  threshold_file.open("output.txt");
  DF_file.open("DF.txt",ios_base::app);
  motion_file.open(argv[2]);
  bit_file.open(argv[3]);

  if (argc > 4){
    isTesting = 1;
    int x = strlen(argv[4]);
    test_theshold = 0;
    for (int i=0;i<x;i++){
      test_theshold = (test_theshold*10)+(int)(argv[4][i]-'0');
    }
    cout << test_theshold<<"test"<<'\n';
  }
  processVideo(argv[1]);
  threshold_file << "Average: "<< (int)out_threshold /out_count;
  
  float countm = 0;
  float summ = 0;
  for (int i=0;i<300;i++){
    //DF_file << DF[i] << '\n';
    if (i>=50){
      countm +=DF[i];
      summ += DF[i]*i;
    }    
  }
  int mean = round(summ/countm);
  int s2;
  for (int i=50;i<300;i++){
    s2 = s2+DF[i]*(i-mean)*(i-mean);
  }
  s2=s2/countm;
  int s = round(sqrt(s2));
  DF_file << argv[2] << ": " << mean - s << '\n';

  
  motion_file.close();
  bit_file.close();
  threshold_file.close();
  DF_file.close();

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
  Mat segmentationMap;        /* Will contain the segmentation map. This is the binary output map. */
  Mat proposedMap;        /* Will contain the segmentation map. This is the binary output map. */
  int keyboard = 0;           /* Input from keyboard. Used to stop the program. Enter 'q' to quit. */
  
  // long coding
  
  int height, width;
  motion_file >> width >> height;
  bit_file >> width >> height;
  cout << height << ' ' << width;

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
    if (frameNumber==900) break;
    //if ((frameNumber % 100) == 0) { cout << "Frame number = " << frameNumber << endl;}

    /* Applying ViBe.
     * If you want to use the grayscale version of ViBe (which is much faster!):
     * (1) remplace C3R by C1R in this file.
     * (2) uncomment the next line (cvtColor).
     */
    /* cvtColor(frame, frame, CV_BGR2GRAY); */
    resize(input_frame, frame, cv::Size(), 0.25, 0.25);
    if (frameNumber == 1) {
      segmentationMap = Mat(frame.rows, frame.cols, CV_8UC1);
      proposedMap = Mat(frame.rows/4, frame.cols/4, CV_8UC1);
      model = (vibeModel_Sequential_t*)libvibeModel_Sequential_New();
      libvibeModel_Sequential_AllocInit_8u_C3R(model, frame.data, frame.cols, frame.rows);
      cout << frame.cols << " " << frame.rows << '\n';
    }

    /* ViBe: Segmentation and updating. */
    libvibeModel_Sequential_Segmentation_8u_C3R(model, frame.data, segmentationMap.data);
    libvibeModel_Sequential_Update_8u_C3R(model, frame.data, segmentationMap.data);



    /* Post-processes the segmentation map. This step is not compulsory. 
       Note that we strongly recommend to use post-processing filters, as they 
       always smooth the segmentation map. For example, the post-processing filter 
       used for the Change Detection dataset (see http://www.changedetection.net/ ) 
       is a 5x5 median filter. */
    resize(segmentationMap, segmentationMap, cv::Size(), 0.25, 0.25);
    medianBlur(segmentationMap, segmentationMap, 3); /* 3x3 median filtering */
    
    


    // long coding
    cout << "frame" << frameNumber <<"\n";
    if ((frameNumber-1) % GOP != 0 && frameNumber > 100){
      
      read_jm_res(height, width);
      
      for (int i=0;i<height;i++){
          for (int j=0;j<width;j++){
            int index = i*width+j;
            SM[i][j] = segmentationMap.data[index];
            if (SM[i][j]>0 && bit[i][j]<300) DF[bit[i][j]]++;
          }
        }  

      if (isTesting){
        threshold_bit = test_theshold;
        filter(height, width);        
      } 
    }

    resize(input_frame, input_frame, cv::Size(), 0.25, 0.25);
    resize(segmentationMap, segmentationMap, cv::Size(), 4, 4);
    resize(proposedMap, proposedMap, cv::Size(), 4, 4);

    /* Shows the current frame and the segmentation map. */
    imshow("Frame", input_frame);
    imshow("Segmentation by ViBe", segmentationMap);
    imshow("Segmentation by proposed", proposedMap);

    resize(proposedMap, proposedMap, cv::Size(), 0.25, 0.25);
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

// HMI

int dir_x[8] = { -1,0,1,1,1,0,-1,-1 };
int dir_y[8] = { -1,-1,-1,0,1,1,1,0 };
int qx[1000000];
int qy[1000000];
int mark[272][480];


void filter(int height, int width) {
  filter_threshold(height, width);
  filter_spatial(height, width);
  filter_cadidate(height, width, 20);
}

void filter_threshold(int height, int width) {
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++) {

      if (bit[i][j] < threshold_bit) res[i][j] = 0;
      else res[i][j] = bit[i][j];

      if (type[i][j] == 1) {
        int count = 0;
        if ((i - 1 >= 0) && (res[i - 1][j] > 0)) count++;
        if ((j - 1 >= 0) && (res[i][j - 1] > 0)) count++;
        if ((i - 1 >= 0) && (j + 1 < width) && (res[i - 1][j + 1] > 0)) count++;
        if (count == 3) {
          res[i][j] = round((res[i - 1][j] + res[i][j - 1] + res[i - 1][j + 1]) / 3);
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
      if (res[i][j] == 0) {
        int count = 0;
        int amount = 0;
        for (int k = 0; k < 8; k++) {
          int yy = i + y[k];
          int xx = j + x[k];
          if ((xx >= 0) && (xx < width) && (yy >= 0) && (yy < height) && (res[yy][xx] > 0)) {
            count++;
            amount += res[yy][xx];
          }
        }
        if (count > 4) res[i][j] = round(amount / count);
      }
    }
}

void filter_cadidate(int height, int width, int size_min) {
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++) mark[i][j] = 0;

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      if (mark[i][j] == 0 && res[i][j]>0) segmentation(i, j, height, width, size_min);
  
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
    for (int i = 0; i < 8; i++) {
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
    for (int i = 1; i <= cuoi; i++) res[qy[i]][qx[i]] = 0;
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
