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
    if(argc>1) { j=atoi(argv[1]); } else { j = 1; }

    SyncCamera cam(j);

    cam.set_resolution(320,240);
    cam.set_matchval(0.7);
    cam.set_pattern("OTH_logo_small_2.png");

    pthread_t thread;
    camera_thread_data data;
    volatile int return_value;
    return_value = 0;
    data.cap = &cam;
    data.returnvalue = &return_value;
    pthread_create(&thread,NULL,StartSyncCamera,(void*) &data);

    for(;;) {
        if(return_value==-1) { cout << "Tracking error/pattern lost         \r"; }
        else if (return_value==-2) { cout << "Tracker not initialized.           \r";}
        else { cout << "Tracked coordinate:  " << return_value << "            \r";  }
        usleep(30000); //sleep 30 ms
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
