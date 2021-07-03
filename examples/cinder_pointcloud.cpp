#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

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
	CameraPersp mCam;
	double time = 0.0;

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
			in vec4 ciColor;
			in vec2 ciTexCoord0;
			out vec4 color;
			out vec2 uv;

			uniform sampler2D uDepthTex;
			uniform vec2 depthUvScale;
			
			void main(void) {
				color = ciColor;
				uv = ciTexCoord0;
				float depth = texture(uDepthTex, uv * depthUvScale).r;
				vec4 pos = ciPosition;
				pos.z -= depth;
				gl_Position	= ciModelViewProjection * pos;
			}
		 ))
		.fragment(CI_GLSL(150,
			in vec2 uv;
			in vec4 color;
			out vec4 oColor;
			uniform sampler2D uColorTex;
			uniform sampler2D uDepthTex;
			uniform sampler2D uConfidenceTex;
			uniform vec2 colorUvScale;
			uniform vec2 depthUvScale;

			vec4 colorful(float num) {
				num = log(num);
				float fill = step(0.6, fract(num * 8.0));
				num = fract(num) * 6.0;
				vec3 color =
					(              num < 1.0) ? vec3(1.0      , num      , 0.0      ) :
					(1.0 <= num && num < 2.0) ? vec3(2.0 - num, 1.0      , 0.0      ) :
					(2.0 <= num && num < 3.0) ? vec3(0.0      , 1.0      , num - 2.0) :
					(3.0 <= num && num < 4.0) ? vec3(0.0      , 4.0 - num, 1.0      ) :
					(4.0 <= num && num < 5.0) ? vec3(num - 4.0, 0.0      , 1.0      ) :
												vec3(1.0      , 0.0      , 6.0 - num);
				return vec4(color, fill);
			}
			
			void main(void) {
				vec3  color      = texture(uColorTex, uv * colorUvScale).rgb;
				float depth      = texture(uDepthTex, uv * depthUvScale).r;
				int   confidence = int(texture(uConfidenceTex, uv * depthUvScale).r * 255.0);
				vec4  depthColor = colorful(depth);
				vec3  result     = color * 0.5 + depthColor.rgb * depthColor.a * 0.5;
				oColor = vec4(result, 1.0);

				oColor = vec4(color, 1.0);
			}
		)));
		
		gl::enableDepthWrite();
		gl::enableDepthRead();
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

		vert = gl::VertBatch::create( GL_LINE_STRIP );
		for(int y = 0; y < 192; y++) {
			for(int x = 0; x < 256; x++) {
				vert->texCoord( (double)x / 256.0, (double)y / 192.0 );
				vert->vertex( 32.0 * (double)x / 256.0 - 16.0, 16.0 - 32.0 * (double)y / 192.0 );
			}
		}
	}
	void draw() override {
		gl::clear( Color::gray( 0.1f ) );
		gl::color( 1.0f, 0.5f, 0.25f );

		//gl::ScopedFaceCulling cull(true, GL_BACK);
		gl::ScopedTextureBind tex0(mColorTex, 0);
		gl::ScopedTextureBind tex1(mDepthTex, 1);
		gl::ScopedTextureBind tex2(mConfidenceTex, 2);
		gl::ScopedGlslProg scpGlsl(mGlsl);

		time += 0.06;
		mCam.lookAt( vec3( sin(time) * 100.0, 20.0, cos(time) * 100.0 ), vec3( 0 ) );
		gl::setMatrices(mCam);

		vert->draw();
	}
};

CINDER_APP( BasicApp, RendererGl )
