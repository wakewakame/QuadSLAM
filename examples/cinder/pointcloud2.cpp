#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"

// メモ
// "loader/quad_loader.h"を"cinder/gl/gl.h"よりも前にincludeすると
// OpenCV内でdefineされるFARがcinderのコードを置換してエラーが発生してしまう
#include "loader/quad_loader.h"

#include <list>

using namespace ci;
using namespace ci::app;

class PointCloud {
public:
	enum class DrawType { POINT, LINE, MESH };

private:
	gl::GlslProgRef mGlsl;
	gl::BatchRef mMesh;
	struct TexEntry {
		gl::Texture2dRef mColorTex;
		gl::Texture2dRef mDepthTex;
		gl::Texture2dRef mConfidenceTex;
		mat4 viewMatrix;
	};
	std::vector<TexEntry> texturePool;
	size_t textureIndex = 0;
	const DrawType mType;
	const int mDepthRough, mColorRough;
	bool mNoTextureMode = false;
	std::optional<mat4> firstMatrix;

public:
	PointCloud(DrawType type = DrawType::MESH, int depthRough = 0, int colorRough = 0, float invisibleEdgeCoefficient = 0.1)
		: mType(type), mDepthRough(std::max(depthRough + 1, 1)), mColorRough(std::max(colorRough + 1, 1))
	{
		std::string drawType =
			mType == DrawType::MESH ? "triangle_strip" :
			mType == DrawType::LINE ? "line_strip"     :
			                          "points"         ;

		mGlsl = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(R"(
				#version 150

				in vec2 ciTexCoord0;
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
					vec2 normalizePos = vec2(ciTexCoord0.x, 1.0 - ciTexCoord0.y);
					vec2 colorPos = normalizePos * colorResolution;
					vOut.uv = ciTexCoord0;
					float depth = texture(depthTex, vOut.uv * depthUvScale).r;
					vOut.confidence = texture(confidenceTex, vOut.uv * depthUvScale).r * 255.0;

					gl_Position = vec4(
						(colorPos.x - cameraParam.x) * depth / cameraParam.z,
						(colorPos.y - cameraParam.y) * depth / cameraParam.w,
						-depth,
						1.0
					);
				}
			)")
			.geometry(R"(
				#version 150

				layout (triangles) in;
				layout ()" + drawType + R"(, max_vertices = 3) out;

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
					if (max(length(v1), length(v2)) > depth_avg * )" + std::to_string(invisibleEdgeCoefficient) + R"() return;

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
			)")
			.fragment(R"(
				#version 150

				in VertexData {
					vec2 uv;
					float confidence;
					float light;
				} vIn;
				out vec4 oColor;

				uniform sampler2D colorTex;
				uniform vec2 colorUvScale;
				uniform float alpha;
				uniform bool noTextureMode;

				uniform vec4 confidence0ColorMask;
				uniform vec4 confidence1ColorMask;
				uniform vec4 confidence2ColorMask;

				void main(void) {
					vec4 mask =
						vIn.confidence < 0.5 ? confidence0ColorMask :
						vIn.confidence < 1.5 ? confidence1ColorMask :
						                       confidence2ColorMask ;
					vec3 color =
						noTextureMode ? vec3(0.98,0.176,0.463) * vIn.light + vec3(0.494,0.235,0.812) * (1.0 - vIn.light) :
						                texture(colorTex, vIn.uv * colorUvScale).rgb;

					oColor = vec4(color, alpha) * mask;
				}
			)")
		);

		mGlsl->uniform("alpha"        , 1.0f);
		mGlsl->uniform("noTextureMode", mNoTextureMode);
		mGlsl->uniform("colorTex"     , 0);
		mGlsl->uniform("depthTex"     , 1);
		mGlsl->uniform("confidenceTex", 2);
		mGlsl->uniform("confidence0ColorMask", vec4(1.0));
		mGlsl->uniform("confidence1ColorMask", vec4(1.0));
		mGlsl->uniform("confidence2ColorMask", vec4(1.0));

		texturePool.reserve(32);
		for(size_t i = 0; i < texturePool.capacity(); i++) texturePool.push_back(TexEntry{});
	}

	void update(const qs::Camera& camera) {
		// カラー、デプス、信頼度の画像を取得
		cv::Mat color = camera.color, depth = camera.depth, confidence = camera.confidence;

		// カメラの内部パラメータの値を取得
		const float (&intrinsics)[3][3] = camera.ar.intrinsics;
		vec4 cameraParam(intrinsics[0][2], intrinsics[1][2], intrinsics[0][0], intrinsics[1][1]);

		// 点群の省略度が設定されている場合はそれに合わせてデプスと信頼度を縮小
		if (mDepthRough > 1) {
			cv::Size2i depthResolution(
				std::max(depth.cols / mDepthRough, 1),
				std::max(depth.rows / mDepthRough, 1)
			);
			cv::resize(depth     , depth     , depthResolution);
			cv::resize(confidence, confidence, depthResolution);
		}

		// 点群を点で描画する場合、カラーのテクスチャはデプス以上の解像度を必要としないので縮小する
		if (DrawType::POINT == mType) {
			vec2 rescale((float)depth.cols / (float)color.cols, (float)depth.rows / (float)color.rows);
			cameraParam *= vec4(rescale, rescale);
			cv::resize(color, color, depth.size());
		}
		// テクスチャの省略度が設定されている場合はそれに合わせてカラーを縮小
		else if (mColorRough > 1) {
			cv::Size2i colorResolution(
				std::max(color.cols / mColorRough, 1),
				std::max(color.rows / mColorRough, 1)
			);
			vec2 rescale((float)colorResolution.width / (float)color.cols, (float)colorResolution.height / (float)color.rows);
			cameraParam *= vec4(rescale, rescale);
			cv::resize(color, color, colorResolution);
		}

		// カメラ内部パラメータをシェーダに送る
		mGlsl->uniform("cameraParam", cameraParam);

		// テクスチャが未作成の場合は作成
		TexEntry& entry = texturePool[textureIndex];
		gl::Texture2dRef& mColorTex = entry.mColorTex;
		gl::Texture2dRef& mDepthTex = entry.mDepthTex;
		gl::Texture2dRef& mConfidenceTex = entry.mConfidenceTex;
		if (!mColorTex || !mDepthTex || !mConfidenceTex || !mMesh) {
			mColorTex      = gl::Texture2d::create(color.cols     , color.rows     , gl::Texture2d::Format().internalFormat(GL_SRGB8).dataType(GL_UNSIGNED_BYTE));
			mDepthTex      = gl::Texture2d::create(depth.cols     , depth.rows     , gl::Texture2d::Format().internalFormat(GL_R32F ).dataType(GL_FLOAT        ));
			mConfidenceTex = gl::Texture2d::create(confidence.cols, confidence.rows, gl::Texture2d::Format().internalFormat(GL_R8   ).dataType(GL_UNSIGNED_BYTE));
			mGlsl->uniform("colorResolution", vec2((float)mColorTex->getActualWidth(), (float)mColorTex->getActualHeight()));
			mGlsl->uniform("colorUvScale", vec2(
				(float)mColorTex->getActualWidth()  / (float)(color.cols),
				(float)mColorTex->getActualHeight() / (float)(color.rows)
			));
			mGlsl->uniform("depthUvScale", vec2(
				(float)mDepthTex->getActualWidth()  / (float)(depth.cols),
				(float)mDepthTex->getActualHeight() / (float)(depth.rows)
			));
			ivec2 subdivisions = mDepthTex->getSize() - ivec2(1);
			mMesh = gl::Batch::create(geom::Plane().subdivisions(subdivisions), mGlsl);
		}

		// テクスチャの更新
		mColorTex     ->update(color.data     , GL_BGR, GL_UNSIGNED_BYTE, 0, color.cols     , color.rows     );
		mDepthTex     ->update(depth.data     , GL_RED, GL_FLOAT        , 0, depth.cols     , depth.rows     );
		mConfidenceTex->update(confidence.data, GL_RED, GL_UNSIGNED_BYTE, 0, confidence.cols, confidence.rows);

		entry.viewMatrix = mat4(
			camera.ar.viewMatrix[0][0], camera.ar.viewMatrix[1][0], camera.ar.viewMatrix[2][0], camera.ar.viewMatrix[3][0],
			camera.ar.viewMatrix[0][1], camera.ar.viewMatrix[1][1], camera.ar.viewMatrix[2][1], camera.ar.viewMatrix[3][1],
			camera.ar.viewMatrix[0][2], camera.ar.viewMatrix[1][2], camera.ar.viewMatrix[2][2], camera.ar.viewMatrix[3][2],
			camera.ar.viewMatrix[0][3], camera.ar.viewMatrix[1][3], camera.ar.viewMatrix[2][3], camera.ar.viewMatrix[3][3]
		);
		if (!firstMatrix) { firstMatrix = entry.viewMatrix; }

		textureIndex = (textureIndex + 1) % texturePool.size();
	}

	void draw() {
		if (!mMesh || !firstMatrix) return;
		for(int index = (textureIndex - 1) % texturePool.size(); index != textureIndex; index = (index - 1) % texturePool.size()) {
			TexEntry& entry = texturePool[index];
			gl::ScopedViewMatrix mat(gl::getViewMatrix() * firstMatrix.value() * glm::inverse(entry.viewMatrix));
			gl::Texture2dRef& mColorTex = entry.mColorTex;
			gl::Texture2dRef& mDepthTex = entry.mDepthTex;
			gl::Texture2dRef& mConfidenceTex = entry.mConfidenceTex;
			if (!mColorTex || !mDepthTex || !mConfidenceTex) break;
			gl::ScopedTextureBind tex0(mColorTex     , 0);
			gl::ScopedTextureBind tex1(mDepthTex     , 1);
			gl::ScopedTextureBind tex2(mConfidenceTex, 2);
			mMesh->draw();
		}
	}

	void setNoTextureMode(bool value) { mNoTextureMode = value; mGlsl->uniform("noTextureMode", mNoTextureMode); }
	bool getNoTextureMode() const     { return mNoTextureMode; }

	void setAlpha(float alpha) { mGlsl->uniform("alpha", alpha); }
	void setConfidenceColorMask(vec4 c0ColorMask, vec4 c1ColorMask, vec4 c2ColorMask) {
		mGlsl->uniform("confidence0ColorMask", c0ColorMask);
		mGlsl->uniform("confidence1ColorMask", c1ColorMask);
		mGlsl->uniform("confidence2ColorMask", c2ColorMask);
	}
};

