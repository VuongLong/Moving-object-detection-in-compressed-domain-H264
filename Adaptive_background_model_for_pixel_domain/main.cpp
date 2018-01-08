/**
 * @file main-opencv.cpp
 * @date July 2014 
 * @brief An exemplative main file for the use of ViBe and OpenCV
 */
#include <iostream>

#include "opencv2/imgproc.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include "vibe-background-sequential.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <algorithm>    // std::sort
#include <fstream>

using namespace cv;
using namespace std;


int ground_truth[2000][3000];
int result[2000][3000];
int H = 0;
int W = 0;

int TP_corvalue = 0;
int TP_pos = 0;

int FP_corvalue = 0;
int FP_pos = 0;

int TN_corvalue = 0;
int TN_pos = 0;

int FN_corvalue = 0;
int FN_pos = 0;

float TruePositive()
{
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			if (ground_truth[i][j] != 0 && ground_truth[i][j] != 85)
			{
				TP_pos ++;
				if (result[i][j] != 0 && result[i][j] != 85)
					TP_corvalue ++;
			}
		}
	}
	if (TP_pos == 0)
	{
		return 1;
	}
		
	return ((float)TP_corvalue) / TP_pos;
}

float FalsePositive()
{
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			if (ground_truth[i][j] != 0 && ground_truth[i][j] != 85)
			{
				FP_pos++;
				if (result[i][j] == 0 || result[i][j] == 85)
					FP_corvalue++;
			}
		}
	}
	if (FP_pos == 0)
	{
		return 1;
	}
	return ((float)FP_corvalue) / FP_pos;
}

float TrueNegative()
{
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			if (ground_truth[i][j] == 0 || ground_truth[i][j] == 85)
			{
				TN_pos++;
				if (result[i][j] == 0 || result[i][j] == 85)
					TN_corvalue++;
			}
		}
	}
	if (TN_pos == 0)
		return 1;
	return ((float)TN_corvalue) / TN_pos;
}

float FalseNegative()
{
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			if (ground_truth[i][j] == 0 || ground_truth[i][j] == 85)
			{
				FN_pos++;
				if (result[i][j] != 0 && result[i][j] != 0)
					FN_corvalue++;
			}
		}
	}
	if (FN_pos == 0)
		return 1;
	return ((float)FN_corvalue) / FN_pos;
}

bool myComparefunc (string i,string j)
{
	return (i.compare(j) < 0); 
}

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }

    std::sort (files.begin(), files.end(), myComparefunc); 

    closedir(dp);
    return 0;
}

/** Function Headers */
void processVideo(char* videoFilename, char* gtdir);

/**
 * Displays instructions on how to use this program.
 */

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
bool needTest = false;

/**
 * Main program. It shows how to use the grayscale version (C1R) and the RGB version (C3R). 
 */
int main(int argc, char* argv[])
{
	/* Print help information. */
	help();

	/* Check for the input parameter correctness. */
	if (argc < 2) {
		cerr <<"Incorrect input" << endl;
		cerr <<"exiting..." << endl;
		return EXIT_FAILURE;
	}
	char* gtdir = argv[1]; //just for init
	if (argc == 3)
	{
		needTest = true;
		gtdir = argv[2];
	}


	/* Create GUI windows. */
	namedWindow("Frame");
	namedWindow("Segmentation by ViBe");

	processVideo(argv[1], gtdir);

	/* Destroy GUI windows. */
	destroyAllWindows();
	return EXIT_SUCCESS;
}

/**
 * Processes the video. The code of ViBe is included here. 
 *
 * @param videoFilename  The name of the input video file. 
 */
