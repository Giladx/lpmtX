#pragma once

#include "ofMain.h"
/*#include "VideoBuffer.h"
#include "VideoGrabber.h"
#include "VideoRate.h"*/
#include "ofxVideoBuffers.h"

#define NUM_FRAMES 300
#define NUM_BUFFER 4


class VideoSampler{

public:
    VideoSampler();
    virtual ~VideoSampler();
    void setup();
    void setup(int _grabberID, int _grabberHeight, int _grabberWidth, ofPixelFormat _grabberPixelFormat);
    void setup(ofVideoGrabber &_vGrabber, ofPixelFormat _grabberPixelFormat);
    void setup(ofVideoGrabber & _VideoGrabber, ofImageType _samplerPixType);

    void update();
    void draw();


    float getRecordPostion();
    /*void drawPlayerData(float _playhead);*/
    void drawCurrentBuffer(int _x, int _y, int _height, int _width);
    void drawBuffer(int _x, int _y, int _height, int _width, int _BufferNum);

    // Grabber and Buffer
    //ofVideoGrabber * vGrabber;
    int GrabberDeviceID;
    int getGrabberDeviceID ();
    ofVideoGrabber * vGrabber;
    //vector <ofxPm::VideoBuffer*> vBuffer ;
    //ofxPm::VideoRate vRate;
    ofxVideoBuffers     buffer;
    vector<ofxVideoBuffers *> buffers;

    ofImageType pix_type;

    int fps;
	float speed;
	float playHead;
	float loopSize;
	//Bool event for Recording Toggle/ resets play position relative to current Rec position
    void updatePlayHead();
    bool bUpdatePlayheadEvent;

    int NumBuffer=4;
    int currentBufferNum=0;
    //Recording
    bool bSetPosition;
    bool bRecLiveInput;
    float recMix;
    float recordPosition;

    //Playing
    array <bool, NUM_BUFFER> bPlayBuffer ;// (NUM_BUFFER);
    bool bPlayAnyBuffer;    //increment playhead regerdless of current buffer
    bool bPauseBuffer;
    float playStart;
    float playEnd;
    int bufferSize;
	int	sampleRate;

	//Drawing
    int curXpos, curYpos;
    int prevXpos, prevYpos;


};
