#ifndef CAMERA_OPENCV_H
#define CAMERA_OPENCV_H

#include "opencv2/opencv.hpp"
#include "opencv2/video.hpp"
#include "opencv2/tracking/tracker.hpp"
#include "zbar.h"
#include <pthread.h>
#include <iostream>

using namespace std;
using namespace cv;
using namespace zbar;

class SyncCamera {
private:
    VideoCapture cap;
    Mat input_template = imread("pattern.png",IMREAD_COLOR);
    Mat tracking_image;
    Rect2d tracking_rectangle;
    Ptr<Tracker> tracker;
    int tracker_is_initialized = 0;
    int cam_fps = 0;

public:
    SyncCamera(int camera_number);
    ~SyncCamera();
    void set_resolution(int width, int height);
    void set_pattern(String pattern);
    double PatternMatching_scaled(int match_method = CV_TM_CCOEFF_NORMED, double minMatchQuality = 0.55, int iterations = 20);
    long ScanBarcode(int show_rectangle=0);
    int PatternMatching(int match_method = CV_TM_CCOEFF_NORMED, double minMatchQuality = 0.55);
    int TrackNext();
    int InitializeTracker(string tracker_type = "MEDIANFLOW");
    void FlushFrames(int seconds);
};

struct barcode_thread_data {
    SyncCamera *cap = NULL;
    int show_rectangle = 0;
    int retries = 0; // 0 = infinite
};

void *ContinuousScanBarcode(void *thread_data);

#endif

