#pragma once

#include "types.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <atomic>

class SyncCamera {
	cv::VideoCapture cap;
	cv::Mat input_template;
	cv::Mat tracking_image;
	cv::Rect2d tracking_rectangle;
	cv::Ptr<cv::Tracker> tracker;
	int tracker_is_initialized = 0;
	int cam_fps = 0;
	double matchval = 0.7;

public:
	SyncCamera(const std::string& cam_path, const std::string& pattern_path);
	~SyncCamera();
	void set_resolution(u32 width, u32 height);
	void set_pattern(cv::String pattern);
	void set_matchval(double value);
	double pattern_matching_scaled(int match_method = CV_TM_CCOEFF_NORMED, int iterations = 20);
	long scan_barcode(int show_rectangle=0);
	int pattern_matching(int match_method = CV_TM_CCOEFF_NORMED);
	int track_next();
	int initialize_tracker(std::string tracker_type = "MEDIANFLOW");
	void flush_frames(int seconds);
	void start_sync_camera(std::atomic<int> *return_value);
	void continuous_scan_barcode(std::atomic<int> *return_barcode, int retries = 0, int show_rectangle = 0);
};

struct barcode_thread_data {
	SyncCamera *cap = NULL;
	int show_rectangle = 0;
	int retries = 0; // 0 = infinite
};

struct camera_thread_data {
	SyncCamera *cap = NULL;
	volatile int *returnvalue;
};

//void *continuous_scan_barcode(void *thread_data);
//void *start_sync_camera(void *thread_data);


