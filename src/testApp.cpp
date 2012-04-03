#include "testApp.h"
#include "stdio.h"
#include <iostream>
#include "ofxSimpleGuiToo.h"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>

#include "parseOSC.h"


using namespace std;


int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        if (string(dirp->d_name) != "." && string(dirp->d_name) != "..")
        {
            files.push_back(string(dirp->d_name));
        }
    }
    closedir(dp);
    return 0;
}


//--------------------------------------------------------------
void testApp::setup()
{

    //ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);

    bKinectOk = kinect.setup();
    bCloseKinect = false;
    bOpenKinect = false;
    //kinect.startThread(false, false);

    // camera stuff
    bCameraOk = False;
    camWidth = 640;	// try to grab at this size.
    camHeight = 480;
    camGrabber.setVerbose(true);
    camGrabber.listDevices();
    bCameraOk = camGrabber.initGrabber(camWidth,camHeight);
    camWidth = camGrabber.width;
    camHeight= camGrabber.height;
    printf("camera init asked for 640 by 480 - actual size is %i by %i \n", camWidth, camHeight);
    if (camWidth == 0 || camHeight == 0)
    {
        ofSystemAlertDialog("camera not found, live feed not available");
    }

    /*
    while (!camGrabber.isFrameNew())
    {
        cout << "initializing camera\n";
        camGrabber.grabFrame();
    }
    */

    //double click time
    doubleclickTime = 500;
    lastTap = 0;

    if(ofGetScreenWidth()>1024 && ofGetScreenHeight()>800 )
    {
        WINDOW_W = 1024;
        WINDOW_H = 768;
    }
    else
    {
        WINDOW_W = 800;
        WINDOW_H = 600;
    }

    //we run at 60 fps!
    //ofSetVerticalSync(true);

    // splash image
    bSplash = true;
    splashImg.loadImage("lpmt_splash.png");
    splashTime = ofGetElapsedTimef();

    //mask editing
    maskSetup = false;

    //grid editing
    gridSetup = false;

    // OSC setup
    receiver.setup( PORT );
    current_msg_string = 0;

    // we scan the video dir for videos
    //string videoDir = string("./data/video");
    //string videoDir =  ofToDataPath("video",true);
    //videoFiles = vector<string>();
    //getdir(videoDir,videoFiles);
    //string videos[videoFiles.size()];
    //for (unsigned int i = 0; i < videoFiles.size(); i++)
    //{
    //    videos[i]= videoFiles[i];
    //}

    // we scan the slideshow dir for videos
    //string slideshowDir = string("./data/slideshow");
    string slideshowDir = ofToDataPath("slideshow",true);
    slideshowFolders = vector<string>();
    getdir(slideshowDir,slideshowFolders);
    string slideshows[slideshowFolders.size()];
    for (unsigned int i = 0; i < slideshowFolders.size(); i++)
    {
        slideshows[i]= slideshowFolders[i];
    }


    // load shaders
    edgeBlendShader.load("shaders/blend.vert", "shaders/blend.frag");
    quadMaskShader.load("shaders/mask.vert", "shaders/mask.frag");

    //ttf.loadFont("type/frabk.ttf", 11);
    ttf.loadFont("type/OpenSans-Regular.ttf", 11);
    // set border color for quads in setup mode
    borderColor = 0x666666;
    // starts in quads setup mode
    isSetup = True;
    // starts running
    bStarted = True;
    // default is not using MostPixelsEver
    bMpe = False;
    // starts in windowed mode
    bFullscreen	= 0;
    // gui is on at start
    bGui = 1;
    ofSetWindowShape(WINDOW_W, WINDOW_H);


    // camera stuff
    /*
    bCameraOk = False;
    camWidth = 640;	// try to grab at this size.
    camHeight = 480;
    camGrabber.setVerbose(true);
    camGrabber.listDevices();
    bCameraOk = camGrabber.initGrabber(camWidth,camHeight);
    camWidth = camGrabber.width;
    camHeight= camGrabber.height;
    printf("camera init asked for 640 by 480 - actual size is %i by %i \n", camWidth, camHeight);
    */

    // texture for snapshot background
    snapshotTexture.allocate(camWidth,camHeight, GL_RGB);
    snapshotOn = 0;


    // initializes layers array
    for(int i = 0; i < 36; i++)
    {
        layers[i] = -1;
    }

    // defines the first 4 default quads
    quads[0].setup(0.0,0.0,0.5,0.0,0.5,0.5,0.0,0.5, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
    quads[0].quadNumber = 0;
    quads[1].setup(0.5,0.0,1.0,0.0,1.0,0.5,0.5,0.5, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
    quads[1].quadNumber = 1;
    quads[2].setup(0.0,0.5,0.5,0.5,0.5,1.0,0.0,1.0, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
    quads[2].quadNumber = 2;
    quads[3].setup(0.5,0.5,1.0,0.5,1.0,1.0,0.5,1.0, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
    quads[3].quadNumber = 3;
    // define last one as active quad
    activeQuad = 3;
    quads[activeQuad].isActive = True;
    // number of total quads, to be modified later at each quad insertion
    nOfQuads = 4;
    layers[0] = 0;
    quads[0].layer = 0;
    layers[1] = 1;
    quads[1].layer = 1;
    layers[2] = 2;
    quads[2].layer = 2;
    layers[3] = 3;
    quads[3].layer = 3;



    // GUI STUFF ---------------------------------------------------

    // general page
    gui.addTitle("show/hide quads");
    // overriding default theme
    //gui.bDrawHeader = false;
    gui.config->toggleHeight = 16;
    gui.config->buttonHeight = 18;
    gui.config->sliderTextHeight = 18;
    gui.config->titleHeight = 18;
    //gui.config->fullActiveColor = 0x6B404B;
    //gui.config->fullActiveColor = 0x5E4D3E;
    gui.config->fullActiveColor = 0x648B96;
    gui.config->textColor = 0xFFFFFF;
    gui.config->textBGOverColor = 0xDB6800;
    // adding controls
    // first a general page for toggling layers on/off
    for(int i = 0; i < 36; i++)
    {
        gui.addToggle("surface "+ofToString(i), quads[i].isOn);
    }

    // then two pages of settings for each quad surface
    for(int i = 0; i < 36; i++)
    {
        gui.addPage("surface "+ofToString(i)+" - 1/3");
        gui.addTitle("surface "+ofToString(i));
        gui.addToggle("show/hide", quads[i].isOn);
        gui.addToggle("image on/off", quads[i].imgBg);
        gui.addButton("load image", bImageLoad);
        gui.addSlider("img scale X", quads[i].imgMultX, 0.1, 5.0);
        gui.addSlider("img scale Y", quads[i].imgMultY, 0.1, 5.0);
        gui.addToggle("H mirror", quads[i].imgHFlip);
        gui.addToggle("V mirror", quads[i].imgVFlip);
        gui.addColorPicker("img color", &quads[i].imgColorize.r);
        gui.addTitle("Blending modes");
        gui.addToggle("blending on/off", quads[i].bBlendModes);
        string blendModesArray[] = {"screen","add","subtract","multiply"};
        gui.addComboBox("blending mode", quads[i].blendMode, 4, blendModesArray);
        gui.addTitle("Solid color").setNewColumn(true);
        gui.addToggle("solid bg on/off", quads[i].colorBg);
        gui.addColorPicker("Color", &quads[i].bgColor.r);
        gui.addToggle("transition color", quads[i].transBg);
        gui.addColorPicker("second Color", &quads[i].secondColor.r);
        gui.addSlider("trans duration", quads[i].transDuration, 0.1, 60.0);
        gui.addTitle("Mask");
        gui.addToggle("mask on/off", quads[i].bMask);
        gui.addToggle("invert mask", quads[i].maskInvert);
        gui.addTitle("Surface deformation");
        gui.addToggle("surface deform.", quads[i].bDeform);
        gui.addToggle("use bezier", quads[i].bBezier);
        gui.addToggle("use grid", quads[i].bGrid);
        gui.addSlider("grid rows", quads[i].gridRows, 2, 15);
        gui.addSlider("grid columns", quads[i].gridColumns, 2, 20);
        gui.addButton("spherize light", bQuadBezierSpherize);
        gui.addButton("spherize strong", bQuadBezierSpherizeStrong);
        gui.addButton("reset bezier warp", bQuadBezierReset);
        gui.addTitle("Edge blending").setNewColumn(true);
        gui.addToggle("edge blend on/off", quads[i].edgeBlendBool);
        gui.addSlider("power", quads[i].edgeBlendExponent, 0.0, 4.0);
        gui.addSlider("gamma", quads[i].edgeBlendGamma, 0.0, 4.0);
        gui.addSlider("luminance", quads[i].edgeBlendLuminance, -4.0, 4.0);
        gui.addSlider("left edge", quads[i].edgeBlendAmountSin, 0.0, 0.5);
        gui.addSlider("right edge", quads[i].edgeBlendAmountDx, 0.0, 0.5);
        gui.addSlider("top edge", quads[i].edgeBlendAmountTop, 0.0, 0.5);
        gui.addSlider("bottom edge", quads[i].edgeBlendAmountBottom, 0.0, 0.5);
        gui.addTitle("Content placement");
        gui.addSlider("X displacement", quads[i].quadDispX, -1280, 1280);
        gui.addSlider("Y displacement", quads[i].quadDispY, -1280, 1280);
        gui.addSlider("Width", quads[i].quadW, 0, 2400);
        gui.addSlider("Height", quads[i].quadH, 0, 2400);
        gui.addButton("Reset", bQuadReset);

        gui.addPage("surface "+ofToString(i)+" - 2/3");
        gui.addTitle("Video");
        gui.addToggle("video on/off", quads[i].videoBg);
        //gui.addComboBox("video bg", quads[i].bgVideo, videoFiles.size(), videos);
        gui.addButton("load video", bVideoLoad);
        gui.addSlider("video scale X", quads[i].videoMultX, 0.1, 5.0);
        gui.addSlider("video scale Y", quads[i].videoMultY, 0.1, 5.0);
        gui.addToggle("H mirror", quads[i].videoHFlip);
        gui.addToggle("V mirror", quads[i].videoVFlip);
        gui.addColorPicker("video color", &quads[i].videoColorize.r);
        gui.addSlider("video sound vol", quads[i].videoVolume, 0, 100);
        gui.addSlider("video speed", quads[i].videoSpeed, -2.0, 4.0);
        gui.addToggle("video loop", quads[i].videoLoop);
        gui.addToggle("video greenscreen", quads[i].videoGreenscreen);
        if (camWidth > 0)
        {
            gui.addTitle("Camera").setNewColumn(true);
            gui.addToggle("cam on/off", quads[i].camBg);
            gui.addSlider("camera scale X", quads[i].camMultX, 0.1, 5.0);
            gui.addSlider("camera scale Y", quads[i].camMultY, 0.1, 5.0);
            gui.addToggle("H mirror", quads[i].camHFlip);
            gui.addToggle("V mirror", quads[i].camVFlip);
            gui.addColorPicker("cam color", &quads[i].camColorize.r);
            gui.addToggle("camera greenscreen", quads[i].camGreenscreen);
            gui.addTitle("Greenscreen");
        }
        else
        {
            gui.addTitle("Greenscreen").setNewColumn(true);
        }
        gui.addSlider("g-screen threshold", quads[i].thresholdGreenscreen, 0, 128);
        gui.addColorPicker("greenscreen col", &quads[i].colorGreenscreen.r);
        gui.addTitle("Slideshow").setNewColumn(false);
        gui.addToggle("slideshow on/off", quads[i].slideshowBg);
        gui.addComboBox("slideshow folder", quads[i].bgSlideshow, slideshowFolders.size(), slideshows);
        gui.addSlider("slide duration", quads[i].slideshowSpeed, 0.1, 15.0);
        gui.addToggle("slides to quad size", quads[i].slideFit);
        gui.addToggle("keep aspect ratio", quads[i].slideKeepAspect);
        if(bKinectOk)
        {
            gui.addTitle("Kinect").setNewColumn(true);
            gui.addToggle("use kinect", quads[i].kinectBg);
            gui.addToggle("show kinect image", quads[i].kinectImg);
            gui.addToggle("show kinect gray image", quads[i].getKinectGrayImage);
            gui.addToggle("use kinect as mask", quads[i].kinectMask);
            gui.addToggle("kinect blob detection", quads[i].getKinectContours);
            gui.addToggle("blob curved contour", quads[i].kinectContourCurved);
            gui.addSlider("kinect scale X", quads[i].kinectMultX, 0.1, 5.0);
            gui.addSlider("kinect scale Y", quads[i].kinectMultY, 0.1, 5.0);
            gui.addColorPicker("kinect color", &quads[i].kinectColorize.r);
            gui.addSlider("near threshold", quads[i].nearDepthTh, 0, 255);
            gui.addSlider("far threshold", quads[i].farDepthTh, 0, 255);
            gui.addSlider("kinect tilt angle", kinect.kinectAngle, -30, 30);
            gui.addSlider("kinect image blur", quads[i].kinectBlur, 0, 10);
            gui.addSlider("blob min area", quads[i].kinectContourMin, 0.01, 1.0);
            gui.addSlider("blob max area", quads[i].kinectContourMax, 0.0, 1.0);
            gui.addSlider("blob contour smooth", quads[i].kinectContourSmooth, 0, 20);
            gui.addSlider("blob simplify", quads[i].kinectContourSimplify, 0.0, 2.0);
            gui.addButton("close connection", bCloseKinect);
            gui.addButton("reopen connection", bOpenKinect);

        }

        gui.addPage("surface "+ofToString(i)+" - 3/3");
        gui.addTitle("Corner 0");
        gui.addSlider("X", quads[i].corners[0].x, -1.0, 2.0);
        gui.addSlider("Y", quads[i].corners[0].y, -1.0, 2.0);
        gui.addTitle("Corner 3");
        gui.addSlider("X", quads[i].corners[3].x, -1.0, 2.0);
        gui.addSlider("Y", quads[i].corners[3].y, -1.0, 2.0);
        gui.addTitle("Corner 1").setNewColumn(true);
        gui.addSlider("X", quads[i].corners[1].x, -1.0, 2.0);
        gui.addSlider("Y", quads[i].corners[1].y, -1.0, 2.0);
        gui.addTitle("Corner 2");
        gui.addSlider("X", quads[i].corners[2].x, -1.0, 2.0);
        gui.addSlider("Y", quads[i].corners[2].y, -1.0, 2.0);
    }

    // then we set displayed gui page to the one corresponding to active quad and show the gui
    gui.setPage((activeQuad*3)+2);
    gui.show();

}


void testApp::openImageFile()
{
    ofFileDialogResult dialog_result = ofSystemLoadDialog("load image file", false);
    if(dialog_result.bSuccess)
    {
        quads[activeQuad].loadImageFromFile(dialog_result.getName(), dialog_result.getPath());
    }
}

void testApp::openVideoFile()
{
    ofFileDialogResult dialog_result = ofSystemLoadDialog("load video file", false);
    if(dialog_result.bSuccess)
    {
        quads[activeQuad].loadVideoFromFile(dialog_result.getName(), dialog_result.getPath());
    }
}

void testApp::mpeSetup()
{
    stopProjection();
    bMpe = True;
    // MPE stuff
    lastFrameTime = ofGetElapsedTimef();
    client.setup("mpe_client_settings.xml", true); //false means you can use backthread
    ofxMPERegisterEvents(this);
    //resync();
    startProjection();
    client.start();
    ofSetBackgroundAuto(false);
}


//--------------------------------------------------------------
void testApp::prepare()
{
    // check for waiting OSC messages
    while( receiver.hasWaitingMessages() )
    {
        parseOsc();
    }

    if (bStarted)
    {

        //check if quad dimensions reset button on GUI is pressed
        if(bQuadReset)
        {
            bQuadReset = false;
            quadDimensionsReset(activeQuad);
            quadPlacementReset(activeQuad);
        }

        //check if quad bezier spherize button on GUI is pressed
        if(bQuadBezierSpherize)
        {
            bQuadBezierSpherize = false;
            quadBezierSpherize(activeQuad);
        }

        //check if quad bezier spherize strong button on GUI is pressed
        if(bQuadBezierSpherizeStrong)
        {
            bQuadBezierSpherizeStrong = false;
            quadBezierSpherizeStrong(activeQuad);
        }

        //check if quad bezier reset button on GUI is pressed
        if(bQuadBezierReset)
        {
            bQuadBezierReset = false;
            quadBezierReset(activeQuad);
        }


        // check if image load button on GUI is pressed
        if(bImageLoad)
        {
            bImageLoad = false;
            openImageFile();
        }

        // check if image load button on GUI is pressed
        if(bVideoLoad)
        {
            bVideoLoad = false;
            openVideoFile();
        }

        // check if kinect close button on GUI is pressed
        if(bCloseKinect)
        {
            bCloseKinect = false;
            kinect.kinect.setCameraTiltAngle(0);
            kinect.kinect.close();
        }

        // check if kinect close button on GUI is pressed
        if(bOpenKinect)
        {
            bOpenKinect = false;
            kinect.kinect.open();
        }


        if (camGrabber.getHeight() > 0)  // isLoaded check
        {
            camGrabber.grabFrame();
        }


        // sets default window background, grey in setup mode and black in projection mode
        if (isSetup)
        {
            ofBackground(20, 20, 20);
        }
        else
        {
            ofBackground(0, 0, 0);
        }
        //ofSetWindowShape(800, 600);

        // kinect update
        kinect.update();

        // loops through initialized quads and runs update, setting the border color as well
        for(int j = 0; j < 36; j++)
        {
            int i = layers[j];
            if (i >= 0)
            {
                if (quads[i].initialized)
                {
                    quads[i].update();
                    quads[i].borderColor = borderColor;
                }
            }
        }

    }
}


//--------------------------------------------------------------
void testApp::dostuff()
{
    if (bStarted)
    {

        // if snapshot is on draws it as window background
        if (isSetup && snapshotOn)
        {
            ofEnableAlphaBlending();
            ofSetHexColor(0xFFFFFF);
            snapshotTexture.draw(0,0,ofGetWidth(),ofGetHeight());
            ofDisableAlphaBlending();
        }

        // loops through initialized quads and calls their draw function
        for(int j = 0; j < 36; j++)
        {
            int i = layers[j];
            if (i >= 0)
            {
                if (quads[i].initialized)
                {
                    quads[i].draw();
                }
            }
        }

    }
}

//--------------------------------------------------------------
void testApp::update()
{

    if (!bMpe)
    {
        if (bSplash)
        {
            if (abs(splashTime - ofGetElapsedTimef()) > 8.0)
            {
                bSplash = ! bSplash;
            }
        }
        prepare();
    }
}

//--------------------------------------------------------------
void testApp::draw()
{
    // in setup mode writes the number of active quad at the bottom of the window
    if (isSetup)
    {
        // in setup mode sets active quad border to be white
        quads[activeQuad].borderColor = 0xFFFFFF;
    }

    if (!bMpe)
    {
        dostuff();
    }

    if (isSetup)
    {

        if (bStarted)
        {

            ofSetHexColor(0xFFFFFF);
            ttf.drawString("active surface: "+ofToString(activeQuad), 30, ofGetHeight()-25);
            if(maskSetup)
            {
                ofSetHexColor(0xFF0000);
                ttf.drawString("Mask-editing mode ", 170, ofGetHeight()-25);
            }
            // draws gui
            gui.draw();
        }
    }

    if (bSplash)
    {
        ofEnableAlphaBlending();
        splashImg.draw(((ofGetWidth()/2)-230),((ofGetHeight()/2)-110));
        ofDisableAlphaBlending();
    }
}


//--------------------------------------------------------------
void testApp::mpeFrameEvent(ofxMPEEventArgs& event)
{
    if (bMpe)
    {
        if(client.getFrameCount()<=1)
        {
            resync();
        }
        prepare();
        dostuff();
    }
}

//--------------------------------------------------------------
void testApp::mpeMessageEvent(ofxMPEEventArgs& event)
{
    //received a message from the server
}


void testApp::mpeResetEvent(ofxMPEEventArgs& event)
{
    //triggered if the server goes down, another client goes offline, or a reset was manually triggered in the server code
}



//--------------------------------------------------------------
void testApp::keyPressed(int key)
{

    // moves active layer one position up
    if ( key == '+')
    {
        int position;
        int target;

        for(int i = 0; i < 35; i++)
        {
            if (layers[i] == quads[activeQuad].quadNumber)
            {
                position = i;
                target = i+1;
            }

        }
        if (layers[target] != -1)
        {
            int target_content = layers[target];
            layers[target] = quads[activeQuad].quadNumber;
            layers[position] = target_content;
            quads[activeQuad].layer = target;
            quads[target_content].layer = position;
        }
    }


    // moves active layer one position down
    if ( key == '-')
    {
        int position;
        int target;

        for(int i = 0; i < 36; i++)
        {
            if (layers[i] == quads[activeQuad].quadNumber)
            {
                position = i;
                target = i-1;
            }

        }
        if (target >= 0)
        {
            if (layers[target] != -1)
            {
                int target_content = layers[target];
                layers[target] = quads[activeQuad].quadNumber;
                layers[position] = target_content;
                quads[activeQuad].layer = target;
                quads[target_content].layer = position;
            }
        }
    }


    // saves quads settings to an xml file in data directory
    if ( key == 's' || key == 'S')
    {
        setXml();
        XML.saveFile("_lpmt_settings.xml");
        cout<<"saved settings to data/_lpmt_settings.xml"<<endl;

    }

    // loads settings and quads from default xml file
    if (key == 'l' || key == 'L')
    {
        XML.loadFile("_lpmt_settings.xml");
        getXml();
        cout<<"loaded settings from data/_lpmt_settings.xml"<<endl;
        gui.setPage((activeQuad*3)+2);
    }

    // takes a snapshot of attached camera and uses it as window background
    if (key == 'w')
    {
        snapshotOn = !snapshotOn;
        if (snapshotOn == 1)
        {
            camGrabber.grabFrame();
            snapshotTexture.allocate(camWidth,camHeight, GL_RGB);
            unsigned char * pixels = camGrabber.getPixels();
            snapshotTexture.loadData(pixels, camWidth,camHeight, GL_RGB);
        }
    }

    // takes a snapshot from an image file and uses it as window background
    if (key == 'W')
    {
        snapshotOn = !snapshotOn;
        if (snapshotOn == 1)
        {
            ofImage img;
            img.clone(loadImageFromFile());
            snapshotTexture.allocate(img.width,img.height, GL_RGB);
            unsigned char * pixels = img.getPixels();
            snapshotTexture.loadData(pixels, img.width,img.height, GL_RGB);
        }
    }

    // fills window with active quad
    if ( key =='q' || key == 'Q' )
    {
        if (isSetup)
        {
            quads[activeQuad].corners[0].x = 0.0;
            quads[activeQuad].corners[0].y = 0.0;

            quads[activeQuad].corners[1].x = 1.0;
            quads[activeQuad].corners[1].y = 0.0;

            quads[activeQuad].corners[2].x = 1.0;
            quads[activeQuad].corners[2].y = 1.0;

            quads[activeQuad].corners[3].x = 0.0;
            quads[activeQuad].corners[3].y = 1.0;
        }
    }

    // activates next quad
    if ( key =='>' )
    {
        if (isSetup)
        {
            quads[activeQuad].isActive = False;
            activeQuad += 1;
            if (activeQuad > nOfQuads-1)
            {
                activeQuad = 0;
            }
            quads[activeQuad].isActive = True;
        }
        gui.setPage((activeQuad*3)+2);
    }

    // activates prev quad
    if ( key =='<' )
    {
        if (isSetup)
        {
            quads[activeQuad].isActive = False;
            activeQuad -= 1;
            if (activeQuad < 0)
            {
                activeQuad = nOfQuads-1;
            }
            quads[activeQuad].isActive = True;
        }
        gui.setPage((activeQuad*3)+2);
    }

    // goes to first page of gui for active quad or, in mask edit mode, delete last drawn point
    if ( key == 'z' || key == 'Z')
    {
        if(maskSetup && quads[activeQuad].maskPoints.size()>0)
        {
            quads[activeQuad].maskPoints.pop_back();
        }
        else
        {
            gui.setPage((activeQuad*3)+2);
        }
    }

    if ( key == OF_KEY_F1)
    {
        gui.setPage((activeQuad*3)+2);
    }

    if ( key == 'd' || key == 'D')
    {
        if(maskSetup && quads[activeQuad].maskPoints.size()>0)
        {
            if (quads[activeQuad].bHighlightMaskPoint)
            {
                quads[activeQuad].maskPoints.erase(quads[activeQuad].maskPoints.begin()+quads[activeQuad].highlightedMaskPoint);
            }

        }
    }


    // goes to second page of gui for active quad
    if ( key == 'x' || key == 'X' || key == OF_KEY_F2)
    {
        gui.setPage((activeQuad*3)+3);
    }

    // goes to second page of gui for active quad or, in edit mask mode, clears mask
    if ( key == 'c' || key == 'C')
    {

        if(maskSetup)
        {
            quads[activeQuad].maskPoints.clear();
        }
        else
        {
            gui.setPage((activeQuad*3)+4);
        }
    }

    if (key == OF_KEY_F3)
    {
        gui.setPage((activeQuad*3)+4);
    }


    // adds a new quad in the middle of the screen
    if ( key =='a' )
    {
        if (isSetup)
        {
            if (nOfQuads < 36)
            {
                quads[nOfQuads].setup(0.25,0.25,0.75,0.25,0.75,0.75,0.25,0.75, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
                quads[nOfQuads].quadNumber = nOfQuads;
                layers[nOfQuads] = nOfQuads;
                quads[nOfQuads].layer = nOfQuads;
                quads[activeQuad].isActive = False;
                activeQuad = nOfQuads;
                quads[activeQuad].isActive = True;
                ++nOfQuads;
                quads[activeQuad].allocateFbo(ofGetWidth(),ofGetHeight());
                gui.setPage((activeQuad*3)+2);
                //gui.show(); // bad workaround for image disappearing bug when adding quad and gui is off
                if (!bGui)
                {
                    gui.toggleDraw();
                    bGui = !bGui;
                }
            }
        }
    }

    // toggles setup mode
    if ( key ==' ' )
    {
        if (isSetup)
        {
            isSetup = False;
            gui.hide();
            bGui = False;
            for(int i = 0; i < 36; i++)
            {
                if (quads[i].initialized)
                {
                    quads[i].isSetup = False;
                }
            }
        }
        else
        {
            isSetup = True;
            gui.show();
            bGui = True;
            for(int i = 0; i < 36; i++)
            {
                if (quads[i].initialized)
                {
                    quads[i].isSetup = True;
                }
            }
        }
    }

    // toggles fullscreen mode
    if(key == 'f')
    {

        bFullscreen = !bFullscreen;

        if(!bFullscreen)
        {
            ofSetWindowShape(WINDOW_W, WINDOW_H);
            ofSetFullscreen(false);
            // figure out how to put the window in the center:
            int screenW = ofGetScreenWidth();
            int screenH = ofGetScreenHeight();
            ofSetWindowPosition(screenW/2-WINDOW_W/2, screenH/2-WINDOW_H/2);
        }
        else if(bFullscreen == 1)
        {
            ofSetFullscreen(true);
        }
    }

    // toggles gui
    if(key == 'g')
    {
        if (maskSetup)
        {
            maskSetup = False;
            for(int i = 0; i < 36; i++)
            {
                if (quads[i].initialized)
                {
                    quads[i].isMaskSetup = False;
                }
            }
        }
        gui.toggleDraw();
        bGui = !bGui;
    }

    // toggles mask editing
    if(key == 'm')
    {
        if (!bGui)
        {
            maskSetup = !maskSetup;
            for(int i = 0; i < 36; i++)
            {
                if (quads[i].initialized)
                {
                    quads[i].isMaskSetup = !quads[i].isMaskSetup;
                }
            }
        }
    }

    // toggles bezier deformation editing
    if(key == 'b')
    {
        if (!bGui)
        {
            gridSetup = !gridSetup;
            for(int i = 0; i < 36; i++)
            {
                if (quads[i].initialized)
                {
                    quads[i].isBezierSetup = !quads[i].isBezierSetup;
                }
            }
        }
    }

    if(key == '[')
    {
        gui.prevPage();
    }

    if(key == ']')
    {
        gui.nextPage();
    }

    // show general settings page of gui
    if(key == '1')
    {
        gui.setPage(1);
    }

    // resyncs videos to start point in every quad
    if(key == 'r' || key == 'R')
    {
        resync();
    }


    // starts and stops rendering

    if(key == 'p')
    {
        startProjection();
    }

    if(key == 'o')
    {
        stopProjection();
    }

    if(key == 'n')
    {
        mpeSetup();
    }

    // displays help in system dialog
    if(key == 'h')
    {
        ofBuffer buf = ofBufferFromFile("help_keys.txt");
        ofSystemAlertDialog(buf);
    }

}

//--------------------------------------------------------------
void testApp::keyReleased(int key)
{
}


//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y )
{

    if (isSetup && !bGui && !maskSetup && !gridSetup)
    {
        float smallestDist = 1.0;
        whichCorner = -1;

        for(int i = 0; i < 4; i++)
        {
            float distx = quads[activeQuad].corners[i].x - (float)x/ofGetWidth();
            float disty = quads[activeQuad].corners[i].y - (float)y/ofGetHeight();
            float dist  = sqrt( distx * distx + disty * disty);

            if(dist < smallestDist && dist < 0.1)
            {
                whichCorner = i;
                smallestDist = dist;
            }
        }

        if(whichCorner >= 0)
        {
            quads[activeQuad].bHighlightCorner = True;
            quads[activeQuad].highlightedCorner = whichCorner;
        }
        else
        {
            quads[activeQuad].bHighlightCorner = False;
            quads[activeQuad].highlightedCorner = -1;
        }
    }

    else if (maskSetup && !gridSetup)
    {
        float smallestDist = sqrt( ofGetWidth() * ofGetWidth() + ofGetHeight() * ofGetHeight());;
        int whichPoint = -1;
        ofVec3f warped;
        for(int i = 0; i < quads[activeQuad].maskPoints.size(); i++)
        {
            warped = quads[activeQuad].getWarpedPoint(x,y);
            float distx = (float)quads[activeQuad].maskPoints[i].x - (float)warped.x;
            float disty = (float)quads[activeQuad].maskPoints[i].y - (float)warped.y;
            float dist  = sqrt( distx * distx + disty * disty);

            if(dist < smallestDist && dist < 20.0)
            {
                whichPoint = i;
                smallestDist = dist;
            }
        }
        if(whichPoint >= 0)
        {
            quads[activeQuad].bHighlightMaskPoint = True;
            quads[activeQuad].highlightedMaskPoint = whichPoint;
        }
        else
        {
            quads[activeQuad].bHighlightMaskPoint = False;
            quads[activeQuad].highlightedMaskPoint = -1;
        }
    }

    else if (gridSetup && !maskSetup)
    {
        float smallestDist = sqrt( ofGetWidth() * ofGetWidth() + ofGetHeight() * ofGetHeight());;
        int whichPointRow = -1;
        int whichPointCol = -1;
        ofVec3f warped;

        if(quads[activeQuad].bBezier)
        {
            for(int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    warped = quads[activeQuad].getWarpedPoint(x,y);
                    float distx = (float)quads[activeQuad].bezierPoints[i][j][0] * ofGetWidth() - (float)warped.x;
                    float disty = (float)quads[activeQuad].bezierPoints[i][j][1] * ofGetHeight() - (float)warped.y;
                    float dist  = sqrt( distx * distx + disty * disty);

                    if(dist < smallestDist && dist < 20.0)
                    {
                        whichPointRow = i;
                        whichPointCol = j;
                        smallestDist = dist;
                    }
                }
            }
        }

        else if(quads[activeQuad].bGrid)
        {
            for(int i = 0; i <= quads[activeQuad].gridRows; i++)
            {
                for (int j = 0; j <= quads[activeQuad].gridColumns; j++)
                {
                    warped = quads[activeQuad].getWarpedPoint(x,y);
                    float distx = (float)quads[activeQuad].gridPoints[i][j][0] * ofGetWidth() - (float)warped.x;
                    float disty = (float)quads[activeQuad].gridPoints[i][j][1] * ofGetHeight() - (float)warped.y;
                    float dist  = sqrt( distx * distx + disty * disty);

                    if(dist < smallestDist && dist < 20.0)
                    {
                        whichPointRow = i;
                        whichPointCol = j;
                        smallestDist = dist;
                    }
                }
            }
        }

        if(whichPointRow >= 0)
        {
            quads[activeQuad].bHighlightCtrlPoint = True;
            quads[activeQuad].highlightedCtrlPointRow = whichPointRow;
            quads[activeQuad].highlightedCtrlPointCol = whichPointCol;
        }
        else
        {
            quads[activeQuad].bHighlightCtrlPoint = False;
            quads[activeQuad].highlightedCtrlPointRow = -1;
            quads[activeQuad].highlightedCtrlPointCol = -1;
        }
    }
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{
    if (isSetup && !bGui && !maskSetup && !gridSetup)
    {

        float scaleX = (float)x / ofGetWidth();
        float scaleY = (float)y / ofGetHeight();

        if(whichCorner >= 0)
        {
            quads[activeQuad].corners[whichCorner].x = scaleX;
            quads[activeQuad].corners[whichCorner].y = scaleY;
        }
        // check if we can move whole quad by dragging its center
        else
        {
            float distx = quads[activeQuad].center.x - scaleX;
            float disty = quads[activeQuad].center.y - scaleY;
            float dist  = sqrt( distx * distx + disty * disty);
            if(dist < 0.1) // TODO: verifiy if threshold value is good for distance
            {
                ofPoint mouse;
                ofPoint movement;
                mouse.x = x;
                mouse.y = y;
                movement = mouse-startDrag;

                for(int i = 0; i < 4; i++)
                {
                    quads[activeQuad].corners[i].x = quads[activeQuad].corners[i].x + ((float)movement.x / ofGetWidth());
                    quads[activeQuad].corners[i].y = quads[activeQuad].corners[i].y + ((float)movement.y / ofGetHeight());
                }
                startDrag.x = x;
                startDrag.y = y;
            }
        }
    }
    else if(maskSetup && quads[activeQuad].bHighlightMaskPoint)
    {
        ofVec3f punto;
        punto = quads[activeQuad].getWarpedPoint(x,y);
        quads[activeQuad].maskPoints[quads[activeQuad].highlightedMaskPoint].x = punto.x;
        quads[activeQuad].maskPoints[quads[activeQuad].highlightedMaskPoint].y = punto.y;
    }

    else if(gridSetup && quads[activeQuad].bHighlightCtrlPoint)
    {
        if(quads[activeQuad].bBezier)
        {
            ofVec3f punto;
            punto = quads[activeQuad].getWarpedPoint(x,y);
            quads[activeQuad].bezierPoints[quads[activeQuad].highlightedCtrlPointRow][quads[activeQuad].highlightedCtrlPointCol][0] = (float)punto.x/ofGetWidth();
            quads[activeQuad].bezierPoints[quads[activeQuad].highlightedCtrlPointRow][quads[activeQuad].highlightedCtrlPointCol][1] = (float)punto.y/ofGetHeight();
        }
        else if(quads[activeQuad].bGrid)
        {
            ofVec3f punto;
            punto = quads[activeQuad].getWarpedPoint(x,y);
            quads[activeQuad].gridPoints[quads[activeQuad].highlightedCtrlPointRow][quads[activeQuad].highlightedCtrlPointCol][0] = (float)punto.x/ofGetWidth();
            quads[activeQuad].gridPoints[quads[activeQuad].highlightedCtrlPointRow][quads[activeQuad].highlightedCtrlPointCol][1] = (float)punto.y/ofGetHeight();
        }
    }
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{
    // this is used for dragging the whole quad using its centroid
    startDrag.x = x;
    startDrag.y = y;

    if(bSplash)
    {
        bSplash = !bSplash;
    }

    if (isSetup && !bGui)
    {

        if(maskSetup && !gridSetup)
        {
            if (!quads[activeQuad].bHighlightMaskPoint)
            {
                quads[activeQuad].maskAddPoint(x, y);
            }
        }

        else
        {
            float smallestDist = 1.0;
            whichCorner = -1;
            unsigned long curTap = ofGetElapsedTimeMillis();
            if(lastTap != 0 && curTap - lastTap < doubleclickTime)
            {
                activateQuad(x,y);
            }
            lastTap = curTap;

            // check if we are near once of active quad's corners
            for(int i = 0; i < 4; i++)
            {
                float distx = quads[activeQuad].corners[i].x - (float)x/ofGetWidth();
                float disty = quads[activeQuad].corners[i].y - (float)y/ofGetHeight();
                float dist  = sqrt( distx * distx + disty * disty);

                if(dist < smallestDist && dist < 0.1)
                {
                    whichCorner = i;
                    smallestDist = dist;
                }
            }
        }
    }
}

//--------------------------------------------------------------
void testApp::mouseReleased()
{
    if (isSetup && !bGui)
    {

        if (whichCorner >= 0)
        {
            // snap detection for near quads
            float smallestDist = 1.0;
            int snapQuad = -1;
            int snapCorner = -1;
            for (int i = 0; i < 36; i++)
            {
                if ( i != activeQuad && quads[i].initialized)
                {
                    for(int j = 0; j < 4; j++)
                    {
                        float distx = quads[activeQuad].corners[whichCorner].x - quads[i].corners[j].x;
                        float disty = quads[activeQuad].corners[whichCorner].y - quads[i].corners[j].y;
                        float dist = sqrt( distx * distx + disty * disty);
                        // to tune snapping change dist value inside next if statement
                        if (dist < smallestDist && dist < 0.0075)
                        {
                            snapQuad = i;
                            snapCorner = j;
                            smallestDist = dist;
                        }
                    }
                }
            }
            if (snapQuad >= 0 && snapCorner >= 0)
            {
                quads[activeQuad].corners[whichCorner].x = quads[snapQuad].corners[snapCorner].x;
                quads[activeQuad].corners[whichCorner].y = quads[snapQuad].corners[snapCorner].y;
            }
        }
        whichCorner = -1;
        quads[activeQuad].bHighlightCorner = False;
    }
}


void testApp::windowResized(int w, int h)
{
    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {
            quads[i].bHighlightCorner = False;
            quads[i].allocateFbo(ofGetWidth(),ofGetHeight());
            quadDimensionsReset(i);
        }
    }
}



//--------------------------------------------------------------
void testApp::resync()
{
    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {
            // resets video to start ing point
            if (quads[i].videoBg && quads[i].video.isLoaded())
            {
                quads[i].video.setPosition(0.0);
            }
            // resets slideshow to first slide
            if (quads[i].slideshowBg)
            {
                quads[i].currentSlide = 0;
                quads[i].slideTimer = 0;
            }
            // reset trans colors
            if (quads[i].colorBg && quads[i].transBg)
            {
                quads[i].transCounter = 0;
                quads[i].transUp = True;
            }
        }
    }
}

//--------------------------------------------------------------
void testApp::startProjection()
{
    bStarted = True;
    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {
            quads[i].isOn = True;
            if (quads[i].videoBg && quads[i].video.isLoaded())
            {
                quads[i].video.setVolume(quads[i].videoVolume);
                quads[i].video.play();
            }
        }
    }
}

//--------------------------------------------------------------
void testApp::stopProjection()
{
    bStarted = False;
    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {
            quads[i].isOn = False;
            if (quads[i].videoBg && quads[i].video.isLoaded())
            {
                quads[i].video.setVolume(0);
                quads[i].video.stop();
            }
        }
    }
}


//---------------------------------------------------------------
void testApp::quadDimensionsReset(int q)
{
    quads[q].quadW = ofGetWidth();
    quads[q].quadH = ofGetHeight();
}

//---------------------------------------------------------------
void testApp::quadPlacementReset(int q)
{
    quads[q].quadDispX = 0;
    quads[q].quadDispY = 0;
}

//---------------------------------------------------------------
void testApp::quadBezierSpherize(int q)
{
    float w = (float)ofGetWidth();
    float h = (float)ofGetHeight();
    float k = (sqrt(2)-1)*4/3;
    quads[q].bBezier = true;
    quads[q].bezierPoints =
    {
        {   {0*h/w+(0.5*(w/h-1))*h/w, 0, 0},          {0.5*k*h/w+(0.5*(w/h-1))*h/w, -0.5*k, 0},    {(1.0*h/w)-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, -0.5*k, 0},    {1.0*h/w+(0.5*(w/h-1))*h/w, 0, 0}    },
        {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0},        {0*h/w+(0.5*(w/h-1))*h/w, 0, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 0, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0}  },
        {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0},        {0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0}  },
        {   {0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},        {0.5*k*h/w+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {(1.0*h/w)-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0}  }
    };

    /*  quads[q].bezierPoints =
      {
          {   {(0.5*w/h-0.5)*h/w, 0, 0},  {0.5*(k+w/h-1)*h/w, -0.5*k, 0},    {0.5*(1-k+w/h)*h/w, -0.5*k, 0},    {1.0*h/w+(0.5*(w/h-1))*h/w, 0, 0}    },
          {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0},        {0*h/w+(0.5*(w/h-1))*h/w, 0, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 0, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0}  },
          {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0},        {0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0}  },
          {   {0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},        {0.5*k*h/w+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {(1.0*h/w)-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0}  }
      }; */
}

//---------------------------------------------------------------
void testApp::quadBezierSpherizeStrong(int q)
{
    float w = (float)ofGetWidth();
    float h = (float)ofGetHeight();
    float k = (sqrt(2)-1)*4/3;
    quads[q].bBezier = true;
    quads[q].bezierPoints =
    {
        {   {0*h/w+(0.5*(w/h-1))*h/w, 0, 0},  {0.5*k*h/w+(0.5*(w/h-1))*h/w, -0.5*k, 0},    {(1.0*h/w)-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, -0.5*k, 0},    {1.0*h/w+(0.5*(w/h-1))*h/w, 0, 0}    },
        {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0},        {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, -0.5*k, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, -0.5*k, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 0.5*k, 0}  },
        {   {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0},        {0*h/w-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {1.0*h/w+(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0-0.5*k, 0}  },
        {   {0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0},        {0.5*k*h/w+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {(1.0*h/w)-(0.5*k*h/w)+(0.5*(w/h-1))*h/w, 1.0+0.5*k, 0},  {1.0*h/w+(0.5*(w/h-1))*h/w, 1.0, 0}  }
    };
}

//---------------------------------------------------------------
void testApp::quadBezierReset(int q)
{
    quads[q].bBezier = true;
    quads[q].bezierPoints =
    {
        {   {0, 0, 0},          {0.333, 0, 0},    {0.667, 0, 0},    {1.0, 0, 0}    },
        {   {0, 0.333, 0},        {0.333, 0.333, 0},  {0.667, 0.333, 0},  {1.0, 0.333, 0}  },
        {   {0, 0.667, 0},        {0.333, 0.667, 0},  {0.667, 0.667, 0},  {1.0, 0.667, 0}  },
        {   {0, 1.0, 0},        {0.333, 1.0, 0},  {0.667, 1.0, 0},  {1.0, 1.0, 0}  }
    };
}


//---------------------------------------------------------------
void testApp::activateQuad(int x, int y)
{
    float smallestDist = 1.0;
    int whichQuad = activeQuad;

    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {
            float distx = quads[i].center.x - (float)x/ofGetWidth();
            float disty = quads[i].center.y - (float)y/ofGetHeight();
            float dist  = sqrt( distx * distx + disty * disty);

            if(dist < smallestDist && dist < 0.1)
            {
                whichQuad = i;
                smallestDist = dist;
            }
        }
    }
    if (whichQuad != activeQuad)
    {
        quads[activeQuad].isActive = False;
        activeQuad = whichQuad;
        quads[activeQuad].isActive = True;
        gui.setPage((activeQuad*3)+2);
    }
}


//-----------------------------------------------------------
ofImage testApp::loadImageFromFile()
{
    ofFileDialogResult dialog_result = ofSystemLoadDialog("load image file", false);
    if(dialog_result.bSuccess)
    {
        ofImage img;
        string imgName = dialog_result.getName();
        string imgPath = dialog_result.getPath();
        ofFile image(imgPath);
        img.loadImage(image);
        return img;
    }

}





//--------------------------------------------------------------

void testApp::setXml()

{
    XML.setValue("GENERAL:ACTIVE_QUAD",activeQuad);
    XML.setValue("GENERAL:N_OF_QUADS",nOfQuads);

    for(int i = 0; i < 36; i++)
    {
        if (quads[i].initialized)
        {

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":NUMBER",quads[i].quadNumber);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":LAYER",quads[i].layer);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:DISPX",quads[i].quadDispX);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:DISPY",quads[i].quadDispY);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:WIDTH",quads[i].quadW);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:HEIGHT",quads[i].quadH);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:LOADED_IMG",quads[i].loadedImg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:LOADED_IMG_PATH",quads[i].bgImg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOADED_VIDEO",quads[i].loadedVideo);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOADED_VIDEO_PATH",quads[i].bgVideo);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:LOADED_SLIDESHOW",quads[i].bgSlideshow);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_0:X",quads[i].corners[0].x);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_0:Y",quads[i].corners[0].y);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_1:X",quads[i].corners[1].x);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_1:Y",quads[i].corners[1].y);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_2:X",quads[i].corners[2].x);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_2:Y",quads[i].corners[2].y);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_3:X",quads[i].corners[3].x);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_3:Y",quads[i].corners[3].y);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IS_ON",quads[i].isOn);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:ACTIVE",quads[i].colorBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:TRANS:ACTIVE",quads[i].transBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:TRANS:DURATION",quads[i].transDuration);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:ACTIVE",quads[i].slideshowBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:SPEED",quads[i].slideshowSpeed);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:FIT",quads[i].slideFit);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:KEEP_ASPECT",quads[i].slideKeepAspect);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:ACTIVE",quads[i].camBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:ACTIVE",quads[i].imgBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:ACTIVE",quads[i].videoBg);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:WIDTH",quads[i].camWidth);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:HEIGHT",quads[i].camHeight);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:MULT_X",quads[i].camMultX);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:MULT_Y",quads[i].camMultY);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:MULT_X",quads[i].imgMultX);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:MULT_Y",quads[i].imgMultY);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:MULT_X",quads[i].videoMultX);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:MULT_Y",quads[i].videoMultY);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:SPEED",quads[i].videoSpeed);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:VOLUME",quads[i].videoVolume);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOOP",quads[i].videoLoop);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:R",quads[i].bgColor.r);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:G",quads[i].bgColor.g);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:B",quads[i].bgColor.b);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:A",quads[i].bgColor.a);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:R",quads[i].secondColor.r);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:G",quads[i].secondColor.g);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:B",quads[i].secondColor.b);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:A",quads[i].secondColor.a);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:R",quads[i].imgColorize.r);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:G",quads[i].imgColorize.g);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:B",quads[i].imgColorize.b);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:A",quads[i].imgColorize.a);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:R",quads[i].videoColorize.r);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:G",quads[i].videoColorize.g);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:B",quads[i].videoColorize.b);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:A",quads[i].videoColorize.a);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:R",quads[i].camColorize.r);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:G",quads[i].camColorize.g);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:B",quads[i].camColorize.b);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:A",quads[i].camColorize.a);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:FLIP:H",quads[i].imgHFlip);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":IMG:FLIP:V",quads[i].imgVFlip);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:FLIP:H",quads[i].videoHFlip);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:FLIP:V",quads[i].videoVFlip);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:FLIP:H",quads[i].camHFlip);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":CAM:FLIP:V",quads[i].camVFlip);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":BLENDING:ON",quads[i].bBlendModes);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":BLENDING:MODE",quads[i].blendMode);

            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:ON",quads[i].edgeBlendBool);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:EXPONENT",quads[i].edgeBlendExponent);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:GAMMA",quads[i].edgeBlendGamma);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:LUMINANCE", quads[i].edgeBlendLuminance);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:SIN",quads[i].edgeBlendAmountSin);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:DX",quads[i].edgeBlendAmountDx);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:TOP",quads[i].edgeBlendAmountTop);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:BOTTOM",quads[i].edgeBlendAmountBottom);

            //mask stuff
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":MASK:ON",quads[i].bMask);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":MASK:INVERT_MASK",quads[i].maskInvert);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":MASK:N_POINTS",(int)quads[i].maskPoints.size());
            if (quads[i].maskPoints.size() > 0)
            {
                for(int j=0; j<quads[i].maskPoints.size(); j++)
                {
                    XML.setValue("QUADS:QUAD_"+ofToString(i)+":MASK:POINTS:POINT_"+ofToString(j)+":X",quads[i].maskPoints[j].x);
                    XML.setValue("QUADS:QUAD_"+ofToString(i)+":MASK:POINTS:POINT_"+ofToString(j)+":Y",quads[i].maskPoints[j].y);
                }
            }
            // deform stuff
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:ON",quads[i].bDeform);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:ON",quads[i].bBezier);
            XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:GRID:ON",quads[i].bGrid);
            // bezier stuff
            for (int j = 0; j < 4; ++j)
            {
                for (int k = 0; k < 4; ++k)
                {
                    XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":X",quads[i].bezierPoints[j][k][0]);
                    XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":Y",quads[i].bezierPoints[j][k][1]);
                    XML.setValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":Z",quads[i].bezierPoints[j][k][2]);
                }
            }


        }
    }
}


