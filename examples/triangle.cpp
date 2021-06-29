#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;

class BasicApp : public App {
public:
	void draw() override {
		gl::clear( Color::gray( 0.1f ) );
		gl::color( 1.0f, 0.5f, 0.25f );
		gl::begin( GL_TRIANGLES );
		gl::vertex( 300.0, 100.0 );
		gl::vertex( 100.0, 400.0 );
		gl::vertex( 500.0, 400.0 );
		gl::end();
	}
};

CINDER_APP( BasicApp, RendererGl )
