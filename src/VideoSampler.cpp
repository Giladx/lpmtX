#include "VideoSampler.h"

void VideoSampler::setup(){
        bufferSize=512;
        playStart=0;
        playEnd=1.0;
        bPauseBuffer=false;
        fps=30;

        //setup grabber
        //vGrabber.setPixelFormat(OF_PIXELS_I420);
        vGrabber.initGrabber(640,480);
        vGrabber.setVerbose(true);

        //setup Buffer
        vRate.setup(vGrabber,fps);
        for (int i=0;i<NumBuffer; i++){

            vBuffer.push_back(new ofxPm::VideoBuffer(vRate,NUM_FRAMES));
            bPlayBuffer.push_back(false);

        }

}
//draw function with both grabbers and buffers
void VideoSampler::setup(int _grabberID, int _grabberHeight, int _grabberWidth, ofPixelFormat _grabberPixelFormat){
        bufferSize=512;
        playStart=0;
        playEnd=1.0;
        bPauseBuffer=false;
        fps=30;
        GrabberDeviceID= _grabberID;

        //setup grabber
        vGrabber.setPixelFormat(_grabberPixelFormat);
        vGrabber.setDeviceID(GrabberDeviceID);
        vGrabber.initGrabber(_grabberHeight, _grabberWidth);
        vGrabber.setVerbose(true);

        //setup Buffer
        vRate.setup(vGrabber,fps);
        for (int i=0;i<NumBuffer; i++){

            vBuffer.push_back(new ofxPm::VideoBuffer(vRate,NUM_FRAMES));
            bPlayBuffer.push_back(false);

        }

}
//draw function with both grabbers and buffers

void VideoSampler::draw(){

        ofSetColor(255,255,255);

        //draw grabber
        vGrabber.getNextVideoFrame().getTextureRef().draw(320,0,320,240);

        //draw player videoframe
    for (int i; i<vBuffer.size();i++){
    if ((vBuffer[i]->getVideoFrame(playHead)!= NULL)&&(bPlayBuffer[i])){

        vBuffer[i]->getVideoFrame((int)playHead).getTextureRef().draw(640 , 160*i, 160, 120);

    }
    }

        //draw head position
    ofDrawBitmapString("FPS: " + ofToString(int(ofGetFrameRate()))
                       + " || cameraBuffer FPS " + ofToString(vBuffer[0]->getRealFPS())
                       //+ " || videoframes pool size: " + ofToString(VideoFrame::getPoolSize(VideoFormat(640,480,3)))
                       + " || total frames: " +ofToString(NUM_FRAMES),20,ofGetHeight()-40);


    drawPlayerData(playHead/NUM_FRAMES);

}

void VideoSampler::drawCurrentBuffer(int _x, int _y, int _height, int _width){
    if ((vBuffer[currentBufferNum]->getVideoFrame(playHead)!= NULL)&&(bPlayBuffer[currentBufferNum])){

        vBuffer[currentBufferNum]->getVideoFrame((int)playHead).getTextureRef().draw(_x , _y, _height, _width);

    }
}

void VideoSampler::drawBuffer(int _x, int _y, int _height, int _width, int _BufferNum){
    if ((vBuffer[_BufferNum]->getVideoFrame(playHead)!= NULL)&&(bPlayBuffer[_BufferNum])){

        vBuffer[_BufferNum]->getVideoFrame((int)playHead).getTextureRef().draw(_x , _y, _height, _width);

    }
}

void VideoSampler::update(){
    vGrabber.update();

    if (bRecLiveInput){

        vBuffer[currentBufferNum]->resume();
        vBuffer[currentBufferNum]->setFramePos((int)recordPosition);

            //increment recordPosition
        if (recordPosition<NUM_FRAMES-1){

                recordPosition++;

        }else {

                bRecLiveInput=false;
                bPlayAnyBuffer=true;
                bPlayBuffer[currentBufferNum]=true;
                recordPosition=0;

        }

    }
    else{

        vBuffer[currentBufferNum]->stop();

        if (bPlayAnyBuffer){

                updatePlayHead();

        }

    }

    //vRate.setSpeed(speed);


}

void VideoSampler::updatePlayHead(){

    if (!bPauseBuffer){
        if (playHead/NUM_FRAMES<playEnd){

            playHead++;

        }else {

            playHead=playStart*NUM_FRAMES;

            bRecLiveInput=false;
        }
    }


    if(ofGetFrameNum()==100){
        speed = 1.0;
    }

}

float VideoSampler::getRecordPostion(){
    return recordPosition;
}

int VideoSampler::getGrabberDeviceID (){
    return GrabberDeviceID;
}

void VideoSampler::drawPlayerData(float _playheadPerc){



    const float waveformWidth  = ofGetWidth() - 40;
    const float waveformHeight = 300;

    //float top = ofGetHeight() - waveformHeight - 20;
    float top = 500;
    float left = 20;
    float framePosPerc;

    ////////// Video Header Play Pos ///////////////////////
    ofSetColor(255,0,0);
    ofDrawBitmapString("Video Header Play Pos", left, top-10);
    ofLine(left, top, waveformWidth, top);

    // frame pos
    ofSetColor(0,0,255);
    framePosPerc = (float)vBuffer[currentBufferNum]->getFramePos() / (float)NUM_FRAMES;
    ofLine(left+ (framePosPerc * (waveformWidth-left)), top, left+ (framePosPerc * (waveformWidth-left)), top+waveformHeight);
    ofDrawBitmapString("FramePos", left + framePosPerc * waveformWidth-76, top+45);

    ofCircle(left+(framePosPerc*(waveformWidth-left)), top, 10);

    // player frame pos
    ofSetColor(0,255,255);
    framePosPerc = _playheadPerc ;
    ofLine(left+ (framePosPerc * (waveformWidth-left)), top, left+ (framePosPerc * (waveformWidth-left)), top+waveformHeight);
    ofDrawBitmapString("PlayheadPos", left + framePosPerc * waveformWidth-76, top+45);

    ofCircle(left+(framePosPerc*(waveformWidth-left)), top, 10);
}

