#pragma once

#include "types.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <atomic>

/**
 * @brief The SyncCamera class
 * @author Johannes Eichenseer
 */
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
	/**
	 * @brief SyncCamera
	 * @param cam_path Video device file like /dev/video0
	 * @param pattern_path Image file
	 */
	SyncCamera(const std::string& cam_path, const std::string& pattern_path);
	~SyncCamera();
	/**
	 * @brief Set the camera resolution
	 * @param width width in pixels
	 * @param height height in pixels
	 */
	void set_resolution(u32 width, u32 height);

	/**
	 * @brief Sets the template for pattern recognition
	 * @param pattern path to a pattern file
	 */
	void set_pattern(cv::String pattern);
	/**
	 * @brief set_matchval
	 * @param value minimum quality to get accepted as a match, values between 0 and 1 (good matches are >0.7)
	 */
	void set_matchval(double value);

	/**
	 * @brief Multi-scale template matching
	 * @param match_method the CV match method
	 * @param iterations number of downscaled images to process (higher gets better matches, but is slower)
	 * @return -1 on error, 0 otherwise
	 */
	double pattern_matching_scaled(int match_method = CV_TM_CCOEFF_NORMED, int iterations = 20);
	/**
	 * @brief Scans an image for a barcode
	 * @param show_rectangle If set to 1, draws a rectangle around the barcode in the provided image (defaults to 0)
	 * @return Barcod#include "opencv2/opencv.hpp"e in numerical form
	 */
	long scan_barcode(int show_rectangle=0);
	/**
	 * @brief Searches for a template pattern in an image
	 * @param match_method normalized OpenCV match method (defaults to CV_TM_SQDIFF_NORMED)
	 * @return 1 if a pattern was found
	 * @return 0 if the pattern wasn't found within the image (score below threshold)
	 * @return -1 on error
	 */
	int pattern_matching(int match_method = CV_TM_CCOEFF_NORMED);
	/**
	 * @brief Updates the previously initialized tracker with the next image
	 * @return -1 on tracking error, -2 if tracker is not initialized, else X-coordinate of the center of the image
	 */
	int track_next();
	/**
	 * @brief Initializes the motion tracker from a previously detected pattern
	 * @param tracker_type MEDIANFLOW and KCF are supported
	 * @return -1 on error, else 0
	 */
	int initialize_tracker(std::string tracker_type = "MEDIANFLOW");
	/**
	 * @brief SyncCamera::FlushFrames Keeps camera busy so we don't read stale frames
	 * @param seconds seconds to delay
	 */
	void flush_frames(int seconds);
	void start_sync_camera(std::atomic<int> *return_value);
	/**
	 * @brief ContinuousScanBarcode Keeps scanning for barcodes
	 * @param return_barcode A pointer to a barcode_thread_data structure
	 * @param retries number of tries
	 * @param show_rectangle show match rectangle
	 * @return Numeric barcode in exit message
	 */
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


