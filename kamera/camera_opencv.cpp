#include "camera_opencv.h"

#define DEBUG 1

void *StartSyncCamera(void *thread_data) {
    struct camera_thread_data *local_data;
    local_data = (struct camera_thread_data*) thread_data;
    cout << "Searching for pattern...\n";
    double matchvalue = 0.0;
    *local_data->returnvalue = -1;
    for(;;) {
        matchvalue = local_data->cap->PatternMatching_scaled(CV_TM_SQDIFF_NORMED); //look for pattern
        if (matchvalue>0) break; //pattern found, break loop
        local_data->cap->FlushFrames(1); //if no pattern was detected, keep camera busy for 1 second
    }
    cout << "Pattern found, matchvalue: " << matchvalue << "\n";
    local_data->cap->InitializeTracker("KCF");
    //int xcoord = 0;
    for(;;) {
        *local_data->returnvalue = local_data->cap->TrackNext();
    }
}


/**
 * @brief ContinuousScanBarcode Keeps scanning for barcodes
 * @param thread_data A pointer to a barcode_thread_data structure
 * @return Numeric barcode in exit message
 */

void *ContinuousScanBarcode(void *thread_data) {
    long* barcode = new long(0);
    struct barcode_thread_data *local_data;
    local_data = (struct barcode_thread_data*) thread_data;
    //keep scanning until we find a barcode
    if (local_data->retries!=0) {
        for (int i = 0;i<local_data->retries;i++) {
            *barcode=local_data->cap->ScanBarcode(local_data->show_rectangle);
            if(*barcode != 0) {
                pthread_exit((void**)barcode); //found barcode or encountered an error, exiting
            }
        }
    }
    else for (;;) { // infinite scan
        *barcode=local_data->cap->ScanBarcode(local_data->show_rectangle);
        if(*barcode != 0) {
            pthread_exit((void**)barcode); //found barcode or encountered an error, exiting
        }
    }
    pthread_exit((void**)barcode); // exit with zero
}


SyncCamera::SyncCamera(int camera_number) {
    cap.open(camera_number);
    cam_fps = cap.get(CAP_PROP_FPS);
}

SyncCamera::~SyncCamera() {
    cap.release();
}

/**
 * @brief SyncCamera::set_resolution Set the camera resolution
 * @param width width in pixels
 * @param height height in pixels
 */

void SyncCamera::set_resolution(int width, int height) {
    cap.set(CAP_PROP_FOURCC, CAP_OPENCV_MJPEG);
    cap.set(CAP_PROP_FRAME_WIDTH,width);
    cap.set(CAP_PROP_FRAME_HEIGHT,height);
    cam_fps = cap.get(CAP_PROP_FPS); //read back fps
}

/**
 * @brief SyncCamera::set_pattern Sets the template for pattern recognition
 * @param pattern path to a pattern file
 */
void SyncCamera::set_pattern(String pattern) {
    input_template = imread(pattern,IMREAD_COLOR);
}

void SyncCamera::set_matchval(double value) {
    matchval = value;
}


/**
 * @brief ScanBarcode Scans an image for a barcode
 * @param image The image to search the barcode in
 * @param show_rectangle If set to 1, draws a rectangle around the barcode in the provided image (defaults to 0)
 * @return Barcod#include "opencv2/opencv.hpp"e in numerical form
 */

