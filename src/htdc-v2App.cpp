#include "cinder/app/AppBasic.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "VOpenNIHeaders.h"

#include "GlslHotProg.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class htdcApp : public AppBasic {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
    void keyDown( KeyEvent event );
    
private:
    V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;
    
    cv::Mat _depth, _background, _mask;
    
    bool _captureBackground = true;
    int _depthThresholdMm = 120;
    
    cv::VideoCapture capture = cv::VideoCapture( CV_CAP_OPENNI );
};

void htdcApp::setup()
{
    setWindowSize( 1280, 900 );
//    
//    capture.grab();
//    capture.retrieve( _background, CV_CAP_OPENNI_BGR_IMAGE );

}

void htdcApp::keyDown( KeyEvent event ) {
    switch ( event.getCode() ) {
        case KeyEvent::KEY_SPACE:
            _captureBackground = true;
            break;
            
        case KeyEvent::KEY_UP:
            _depthThresholdMm++;
            console() << _depthThresholdMm << endl;
            break;
            
        case KeyEvent::KEY_DOWN:
            _depthThresholdMm--;
            console() << _depthThresholdMm << endl;
            break;
    }
}

void htdcApp::mouseDown( MouseEvent event )
{
}

void htdcApp::update()
{
    static cv::Mat _tempColor, _tempDepth, _tempMask,
                    _erodeElem = cv::getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 9, 9 ) );
    
    capture.grab();
    capture.retrieve( _tempDepth, CV_CAP_OPENNI_DEPTH_MAP );
    capture.retrieve( _tempColor, CV_CAP_OPENNI_GRAY_IMAGE );
    
    if ( _captureBackground ) {
        _tempColor.copyTo( _background );
        _captureBackground = false;
    }

    _tempDepth.convertTo( _depth, CV_8U, 1.0/54.0 );

    // TODO: switch to Mat::setTo
    cv::threshold( _depth, _tempMask, 256 * _depthThresholdMm / 1400.0, 1, cv::THRESH_TOZERO_INV );
    cv::threshold( _tempMask, _mask, 0, 255, cv::THRESH_BINARY_INV );
    cv::erode( _mask, _mask, _erodeElem, cv::Point(-1,-1), 9 );

    _tempColor.copyTo( _background, _mask );
    
//    cv::blur( _tempColor, _background, cv::Size( 30, 3 ) );
}

void htdcApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
    
    gl::color( Color::white() );
    
    gl::Texture depthTex = gl::Texture( fromOcv( _depth ) );
    gl::Texture maskTex = gl::Texture( fromOcv( _mask ) );
    gl::Texture bgTex = gl::Texture( fromOcv( _background ) );
	gl::draw( depthTex, Vec2i( 0, 0 ) );
    gl::drawStrokedRect(Rectf(0,0,640,480));
	gl::draw( maskTex, Vec2i( 640, 0 ) );
    gl::drawStrokedRect(Rectf(640,0,640*2,480));
    gl::draw( bgTex, Vec2i( 0, 480 ) );
    gl::drawStrokedRect(Rectf(0,480,640,480*2));
    
    gl::drawString( toString( getAverageFps() ), Vec2f::one() * 30.0f );
}


CINDER_APP_BASIC( htdcApp, RendererGl )
