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

class PreviewApp : public App {
private:
	qs::QuadLoader loader;
	gl::GlslProgRef mGlsl;
	gl::Texture2dRef mColorTex;
	gl::Texture2dRef mDepthTex;
	gl::Texture2dRef mConfidenceTex;
	TriMesh mMesh;
	double time = 0.0;

	CameraPersp			mCamera;
	CameraUi			mCamUi;

	bool play = false;
	bool diffuse = false;

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
			in vec4 ciPosition;
			out VertexData {
				vec2 uv;
				float confidence;
			} vOut;

			uniform vec4 cameraParam;
			uniform sampler2D depthTex;
			uniform sampler2D confidenceTex;
			uniform vec2 colorResolution;
			uniform vec2 depthUvScale;
			
			void main(void) {
				vec2 colorPos = ciPosition.xy * colorResolution;
				vOut.uv = vec2(ciPosition.x, 1.0 - ciPosition.y);
				float depth = texture(depthTex, vOut.uv * depthUvScale).r;
				vOut.confidence = texture(confidenceTex, vOut.uv * depthUvScale).r * 255.0;

				gl_Position = vec4(
					(colorPos.x - cameraParam.x) * depth / cameraParam.z,
					(colorPos.y - cameraParam.y) * depth / cameraParam.w,
					-depth,
					1.0
				);
			}
		 ))
		.geometry(CI_GLSL(150,
			layout (triangles) in;
			layout (triangle_strip, max_vertices = 3) out;

			in VertexData {
				vec2 uv;
				float confidence;
			} vIn[];

			out VertexData {
				vec2 uv;
				float confidence;
				float light;
			} vOut;

			uniform mat4 ciModelViewInverse;
			uniform mat4 ciModelViewProjection;

			void main()
			{
				vec3 p1 = gl_in[0].gl_Position.xyz;
				vec3 p2 = gl_in[1].gl_Position.xyz;
				vec3 p3 = gl_in[2].gl_Position.xyz;
				vec3 v1 = p2 - p1;
				vec3 v2 = p3 - p1;
				float depth_avg = abs(p1.z + p2.z + p3.z) / 3.0;

				// カメラに対する勾配が急な面を描画しないようにする (depthが近いほど判定を厳しくする)
				if (max(length(v1), length(v2)) > depth_avg * 0.1) return;

				// 法線ベクトルとライティングの計算
				vec3 normal = normalize(cross(v1, v2));
				vec3 lightDirection = vec3(0.0, 0.0, 1.0);
				lightDirection = normalize(ciModelViewInverse * vec4(lightDirection, 0.0)).xyz;
				float light = abs(dot(normal, lightDirection));

				vOut.uv = vIn[0].uv;
				vOut.confidence = vIn[0].confidence;
				vOut.light = light;
				gl_Position = ciModelViewProjection * gl_in[0].gl_Position;
				EmitVertex();

				vOut.uv = vIn[1].uv;
				vOut.confidence = vIn[1].confidence;
				vOut.light = light;
				gl_Position = ciModelViewProjection * gl_in[1].gl_Position;
				EmitVertex();

				vOut.uv = vIn[2].uv;
				vOut.confidence = vIn[2].confidence;
				vOut.light = light;
				gl_Position = ciModelViewProjection * gl_in[2].gl_Position;
				EmitVertex();

				EndPrimitive();
			}
		))
		.fragment(CI_GLSL(150,
			in VertexData {
				vec2 uv;
				float confidence;
				float light;
			} vIn;
			out vec4 oColor;

			uniform sampler2D colorTex;
			uniform vec2 colorUvScale;
			uniform bool mode;

			void main(void) {
				vec3 color = texture(colorTex, vIn.uv * colorUvScale).rgb;
				if (mode) { color = vec3(0.98,0.176,0.463) * vIn.light + vec3(0.494,0.235,0.812) * (1.0 - vIn.light); }
				oColor = vec4(color, 1.0);
			}
		)));
		
		gl::enableDepthWrite();
		gl::enableDepthRead();

		mMesh = TriMesh(TriMesh::Format().positions(2));
		for(uint32_t y = 0; y < 192; y++) { for(uint32_t x = 0; x < 256; x++) {
			mMesh.appendPosition(vec2((double)(x + 0) / 255.0, (double)(y + 0) / 191.0));
		}}
		for(uint32_t y = 0; y < 192 - 1; y++) { for(uint32_t x = 0; x < 256 - 1; x++) {
			uint32_t p1 = (y + 0) * 256 + x;
			uint32_t p2 = (y + 1) * 256 + x;
			uint32_t p3 = p2 + 1;
			uint32_t p4 = p1 + 1;
			mMesh.appendTriangle(p3, p2, p1);
			mMesh.appendTriangle(p1, p4, p3);
		}}

		mCamera.lookAt( normalize( vec3( 3, 3, 6 ) ) * 5.0f, vec3(0.0) );
		mCamUi = CameraUi( &mCamera );

		updateCamera();

		setWindowSize(1280, 720);

		mCamera.lookAt(vec3(0), vec3(0, 0, -1));

		mGlsl->uniform("mode", false);
	}
	void update() override { if (play) updateCamera(); }
	void keyDown(KeyEvent e) override {
		switch(e.getCode()) {
		case KeyEvent::KEY_SPACE:
			play = !play;
			break;
		case KeyEvent::KEY_a:
			mGlsl->uniform("mode", diffuse = !diffuse);
			break;
		case KeyEvent::KEY_r:
			mCamera.lookAt(vec3(0), vec3(0, 0, -1));
			break;
		}
	}
	void updateCamera() {
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
			mGlsl->uniform("colorTex", 0);
		}
		if (!mDepthTex) {
			mDepthTex = gl::Texture2d::create(camera.depth.cols, camera.depth.rows, gl::Texture2d::Format().internalFormat(GL_R32F).dataType(GL_FLOAT));
			mDepthTex->bind(1);
			mGlsl->uniform("depthTex", 1);
		}
		if (!mConfidenceTex) {
			mConfidenceTex = gl::Texture2d::create(camera.confidence.cols, camera.confidence.rows, gl::Texture2d::Format().internalFormat(GL_R8).dataType(GL_UNSIGNED_BYTE));
			mConfidenceTex->bind(2);
			mGlsl->uniform("confidenceTex", 2);
		}

		mColorTex->update(camera.color.data, GL_BGR, GL_UNSIGNED_BYTE, 0, camera.color.cols, camera.color.rows);
		mDepthTex->update(camera.depth.data, GL_RED, GL_FLOAT, 0, camera.depth.cols, camera.depth.rows);
		mConfidenceTex->update(camera.confidence.data, GL_RED, GL_UNSIGNED_BYTE, 0, camera.confidence.cols, camera.confidence.rows);

		mGlsl->uniform("colorResolution", glm::vec2((float)mColorTex->getActualWidth(), (float)mColorTex->getActualHeight()));
		mGlsl->uniform("colorUvScale", glm::vec2(
			(float)mColorTex->getActualWidth()  / (float)(camera.color.cols),
			(float)mColorTex->getActualHeight() / (float)(camera.color.rows)
		));
		mGlsl->uniform("depthUvScale", glm::vec2(
			(float)mDepthTex->getActualWidth()  / (float)(camera.depth.cols),
			(float)mDepthTex->getActualHeight() / (float)(camera.depth.rows)
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

		if (!mColorTex || !mDepthTex || !mConfidenceTex) return;

		gl::ScopedFaceCulling cull(true, GL_BACK);
		gl::ScopedTextureBind tex0(mColorTex, 0);
		gl::ScopedTextureBind tex1(mDepthTex, 1);
		gl::ScopedTextureBind tex2(mConfidenceTex, 2);
		gl::ScopedGlslProg scpGlsl(mGlsl);

		mCamera.setAspectRatio(getWindowAspectRatio());
		gl::setMatrices(mCamera);

		time += 0.02;
		const double distance = 50.0;

		gl::pointSize(3.0);
		gl::draw(mMesh);
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

CINDER_APP( PreviewApp, RendererGl )
