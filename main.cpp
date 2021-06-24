#include <iostream>
#include "loader/quad_loader.h"

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
	qs::QuadLoader loader;
	loader.open(recDirPath);
	if (!loader.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }

	while(true) {
		auto quad = loader.next();
		if (!quad.has_value()) break;
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

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