class PreviewApp : public App {
private:
	qs::QuadLoader mLoader;
	PointCloud mPoints{ PointCloud::DrawType::MESH, 0, 0 };

	bool mPlay = false;

	CameraPersp			mCamera;
	CameraUi			mCamUi;

	mat4 mFirst;

	std::list<vec4> trajectory;

public:
	void setup() override {
		// QuadLoaderの初期化
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
		mLoader.open(recDirPath);
		if (!mLoader.isOpened()) {
			std::cout << "failed to open forder" << std::endl;
			quit();
			return;
		}

		// Cinderの初期化
		gl::enableDepthWrite();
		gl::enableDepthRead();

		setWindowSize(1280, 720);
		mCamera.lookAt(vec3(0), vec3(0, 0, -1));
		mCamUi = CameraUi(&mCamera);

		updatePoints();
		//mPoints.setConfidenceColorMask(vec4(1, 1, 1, 0.1), vec4(1, 1, 1, 0.1), vec4(1, 1, 1, 1));
	}

	mat4 viewMatrix;
	mat4 projection;
	mat3 intrinsics;
	void updatePoints() {
		// 次のフレームを取得
		auto quad = mLoader.next();
		if (!quad.has_value()) { quit(); return; }
		qs::QuadFrame& quadFrame = quad.value();
		qs::Camera camera = quadFrame.camera;
		if (camera.color.empty() || camera.depth.empty() || camera.confidence.empty()) { return; }

		// 点群の更新
		static int count = 0;
		if (count == 0) mPoints.update(camera);
		count = (count + 1) % 30;

		viewMatrix = mat4(
			camera.ar.viewMatrix[0][0], camera.ar.viewMatrix[1][0], camera.ar.viewMatrix[2][0], camera.ar.viewMatrix[3][0],
			camera.ar.viewMatrix[0][1], camera.ar.viewMatrix[1][1], camera.ar.viewMatrix[2][1], camera.ar.viewMatrix[3][1],
			camera.ar.viewMatrix[0][2], camera.ar.viewMatrix[1][2], camera.ar.viewMatrix[2][2], camera.ar.viewMatrix[3][2],
			camera.ar.viewMatrix[0][3], camera.ar.viewMatrix[1][3], camera.ar.viewMatrix[2][3], camera.ar.viewMatrix[3][3]
		);

		projection = mat4(
			camera.ar.projectionMatrix[0][0], camera.ar.projectionMatrix[1][0], camera.ar.projectionMatrix[2][0], camera.ar.projectionMatrix[3][0],
			camera.ar.projectionMatrix[0][1], camera.ar.projectionMatrix[1][1], camera.ar.projectionMatrix[2][1], camera.ar.projectionMatrix[3][1],
			camera.ar.projectionMatrix[0][2], camera.ar.projectionMatrix[1][2], camera.ar.projectionMatrix[2][2], camera.ar.projectionMatrix[3][2],
			camera.ar.projectionMatrix[0][3], camera.ar.projectionMatrix[1][3], camera.ar.projectionMatrix[2][3], camera.ar.projectionMatrix[3][3]
		);

		intrinsics = mat3(
			camera.ar.intrinsics[0][0], camera.ar.intrinsics[1][0], camera.ar.intrinsics[2][0],
			camera.ar.intrinsics[0][1], camera.ar.intrinsics[1][1], camera.ar.intrinsics[2][1],
			camera.ar.intrinsics[0][2], camera.ar.intrinsics[1][2], camera.ar.intrinsics[2][2]
		);

		static bool init = true; if (init) { mFirst = viewMatrix; init = false; }
		trajectory.push_back(mFirst * glm::inverse(viewMatrix) * vec4(0, 0, 0, 1));
		for (;trajectory.size() > 512;) { trajectory.pop_front(); }
	}

