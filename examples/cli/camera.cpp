#include <iostream>
#include "quad_loader.h"

int main(int argc, char* argv[]) {
	if (2 != argc) {
		std::cout
			<< "example_camera version 0.0.1\n"
			<< "\n"
			<< "usage: example_camera input_path\n"
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
		if (!quad) break;
		qs::QuadFrame quadFrame = std::move(*quad);
		qs::Camera& camera = quadFrame.camera;

		camera.depth *= 0.1;
		camera.confidence *= 255 / 2;

		auto resolution = camera.depth.size() * 2;
		cv::resize(camera.color     , camera.color     , resolution);
		cv::resize(camera.depth     , camera.depth     , resolution);
		cv::resize(camera.confidence, camera.confidence, resolution);

		cv::imshow("camera"    , camera.color     );
		cv::imshow("depth"     , camera.depth     );
		cv::imshow("confidence", camera.confidence);


		std::cout << "================================\n";
		std::cout << "imu timestamp:\n";
		for(const auto& imu : quadFrame.imu) std::cout << "\t" << imu.timestamp << "\n";
		std::cout << "gps timestamp:\n";
		for(const auto& gps : quadFrame.gps) std::cout << "\t" << gps.timestamp << "\n";
		std::cout << "camera timestamp: " << quadFrame.camera.timestamp << "\n";
		std::cout << std::endl;

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
