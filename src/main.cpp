#include "testApp.h"
#include "ofMain.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main( )
{

    ofAppGlutWindow window;
    //ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
    ofSetOpenGLVersion(1,4);
    ofSetupOpenGL(&window, 1024, 768, OF_FULLSCREEN);			// <-------- setup the GL context
    ofSetWindowPosition(1400,150);



    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:

    ofRunApp(new testApp());

}
