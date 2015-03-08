#pragma once

#include "ofMain.h"
//#include "PlayModes.h"
#include "VideoBuffer.h"
#include "VideoGrabber.h"
#include "VideoRate.h"

#define NUM_FRAMES 300
#define NUM_BUFFER 4


class VideoSampler : public ofBaseApp{

public:
    void setup();
    //void setup(int _grabberID);
    void setup(int _grabberID, int _grabberHeight, int _grabberWidth, ofPixelFormat _grabberPixelFormat);
    void update();
    void draw();


    float getRecordPostion();
    void drawPlayerData(float _playhead);
    void drawCurrentBuffer(int _x, int _y, int _height, int _width);
    void drawBuffer(int _x, int _y, int _height, int _width, int _BufferNum);

    //PlayModes playModes;
    // Grabber and Buffer
    ofxPm::VideoGrabber vGrabber;
    int GrabberDeviceID;
    int getGrabberDeviceID ();
    //ofVideoGrabber vGrabber;
    vector <ofxPm::VideoBuffer*> vBuffer ;
    ofxPm::VideoRate vRate;

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
    vector <bool> bPlayBuffer;// (NUM_BUFFER);
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