long SyncCamera::ScanBarcode(int show_rectangle) {
    if(!cap.isOpened()) { return -1; }
    Mat image;
    cap >> image;
    Mat grayscale;
    cvtColor(image,grayscale,CV_YUV2RGB); // convert to RGB
    cvtColor(grayscale,grayscale,CV_RGBA2GRAY); //convert to grayscale
    //cvtColor(framein,framein,CV_YUV2GRAY_420);
    long barcode = 0;
    ImageScanner barscan;
    if(barscan.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1)!=0) { return -1; }
    // convert Matrix to useable image;
    int width = grayscale.cols;
    int height = grayscale.rows;
    uchar *rawframe = (uchar *)grayscale.data;
    Image image_gray(width, height, "Y800", rawframe, width * height);
    if(DEBUG) {
        imshow("FEED ME BARCODES",image);
        waitKey(1);
    }
    //scan for barcode, returns 0 on no barcode, or value >0 when a barcode was found
    int i = barscan.scan(image_gray);
    // exit on no barcode
    if (i==0) return 0;
    //get barcode from image
    for(Image::SymbolIterator symbol = image_gray.symbol_begin();symbol != image_gray.symbol_end();++symbol) {
        //cout << "Detected Type: " << symbol->get_type_name() << "    Barcode: " << symbol->get_data() << "                  \r";
        // ONLY RETURNS LAST BARCODE DETECTED, IF MULTIPLE!
        barcode = stol(symbol->get_data());
        if(show_rectangle) {
            vector<Point> vp;
            int n = symbol->get_location_size();
            // mark barcode rectangle
            for(int i=0;i<n;i++)  {
                vp.push_back(Point(symbol->get_location_x(i),symbol->get_location_y(i)));
            }
            RotatedRect r = minAreaRect(vp);
            Point2f pts[4];
            r.points(pts);
            // add rectangle points to original image
            for (int i=0;i<4;i++){
                line(image,pts[i],pts[(i+1)%4],Scalar(255,0,0),3);
            }
        }
    }
    // all done, return barcode
    return barcode;
}


/**
 * @brief SyncCamera::PatternMatching_scaled Multi-scale template matching
 * @param match_method the CV match method
 * @param minMatchQuality minimum quality to get accepted as a match, values between 0 and 1 (good matches are >0.7)
 * @param iterations number of downscaled images to process (higher gets better matches, but is slower)
 * @return -1 on error, 0 otherwise
 */

double SyncCamera::PatternMatching_scaled(int match_method, int iterations) {
    Mat result, image_resized;
    if (!cap.isOpened()) { return -1; }
    double matchVal_best = 0.0, scale_best = 0.0;
    Point matchLoc_best;
    cap >> tracking_image;
    image_resized = tracking_image.clone(); //copy the input image
    for (int i=0;i<iterations;i++) {  // loop over input image scales, divide into even chunks between 100% and 20%
        double curr_scale = 1.0 - (0.8/iterations * i);
        resize(tracking_image,image_resized,Size(),curr_scale,curr_scale,INTER_AREA); // resize picture to current scale
        int result_rows = image_resized.rows - input_template.rows + 1;  //calculate max. size
        int result_cols = image_resized.cols - input_template.cols + 1;
        if ( result_rows < 1 || result_cols < 1 ) break; // exit loop if template is bigger than the image
        //create result & match template
        result.create(result_rows,result_cols,CV_32FC1);
        matchTemplate(image_resized,input_template,result,match_method);
        // extract values with minmaxloc
        double minVal, maxVal, matchVal;
        Point minLoc, maxLoc, matchLoc;
        minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
        if(match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED)  // these methods have reverse logic, smaller number = better match
        {
            matchLoc = minLoc;
            matchVal = (1-minVal);
        }
        else
        {
            matchLoc = maxLoc;
            matchVal = maxVal;
        }
        if(matchVal>this->matchval && matchVal>matchVal_best) { // new best match found, saving
            matchVal_best = matchVal;
            matchLoc_best = matchLoc;
            scale_best = curr_scale;
        } // else discard all
    }
    //if(matchVal_best == 0) { return 0; }  //nothing found
    //carefully undo the scaling and save initial rectangle for motion tracking later
    tracking_rectangle.x = matchLoc_best.x/scale_best;
    tracking_rectangle.y = matchLoc_best.y/scale_best;
    tracking_rectangle.width = input_template.cols/scale_best;
    tracking_rectangle.height = input_template.rows/scale_best;
    if(scale_best!=0) {
        rectangle(tracking_image,tracking_rectangle,Scalar::all(0), 2, 8, 0 );  //draw rectangle into the original image
    }
    if(DEBUG) {
        cout << "best match:   " << matchVal_best << "location:  " << matchLoc_best;
        imshow("pattern?",tracking_image);
        waitKey(1);
    }
    return matchVal_best;
}

