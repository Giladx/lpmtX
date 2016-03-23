#include "testApp.h"
#include "ofMain.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main()
{

    ofAppGlutWindow* window = new ofAppGlutWindow();
    ofSetupOpenGL(window, 1280, 1024, OF_WINDOW);			// <-------- setup the GL context
//    ofSetWindowPosition(0,0);
    ofSetWindowTitle("LpmtX recoded by Gil@d.X");



    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:

    ofRunApp(new testApp());

}