void testApp::getXml()

{

    nOfQuads = XML.getValue("GENERAL:N_OF_QUADS", 0);
    activeQuad = XML.getValue("GENERAL:ACTIVE_QUAD", 0);

    for(int i = 0; i < nOfQuads; i++)
    {
        float x0 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_0:X",0.0);
        float y0 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_0:Y",0.0);
        float x1 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_1:X",0.0);
        float y1 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_1:Y",0.0);
        float x2 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_2:X",0.0);
        float y2 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_2:Y",0.0);
        float x3 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_3:X",0.0);
        float y3 = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CORNERS:CORNER_3:Y",0.0);

        quads[i].setup(x0, y0, x1, y1, x2, y2, x3, y3, slideshowFolders, edgeBlendShader, quadMaskShader, camGrabber, kinect);
        quads[i].quadNumber = XML.getValue("QUADS:QUAD_"+ofToString(i)+":NUMBER", 0);
        quads[i].layer = XML.getValue("QUADS:QUAD_"+ofToString(i)+":LAYER", 0);
        layers[quads[i].layer] = quads[i].quadNumber;

        quads[i].quadDispX = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:DISPX",0);
        quads[i].quadDispY = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:DISPY",0);
        quads[i].quadW = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:WIDTH",0);
        quads[i].quadH = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CONTENT:HEIGHT",0);

        quads[i].imgBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:ACTIVE",0);
        quads[i].loadedImg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:LOADED_IMG", "", 0);
        quads[i].bgImg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:LOADED_IMG_PATH", "", 0);
        if ((quads[i].imgBg) && (quads[i].bgImg != ""))
        {
            quads[i].loadImageFromFile(quads[i].loadedImg, quads[i].bgImg);
        }
        quads[i].imgHFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:FLIP:H", 0);
        quads[i].imgVFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:FLIP:V", 0);

        quads[i].videoBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:ACTIVE",0);
        quads[i].loadedVideo = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOADED_VIDEO", "", 0);
        quads[i].bgVideo = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOADED_VIDEO_PATH", "", 0);
        if ((quads[i].videoBg) && (quads[i].bgVideo != ""))
        {
            quads[i].loadVideoFromFile(quads[i].loadedVideo, quads[i].bgVideo);
        }
        quads[i].videoHFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:FLIP:H", 0);
        quads[i].videoVFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:FLIP:V", 0);

        quads[i].bgSlideshow = XML.getValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:LOADED_SLIDESHOW", 0);

        quads[i].colorBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:ACTIVE",0);

        quads[i].transBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:TRANS:ACTIVE",0);
        quads[i].transDuration = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:TRANS:DURATION", 1.0);
        quads[i].slideshowBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:ACTIVE", 0);
        quads[i].slideshowSpeed = XML.getValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:SPEED", 1.0);
        quads[i].slideFit = XML.getValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:FIT", 0);
        quads[i].slideKeepAspect = XML.getValue("QUADS:QUAD_"+ofToString(i)+":SLIDESHOW:KEEP_ASPECT", 1);

        quads[i].camBg = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:ACTIVE",0);
        quads[i].camWidth = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:WIDTH",0);
        quads[i].camHeight = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:HEIGHT",0);
        quads[i].camHFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:FLIP:H", 0);
        quads[i].camVFlip = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:FLIP:V", 0);

        quads[i].camMultX = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:MULT_X",1.0);
        quads[i].camMultY = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:MULT_Y",1.0);
        quads[i].imgMultX = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:MULT_X",1.0);
        quads[i].imgMultY = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:MULT_Y",1.0);
        quads[i].videoMultX = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:MULT_X",1.0);
        quads[i].videoMultY = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:MULT_Y",1.0);
        quads[i].videoSpeed = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:SPEED",1.0);
        quads[i].videoVolume = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:VOLUME",0);
        quads[i].videoLoop = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:LOOP",1);

        quads[i].bgColor.r = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:R",0.0);
        quads[i].bgColor.g = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:G",0.0);
        quads[i].bgColor.b = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:B",0.0);
        quads[i].bgColor.a = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:A",0.0);

        quads[i].secondColor.r = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:R",0.0);
        quads[i].secondColor.g = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:G",0.0);
        quads[i].secondColor.b = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:B",0.0);
        quads[i].secondColor.a = XML.getValue("QUADS:QUAD_"+ofToString(i)+":COLOR:SECOND_COLOR:A",0.0);

        quads[i].imgColorize.r = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:R",1.0);
        quads[i].imgColorize.g = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:G",1.0);
        quads[i].imgColorize.b = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:B",1.0);
        quads[i].imgColorize.a = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IMG:COLORIZE:A",1.0);

        quads[i].videoColorize.r = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:R",1.0);
        quads[i].videoColorize.g = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:G",1.0);
        quads[i].videoColorize.b = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:B",1.0);
        quads[i].videoColorize.a = XML.getValue("QUADS:QUAD_"+ofToString(i)+":VIDEO:COLORIZE:A",1.0);

        quads[i].camColorize.r = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:R",1.0);
        quads[i].camColorize.g = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:G",1.0);
        quads[i].camColorize.b = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:B",1.0);
        quads[i].camColorize.a = XML.getValue("QUADS:QUAD_"+ofToString(i)+":CAM:COLORIZE:A",1.0);

        quads[i].bBlendModes = XML.getValue("QUADS:QUAD_"+ofToString(i)+":BLENDING:ON", 0);
        quads[i].blendMode= XML.getValue("QUADS:QUAD_"+ofToString(i)+":BLENDING:MODE", 0);

        quads[i].edgeBlendBool = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:ON", 0);
        quads[i].edgeBlendExponent = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:EXPONENT", 1.0);
        quads[i].edgeBlendGamma = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:GAMMA", 1.8);
        quads[i].edgeBlendLuminance = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:LUMINANCE", 0.0);
        quads[i].edgeBlendAmountSin = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:SIN", 0.3);
        quads[i].edgeBlendAmountDx = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:DX", 0.3);
        quads[i].edgeBlendAmountTop = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:TOP", 0.0);
        quads[i].edgeBlendAmountBottom = XML.getValue("QUADS:QUAD_"+ofToString(i)+":EDGE_BLENDING:AMOUNT:BOTTOM", 0.0);

        //mask stuff
        quads[i].bMask = XML.getValue("QUADS:QUAD_"+ofToString(i)+":MASK:ON", 0);
        quads[i].maskInvert = XML.getValue("QUADS:QUAD_"+ofToString(i)+":MASK:INVERT_MASK", 0);
        int nOfMaskPoints =  XML.getValue("QUADS:QUAD_"+ofToString(i)+":MASK:N_POINTS", 0);
        quads[i].maskPoints.clear();
        if (nOfMaskPoints > 0)
        {
            for(int j=0; j<nOfMaskPoints; j++)
            {
                ofPoint tempMaskPoint;
                tempMaskPoint.x = XML.getValue("QUADS:QUAD_"+ofToString(i)+":MASK:POINTS:POINT_"+ofToString(j)+":X", 0);
                tempMaskPoint.y = XML.getValue("QUADS:QUAD_"+ofToString(i)+":MASK:POINTS:POINT_"+ofToString(j)+":Y", 0);
                quads[i].maskPoints.push_back(tempMaskPoint);
            }
        }

        // deform stuff
        quads[i].bDeform = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:ON",0);
        quads[i].bBezier = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:ON",0);
        quads[i].bGrid = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:GRID:ON",0);
        // bezier stuff
        for (int j = 0; j < 4; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                quads[i].bezierPoints[j][k][0] = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":X",0.0);
                quads[i].bezierPoints[j][k][1] = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":Y",0.0);
                quads[i].bezierPoints[j][k][2] = XML.getValue("QUADS:QUAD_"+ofToString(i)+":DEFORM:BEZIER:CTRLPOINTS:POINT_"+ofToString(j)+"_"+ofToString(k)+":Z",0.0);
            }
        }


        quads[i].isOn = XML.getValue("QUADS:QUAD_"+ofToString(i)+":IS_ON",0);

    }
}
