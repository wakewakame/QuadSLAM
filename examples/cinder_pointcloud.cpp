#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"

// メモ
// "loader/quad_loader.h"を"cinder/gl/gl.h"よりも前にincludeすると
// OpenCV内でdefineされるFARがcinderのコードを置換してエラーが発生してしまう
#include "loader/quad_loader.h"

using namespace ci;
using namespace ci::app;

class BasicApp : public App {
private:
	qs::QuadLoader loader;
	gl::GlslProgRef mGlsl;
	gl::Texture2dRef mColorTex;
	gl::Texture2dRef mDepthTex;
	gl::Texture2dRef mConfidenceTex;
	gl::VertBatchRef vert;
	double time = 0.0;

	CameraPersp			mCamera;
	CameraUi			mCamUi;

public:
	void setup() override {
		// Initialize QuadLoader ====================================================
		std::vector<std::string> args = getCommandLineArgs();
		if (2 != args.size()) {
			std::cout
				<< "QuadSLAM version 0.0.1\n"
				<< "\n"
				<< "usage: quadslam input_path\n"
				<< "  input_path: Directory containing QuadDump recording files"
				<< "\n"
				<< std::endl;
			quit();
			return;
		}

		std::string recDirPath = args.at(1);
		loader.open(recDirPath);
		if (!loader.isOpened()) {
			std::cout << "failed to open forder" << std::endl;
			quit();
			return;
		}

		// Initialize Cinder ========================================================
		mGlsl = gl::GlslProg::create( gl::GlslProg::Format()
		.vertex(CI_GLSL(150,
			uniform mat4 ciModelViewProjection;
			in vec4 ciPosition;
			in vec2 ciTexCoord0;
			out float alpha;
			out vec2 uv;

			uniform sampler2D uDepthTex;
			uniform sampler2D uConfidenceTex;
			uniform vec2 depthUvScale;
			uniform vec4 cameraParam;
			
			void main(void) {
				uv = ciTexCoord0;
				vec4 pos = ciPosition;
				float depth = texture(uDepthTex, uv * depthUvScale).r;
				int confidence = int(texture(uConfidenceTex, uv * depthUvScale).r * 255.0);

				// Calculate the vertex's world coordinates.
				float xrw = (pos.x - cameraParam.x) * depth / cameraParam.z;
				float yrw = (pos.y - cameraParam.y) * depth / cameraParam.w;
				vec4 xyzw = vec4(xrw, yrw, -depth, 1.0);

				alpha = (confidence == 0) ? 0.0 : 1.0;
				alpha = 1.0;
				gl_Position	= ciModelViewProjection * xyzw;
			}
		 ))
		.fragment(CI_GLSL(150,
			in vec2 uv;
			in float alpha;
			out vec4 oColor;
			uniform sampler2D uColorTex;
			uniform sampler2D uDepthTex;
			uniform sampler2D uConfidenceTex;
			uniform vec2 colorUvScale;
			uniform vec2 depthUvScale;

			void main(void) {
				vec3 color = texture(uColorTex, uv * colorUvScale).rgb;
				oColor = vec4(color, alpha);
			}
		)));
		
		gl::enableDepthWrite();
		gl::enableDepthRead();

		vert = gl::VertBatch::create( GL_POINTS );
		for(int y = 0; y < 192; y++) {
			for(int x = 0; x < 256; x++) {
				vert->texCoord( (double)x / 255.0, 1.0 - (double)y / 191.0 );
				vert->vertex( 1920.0 * (double)x / 255.0, 1440.0 * (double)y / 191.0 );
			}
		}

		mCamera.lookAt( normalize( vec3( 3, 3, 6 ) ) * 5.0f, vec3(0.0) );
		mCamUi = CameraUi( &mCamera );
	}
	void update() override {
		auto quad = loader.next();
		if (!quad.has_value()) {
			quit();
			return;
		}
		qs::QuadFrame& quadFrame = quad.value();
		qs::Camera camera = quadFrame.camera;

		if (camera.color.empty() || camera.depth.empty() || camera.confidence.empty()) {
			return;
		}

		if (!mColorTex) {
			mColorTex = gl::Texture2d::create(camera.color.cols, camera.color.rows, gl::Texture2d::Format().internalFormat(GL_SRGB8).dataType(GL_UNSIGNED_BYTE));
			mColorTex->bind(0);
			mGlsl->uniform("uColorTex", 0);
		}
		if (!mDepthTex) {
			mDepthTex = gl::Texture2d::create(camera.depth.cols, camera.depth.rows, gl::Texture2d::Format().internalFormat(GL_R32F).dataType(GL_FLOAT));
			mDepthTex->bind(1);
			mGlsl->uniform("uDepthTex", 1);
		}
		if (!mConfidenceTex) {
			mConfidenceTex = gl::Texture2d::create(camera.confidence.cols, camera.confidence.rows, gl::Texture2d::Format().internalFormat(GL_R8).dataType(GL_UNSIGNED_BYTE));
			mConfidenceTex->bind(2);
			mGlsl->uniform("uConfidenceTex", 2);
		}

		mColorTex->update(camera.color.data, GL_BGR, GL_UNSIGNED_BYTE, 0, camera.color.cols, camera.color.rows);
		mDepthTex->update(camera.depth.data, GL_RED, GL_FLOAT, 0, camera.depth.cols, camera.depth.rows);
		mConfidenceTex->update(camera.confidence.data, GL_RED, GL_UNSIGNED_BYTE, 0, camera.confidence.cols, camera.confidence.rows);

		mGlsl->uniform("colorUvScale", glm::vec2(
			(float)mColorTex->getActualWidth()  / (float)camera.color.cols,
			(float)mColorTex->getActualHeight() / (float)camera.color.rows
		));
		mGlsl->uniform("depthUvScale", glm::vec2(
			(float)mDepthTex->getActualWidth()  / (float)camera.depth.cols,
			(float)mDepthTex->getActualHeight() / (float)camera.depth.rows
		));
		mGlsl->uniform("cameraParam", glm::vec4(
			camera.ar.intrinsics[0][2],
			camera.ar.intrinsics[1][2],
			camera.ar.intrinsics[0][0],
			camera.ar.intrinsics[1][1]
		));
	}
	void draw() override {
		gl::clear( Color::gray( 0.1f ) );
		gl::color( 1.0f, 0.5f, 0.25f );

		//gl::ScopedFaceCulling cull(true, GL_BACK);
		gl::ScopedTextureBind tex0(mColorTex, 0);
		gl::ScopedTextureBind tex1(mDepthTex, 1);
		gl::ScopedTextureBind tex2(mConfidenceTex, 2);
		gl::ScopedGlslProg scpGlsl(mGlsl);

		time += 0.02;
		const double distance = 50.0;
		gl::setMatrices(mCamera);

		gl::pointSize(3.0);
		vert->draw();
	}
	void mouseUp(MouseEvent event) override {
		mCamUi.mouseUp( event );
	}
	void mouseDown(MouseEvent event) override {
		mCamUi.mouseDown( event );
	}
	void mouseWheel(MouseEvent event) override {
		mCamUi.mouseWheel( event );
	}
	void mouseDrag(MouseEvent event) override {
		mCamUi.mouseDrag( event );
	}
};

CINDER_APP( BasicApp, RendererGl )
