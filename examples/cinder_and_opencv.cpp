#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// メモ
// "loader/quad_loader.h"を"cinder/gl/gl.h"よりも前にincludeすると
// OpenCV内でdefineされるFARがcinderのコードを置換してエラーが発生してしまう
#include "loader/quad_loader.h"

#include <iostream>

using namespace ci;
using namespace ci::app;

class BasicApp : public App {
private:
	qs::QuadLoader loader;

public:
	void setup() override {
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
		}

		std::string recDirPath = args.at(1);
		loader.open(recDirPath);
		if (!loader.isOpened()) {
			std::cout << "failed to open forder" << std::endl;
			quit();
		}
	}
	void draw() override {
		gl::clear( Color::gray( 0.1f ) );
		gl::color( 1.0f, 0.5f, 0.25f );
		gl::begin( GL_TRIANGLES );
		gl::vertex( 300.0, 100.0 );
		gl::vertex( 100.0, 400.0 );
		gl::vertex( 500.0, 400.0 );
		gl::end();

		auto quad = loader.next();
		if (!quad.has_value()) quit();
		qs::QuadFrame& quadFrame = quad.value();
		qs::Camera camera = quadFrame.camera;

		camera.depth *= 0.1;
		camera.confidence *= 255 / 2;

		auto resolution = camera.depth.size() * 2;
		cv::resize(camera.color     , camera.color     , resolution);
		cv::resize(camera.depth     , camera.depth     , resolution);
		cv::resize(camera.confidence, camera.confidence, resolution);

		cv::imshow("camera"    , camera.color     );
		cv::imshow("depth"     , camera.depth     );
		cv::imshow("confidence", camera.confidence);

		std::cout
			<< "================================\n"
			<< "camera timestamp: " << quadFrame.camera.timestamp << "\n"
			<< "imu    timestamp: " << quadFrame.imu.timestamp    << "\n"
			<< "gps    timestamp: " << quadFrame.gps.timestamp    << std::endl;

		if ('q' == cv::waitKey(1)) quit();
	}
};

CINDER_APP( BasicApp, RendererGl )
