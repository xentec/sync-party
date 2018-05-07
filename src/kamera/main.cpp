#include "camera_opencv.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

int main(int argc, char** argv)
{
    setbuf(stdout, NULL);
    int j = 0;
    // open the default camera unless otherwise specified
    if(argc>1) { j=atoi(argv[1]); } else { j = 0; }

    SyncCamera cam(j);

    cam.set_resolution(800,600);
    double matchval = 0.0;
    cout << "Searching for pattern...\n";
    while(!matchval) {
        matchval = cam.PatternMatching_scaled(CV_TM_SQDIFF_NORMED,0.6);
        cam.FlushFrames(1);
    }
    cout << "Pattern found, matchvalue: " << matchval << "\n";
    cam.InitializeTracker("MEDIANFLOW");
    int xcoord = 0;
    for(;;) {
        xcoord = cam.TrackNext();
        if(xcoord==-1) { cout << "Tracking error/pattern lost         \r"; }
        else if (xcoord==-2) { cout << "Tracker not initialized.           \r";}
        else { cout << "Tracked coordinate:  " << xcoord << "            \r";  }
    }
    /*
    cout << "No barcode found (yet).\r";
    void* barcode;
    pthread_t thread;
    barcode_thread_data data;
    data.cap = &cam;
    data.retries = 0;
    if(pthread_create(&thread,NULL,ContinuousScanBarcode,(void*) &data)) { exit(-1); }
    //wait for thread to finish successfully
    pthread_join(thread,&barcode);
    cout << "Barcode: " << *(long*) barcode << "             \n";
    delete (long*)barcode;
    */
    return 0;
}
