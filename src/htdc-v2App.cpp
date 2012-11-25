#include "cinder/app/AppBasic.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "GlslHotProg.h"

#include "VOpenNIHeaders.h"

#include "CinderOpenCV.h"

#include "Particle.h"
#include "ParticleSystem.h"
#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/Timeline.h"
#include "cinder/params/Params.h"

#include "cinder/audio/Input.h"
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
    
    void blowAway();
    
private:
    V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;
    
    cv::Mat _depth, _image, _background, _bgMask, _fgMask, _backgroundDepth;
    
    bool _captureBackground = true;
    int _depthThresholdMm = 120;
    
    double _lastInvisible = 0.0;
    
    cv::VideoCapture capture = cv::VideoCapture( CV_CAP_OPENNI );
    
    ciMsaFluidSolver	fluidSolver;
	ciMsaFluidDrawerGl	fluidDrawer;
	ParticleSystem		particleSystem;
    
    Perlin              perlin;
    
    Area                _validDepthArea = Area(13,44,598,480);
    
    audio::Input        _input;
    
    GlslHotProg         _shader;
    gl::Texture         _distortTex;
    
    Anim<float>         _invisible = 0.0f;
    
    float                _randomGustProb = 0.02f;

    float               _d0 = 1300.0f, _d1 = 0.15f;
    
    float               _invisibleTime = 10.0f;
    float               _noiseProbability = 0.005f;
    
    double              _lastGust = 0.0;
    
    bool                _isFlipped = true;
    
    int                 _backgroundDelta = 3;
    
    float               _x = -100.0f;
    
    params::InterfaceGl _params;
};

void htdcApp::setup()
{
    setWindowSize( 1280, 900 );
    _params = params::InterfaceGl( "htdc", Vec2i( 0, 480 ) );
    _params.addParam( "Distance Threshold (mm)", &_depthThresholdMm );
    _params.addParam( "Invisibility Time (sec)", &_invisibleTime );
    _params.addParam( "Random Gust Probability", &_randomGustProb, "min=0.0 max=1.0 step=0.001" );
    _params.addParam( "Noise Probability", &_noiseProbability, "min=0.0 max=1.0 step=0.001" );
    _params.addParam( "Background Delta", &_backgroundDelta );
    _params.addParam( "Flip Image", &_isFlipped );
    _params.addParam( "Offset X", &_x );
    _params.addSeparator( "Audio" );
    _params.addParam( "d0", &_d0 );
    _params.addParam( "d1", &_d1 );
    _params.hide();
    
    particleSystem.setWindowSize( getWindowSize() );
    
    // setup fluid stuff
	fluidSolver.setup(100, 100);
    fluidSolver.enableRGB(false).setFadeSpeed(0.0002).setDeltaT(0.5).setVisc(0.000005).setColorDiffusion(0);
    fluidSolver.setSize( 150 , 150 / getWindowAspectRatio() );
	fluidDrawer.setup( &fluidSolver );
	particleSystem.setFluidSolver( &fluidSolver );
    
    console() << audio::Input::getDefaultDevice()->getName() << endl;
	_input = audio::Input();
	_input.start();
    
    _distortTex = gl::Texture( loadImage( getAssetPath( "glitch.jpg" )));
    _shader = GlslHotProg( "glitch.vert", "glitch.frag" );
}

void htdcApp::blowAway() {
    if ( _invisible != 0.0f ) return;
    
    _invisible = 1.0f;
    timeline().apply( &_invisible, 0.0f, 0.5f ).delay( _invisibleTime );
    _lastInvisible = getElapsedSeconds();
    
    float n = cv::countNonZero( _fgMask );
    float prob = MAX_PARTICLES / n;
    
    for ( int i = 0; i < _fgMask.cols; i++ )
    {
        for ( int j = 0; j < _fgMask.rows; j++ )
        {
            if ( _fgMask.at<uchar>( j, i ) > 0 && randFloat() < prob ) {
                particleSystem.addParticle( Vec2f( i, j ), _image.at<uchar>( j, i ) / 256.0f );
            }
        }
    }
    
    _lastGust = getElapsedSeconds();
}

void htdcApp::keyDown( KeyEvent event ) {
    switch ( event.getCode() ) {
        case KeyEvent::KEY_SPACE:
            _captureBackground = true;
            break;
            
        case KeyEvent::KEY_RETURN:
            if ( _invisible ) _invisible = false;
            else blowAway();
            break;
            
        case KeyEvent::KEY_UP:
            _depthThresholdMm++;
            console() << _depthThresholdMm << endl;
            break;
            
        case KeyEvent::KEY_DOWN:
            _depthThresholdMm--;
            console() << _depthThresholdMm << endl;
            break;
        case KeyEvent::KEY_p:
            if ( _params.isVisible() ) { _params.hide(); hideCursor(); }
            else { _params.show(); showCursor(); }
            
    }
}

void htdcApp::mouseDown( MouseEvent event )
{
}