/**
 * @brief Searches for a template pattern in an image
 * @param input_template A template image
 * @param image The image to search the template in. On a successful match, draws a rectangle around the match
 * @param match_method normalized OpenCV match method (defaults to CV_TM_SQDIFF_NORMED)
 * @return 1 if a pattern was found
 * @return 0 if the pattern wasn't found within the image (score below threshold)
 * @return -1 on error
 */

int SyncCamera::PatternMatching(int match_method) {
    Mat result, image;
    if (!cap.isOpened()) { return -1; }
    cap >> image;

    //calculate max. size
    int result_rows = image.rows - input_template.rows + 1;
    int result_cols = image.cols - input_template.cols + 1;
    //create result & match template
    result.create(result_rows,result_cols,CV_32FC1);
    matchTemplate(image,input_template,result,match_method);
    // extract values with minmaxloc
    double minVal, maxVal, matchVal;
    Point minLoc, maxLoc, matchLoc;
    minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
    if(match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED)
    { matchLoc = minLoc;
        matchVal = (1-minVal);
    }
    else
    { matchLoc = maxLoc;
        matchVal = maxVal;
    }
    cout << "Val: " << matchVal << "   Location: " << matchLoc << "        \r";
    //reject if beloq minMatchQuality threshold
    if (matchVal<this->matchval) {
        if(DEBUG) {
            imshow("pattern?",image);
            waitKey(1);
        }
        return 0;
    }

    else {
        //normalize and run minmaxloc again for rectangle
        normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
        minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
        rectangle(image, matchLoc, Point(matchLoc.x + input_template.cols, matchLoc.y + input_template.rows), Scalar::all(0), 2, 8, 0  );

        if(DEBUG) {
            imshow("pattern?",image);
            waitKey(1);
        }
    }
    return 1;
}

/**
 * @brief SyncCamera::InitializeTracker Initializes the motion tracker from a previously detected pattern
 * @param tracker_type MEDIANFLOW and KCF are supported
 * @return -1 on error, else 0
 */

int SyncCamera::InitializeTracker(string tracker_type) {
    if(tracker_type=="MEDIANFLOW") {
        tracker = TrackerMedianFlow::create();
    }
    else if(tracker_type=="KCF") {
        tracker = TrackerKCF::create();
    } else return -1;
    //initialize tracker
    tracker->init(tracking_image,tracking_rectangle);
    tracker_is_initialized = 1;
    return 0;
}

/**
 * @brief SyncCamera::TrackNext updates the previously initialized tracker with the next image
 * @return -1 on tracking error, -2 if tracker is not initialized, else X-coordinate of the center of the image
 */

int SyncCamera::TrackNext() {
    int return_value = 0;
    if (!cap.isOpened() || tracker_is_initialized != 1) { return -2; }
    cap >> tracking_image;
    bool ok = tracker->update(tracking_image,tracking_rectangle);  //update tracker
    if(ok) { //if successful, return new x-coordinate
        if(DEBUG) { //draw rectangle and display image
            rectangle(tracking_image,tracking_rectangle, Scalar::all(0), 2, 8, 0);
            imshow("tracking",tracking_image);
            waitKey(1);
        }
        return_value = (int)(tracking_rectangle.x + tracking_rectangle.width/2);
        return return_value;
    }
    else return -1;
}

/**
 * @brief SyncCamera::FlushFrames Keeps camera busy so we don't read stale frames
 * @param seconds seconds to delay
 */

void SyncCamera::FlushFrames(int seconds) {
    if(!cap.isOpened()) return;
    int j = cam_fps * seconds;
    for(int i=0;i<j;i++)
        cap.grab();
    return;
}
