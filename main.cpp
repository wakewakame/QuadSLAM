#include <iostream>
#include "loader/camera_loader.h"

int main(int argc, char* argv[]) {
	if (2 != argc) {
		std::cout
			<< "QuadSLAM version 0.0.1\n"
			<< "\n"
			<< "usage: quadslam input_path\n"
			<< "  input_path: Directory containing QuadDump recording files"
			<< "\n"
			<< std::endl;
		return 0;
	}

	std::string recDirPath = argv[1];
	qs::CameraLoader cl;
	cl.open(recDirPath);
	if (!cl.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }

	while(true) {
		auto camera = cl.next();
		if (!camera.has_value()) break;
		qs::Camera& camera_ = camera.value();

		camera_.depth *= 0.1;
		camera_.confidence *= 255 / 2;

		auto resolution = camera_.depth.size() * 2;
		cv::resize(camera_.color     , camera_.color     , resolution);
		cv::resize(camera_.depth     , camera_.depth     , resolution);
		cv::resize(camera_.confidence, camera_.confidence, resolution);

		cv::imshow("camera"    , camera_.color     );
		cv::imshow("depth"     , camera_.depth     );
		cv::imshow("confidence", camera_.confidence);

		/*
		std::cout
			<< "================================\n"
			<< camera_.ar.cvIntrinsics()       << "\n"
			<< camera_.ar.cvProjectionMatrix() << "\n"
			<< camera_.ar.cvViewMatrix()       << std::endl;
		*/

		camera_.depth.release();

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