void htdcApp::update()
{
    _shader.update();
    
    static cv::Mat _tempDepth,
    _erodeElem = cv::getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 9, 9 ) );
    
    capture.grab();
    capture.retrieve( _tempDepth, CV_CAP_OPENNI_DEPTH_MAP );
    capture.retrieve( _image, CV_CAP_OPENNI_GRAY_IMAGE );
    
    if ( _captureBackground ) {
        _image.copyTo( _background );
        _depth.copyTo( _backgroundDepth );
        _captureBackground = false;
    }
    
    _tempDepth.convertTo( _depth, CV_8U, 1.0/54.0 );
    
    // TODO: switch to Mat::setTo
    cv::threshold( _depth, _fgMask, 256 * _depthThresholdMm / 1400.0, 1, cv::THRESH_TOZERO_INV );
    cv::threshold( _fgMask, _bgMask, 0, 255, cv::THRESH_BINARY_INV );
//    cv::threshold( _depth, _fgMask, 256 * _depthThresholdMm / 1400.0, 1, cv::THRESH_TOZERO_INV );
//    cv::threshold( _fgMask, _bgMask, 0, 255, cv::THRESH_BINARY_INV );
    _depth.setTo( 255, _depth == 0 );
    _fgMask = (_depth < (_backgroundDepth - _backgroundDelta));
    _bgMask = 255 - _fgMask;
    cv::erode( _bgMask, _bgMask, _erodeElem, cv::Point(-1,-1), 9 );
    
    _image.copyTo( _background, _bgMask );
    
    //    cv::blur( _tempColor, _background, cv::Size( 30, 3 ) );
    
    
    if ( _invisible > 0.0f ) {
        float mag = math<float>::clamp( getElapsedSeconds() - .3f - _lastInvisible );
        // update fluid
        for ( int y = 1; y < fluidSolver.getHeight(); y++ )
        {
            for ( int x = 1; x < fluidSolver.getWidth(); x++ )
            {
                float p = perlin.fBm( Vec3f( x, y, getElapsedSeconds() * 0.01f ) * 0.05f ) * 0.005f;
                fluidSolver.addForceAtCell( x, y, Vec2f( Rand::randFloat(0.001f), p + Rand::randFloat( -0.005f, 0.005f ) ) * mag );
            }
        }
    }
    
	fluidSolver.update();
    
    audio::PcmBuffer32fRef pcmBuffer = _input.getPcmBuffer();
    if ( pcmBuffer && pcmBuffer->getSampleCount() > 0 ) {
        audio::Buffer32fRef leftBuffer = pcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT );
        float* pcm = leftBuffer->mData;
        int len = leftBuffer->mSampleCount;
        float sumd0=0, sumd1=0;
        
        for( int n = 1; n < len; n++ )
        {
            sumd0+= math<float>::abs(pcm[n]);
            sumd1+= math<float>::abs(pcm[n]-pcm[n-1]);
        }
        
        if ( sumd0 > _d0 && sumd1/sumd0 < _d1) {
            console() << "NOISE" << std::endl;
            blowAway();
        }
    }
    
    if ( _randomGustProb > 0.0f && _invisible == 0.0f
        && ((getElapsedSeconds() - _lastGust) > 30.0) && Rand::randFloat() < _randomGustProb ) {
        blowAway();
    }
}

void htdcApp::draw()
{
	// clear out the window with black
	gl::clear( Color::black() );
    
    gl::color( Color::white() );
    
    gl::Texture depthTex = gl::Texture( fromOcv( _depth ) );
    gl::Texture maskTex = gl::Texture( fromOcv( _bgMask ) );
    gl::Texture bgTex = gl::Texture( fromOcv( _background ) );
    gl::Texture imageTex = gl::Texture( fromOcv( _image ) );
	gl::draw( depthTex, Vec2i( 0, 480 ) );
	gl::draw( maskTex, Vec2i( 640, 480 ) );
    gl::draw( bgTex, Vec2i( 640, 0 ) );
    
    if ( _invisible == 1.0f ) {
        gl::draw( bgTex, _validDepthArea, Rectf(0,0,640,480) );
        gl::pushMatrices();
        gl::translate( -_validDepthArea.getUL() );
        gl::scale( Vec2f(640,480) / _validDepthArea.getSize() );
        particleSystem.updateAndDraw();
        gl::popMatrices();
    } else {
        
        if ( _invisible > 0.0f || Rand::randFloat() < _noiseProbability ) {
            _shader->bind();
            _shader->uniform( "tex0", 0 );
            _distortTex.bind( 1 );
            _shader->uniform( "tex1", 1 );
            _shader->uniform( "texdim0", imageTex.getSize());
            _shader->uniform( "barsamount", Rand::randFloat(0.1f) );
            _shader->uniform( "distortion", 0.1f );
            _shader->uniform( "resolution", Rand::randInt(100) );
            _shader->uniform( "vsync", Rand::randFloat(0.5f) );
            _shader->uniform( "hsync", 0.0f );
        }
        
        gl::draw( imageTex, _validDepthArea, Rectf(0,0,640,480) );
        _distortTex.unbind();
        _shader->unbind();
    }
    //    gl::drawStrokedRect(Rectf(0,480,640,480*2));
    
    gl::drawString( toString( getAverageFps() ), Vec2f::one() * 30.0f );
    
    if ( _params.isVisible() ) {
        gl::drawString( toString( getAverageFps() ), Vec2f::one() * 30.0f );
        params::InterfaceGl::draw();
    }
}


CINDER_APP_BASIC( htdcApp, RendererGl )