	void update() override { if (mPlay) updatePoints(); }

	void draw() override {
		// カメラのアスペクト比を更新
		mCamera.setAspectRatio(getWindowAspectRatio());

		// カメラのMVP行列をOpenGLに適応
		gl::setProjectionMatrix(mCamera.getProjectionMatrix());
		gl::setViewMatrix(mCamera.getViewMatrix());

		// 背景色の設定
		gl::clear(Color::gray(0.1f));

		// ポリゴンのウラ面を非表示
		gl::ScopedFaceCulling cull(true, GL_BACK);

		// 点群の描画
		{
			gl::ScopedViewMatrix mat(gl::getViewMatrix() * viewMatrix * glm::inverse(mFirst));
			gl::pointSize(3.0f);
			mPoints.draw();
		}

		{
			//gl::ScopedViewMatrix mat(gl::getViewMatrix() * mFirst * glm::inverse(viewMatrix));
			mat4 projInv = glm::inverse(projection);
			vec3 resolution = vec3(1920.0, 1440.0, 0.0);
			float f = intrinsics[0][0];
			vec3 o  = vec3(0, 0, 0);
			vec3 p1 = vec3(-960, -720, -f) / f;
			vec3 p2 = vec3(+960, -720, -f) / f;
			vec3 p3 = vec3(+960, +720, -f) / f;
			vec3 p4 = vec3(-960, +720, -f) / f;

			gl::color(1.0f, 1.0f, 1.0f, 1.0f);
			gl::lineWidth(4.0);
			gl::begin(GL_LINES);
			gl::vertex(o);  gl::vertex(p1);
			gl::vertex(o);  gl::vertex(p2);
			gl::vertex(o);  gl::vertex(p3);
			gl::vertex(o);  gl::vertex(p4);
			gl::vertex(p1); gl::vertex(p2);
			gl::vertex(p2); gl::vertex(p3);
			gl::vertex(p3); gl::vertex(p4);
			gl::vertex(p4); gl::vertex(p1);
			gl::end();
		}

		{
			gl::ScopedViewMatrix mat(gl::getViewMatrix() * viewMatrix * glm::inverse(mFirst));
			gl::color(1.0f, 1.0f, 1.0f, 1.0f);
			gl::lineWidth(4.0);
			gl::begin(GL_LINE_STRIP);
			for(auto&& p : trajectory) gl::vertex(p);
			gl::end();
		}
	}

	// マウスイベント
	void mouseUp(MouseEvent event) override { mCamUi.mouseUp(event); }
	void mouseDown(MouseEvent event) override { mCamUi.mouseDown(event); }
	void mouseWheel(MouseEvent event) override { mCamUi.mouseWheel(event); }
	void mouseDrag(MouseEvent event) override { mCamUi.mouseDrag(event); }

	// キーイベント
	void keyDown(KeyEvent e) override {
		switch(e.getCode()) {

		// スペースキーで再生/一時停止の切り替え
		case KeyEvent::KEY_SPACE:
			mPlay = !mPlay;
			break;

		// Aキーでテキスチャの有無を切り替え
		case KeyEvent::KEY_a:
			mPoints.setNoTextureMode(!mPoints.getNoTextureMode());
			break;

		// Rキーでカメラをリセット
		case KeyEvent::KEY_r:
			mCamera.lookAt(vec3(0), vec3(0, 0, -1));
			break;

		// QキーまたはESCでカメラをリセット
		case KeyEvent::KEY_ESCAPE:
		case KeyEvent::KEY_q:
			quit();
			break;

		}
	}
};

CINDER_APP( PreviewApp, RendererGl )