void processVideo(char* videoFilename, char* gtdir)
{

	string dir = string(videoFilename);
	
	vector<string> files = vector<string>();

	getdir(dir,files);


	string groundtruth_dir = string(gtdir);
	
	vector<string> groundtruth_files = vector<string>();

	if (needTest)
		getdir(groundtruth_dir,groundtruth_files);

	//for (unsigned int i = 0;i < files.size();i++)
	//{
	//	cout << files[i] << endl;
	//}
  /* Create the capture object. */
  //VideoCapture capture(videoFilename);

  //if (!capture.isOpened()) {
  //  /* Error in opening the video input. */
  //  cerr << "Unable to open video file: " << videoFilename << endl;
  //  exit(EXIT_FAILURE);
  //}

  /* Variables. */
  static int frameNumber = 1; /* The current frame number */
  Mat frame;                  /* Current frame. */
  Mat segmentationMap;        /* Will contain the segmentation map. This is the binary output map. */
  int keyboard = 0;           /* Input from keyboard. Used to stop the program. Enter 'q' to quit. */

  /* Model for ViBe. */
  vibeModel_Sequential_t *model = NULL; /* Model used by ViBe. */

  /* Read input data. ESC or 'q' for quitting. */
  int frame_index = 2;
  while ((char)keyboard != 'q' && (char)keyboard != 27) {
    /* Read the current frame. */
    //if (!capture.read(frame)) {
    //  cerr << "Unable to read next frame." << endl;
    //  cerr << "Exiting..." << endl;
    //  exit(EXIT_FAILURE);
    //}
    string image_path = videoFilename;
    if(image_path.at(image_path.length()-1) != '/')
    	image_path.append("/");
    image_path.append(files[frame_index]);

    string groundtruth_path = gtdir;
    if (needTest)
    {
    	if(groundtruth_path.at(groundtruth_path.length()-1) != '/')
    		groundtruth_path.append("/");
   		groundtruth_path.append(groundtruth_files[frame_index]);
    }
    
    cout << image_path;
    if (needTest)
    	cout << "    vs    " << groundtruth_path;
    cout << endl;
  	frame = imread(image_path, CV_LOAD_IMAGE_COLOR);
  	H = frame.rows;
  	W = frame.cols;
  	if(! frame.data )                              // Check for invalid input
    {
        cerr << "Unable to read next frame." << endl;
    	cerr << "Exiting..." << endl;
    	exit(EXIT_FAILURE);
    }
    frame_index ++;

    if ((frameNumber % 100) == 0) { cout << "Frame number = " << frameNumber << endl; }

    /* Applying ViBe.
     * If you want to use the grayscale version of ViBe (which is much faster!):
     * (1) remplace C3R by C1R in this file.
     * (2) uncomment the next line (cvtColor).
     */
    /* cvtColor(frame, frame, CV_BGR2GRAY); */

    if (frameNumber == 1) {
      segmentationMap = Mat(frame.rows, frame.cols, CV_8UC1);
      model = (vibeModel_Sequential_t*)libvibeModel_Sequential_New();
      libvibeModel_Sequential_AllocInit_8u_C3R(model, frame.data, frame.cols, frame.rows);
    }

    /* ViBe: Segmentation and updating. */
    libvibeModel_Sequential_Segmentation_8u_C3R(model, frame.data, segmentationMap.data);
    libvibeModel_Sequential_Update_8u_C3R(model, frame.data, segmentationMap.data);

    /* Post-processes the segmentation map. This step is not compulsory. 
       Note that we strongly recommend to use post-processing filters, as they 
       always smooth the segmentation map. For example, the post-processing filter 
       used for the Change Detection dataset (see http://www.changedetection.net/ ) 
       is a 5x5 median filter. */
    medianBlur(segmentationMap, segmentationMap, 5); /* 3x3 median filtering */

    if (needTest)
    {
	    for (int i = 0; i < H ; i++)
		{
			for (int j = 0; j < W; j++)
			{
				int index = i*W + j;
				result[i][j] = segmentationMap.data[index];
			}
		}

		ifstream gt_fin;
		gt_fin.open(groundtruth_path.c_str());
		for (int i = 0; i < H ; i++)
		{
			for (int j = 0; j < W; j++)
			{
				gt_fin >> ground_truth[i][j];
			}
		}
		float tp = TruePositive();
		float fp = FalsePositive();
		//float tn = TrueNegative();
		float fn = FalseNegative();

		float recall = tp / (tp + fp);
		float precision = tp / (tp + fn);
		cout << "Recall: " << recall << "    vs    Precision: " << precision << endl;
    }
	

    /* Shows the current frame and the segmentation map. */
    imshow("Frame", frame);
    imshow("Segmentation by ViBe", segmentationMap);

    ++frameNumber;
    //break;

    /* Gets the input from the keyboard. */
    keyboard = waitKey(1);
  }

  /* Delete capture object. */
  //capture.release();

  /* Frees the model. */
  libvibeModel_Sequential_Free(model);
}
