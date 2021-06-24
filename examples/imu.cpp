#include <iostream>
#include "loader/imu_loader.h"

int main(int argc, char* argv[]) {
	if (2 != argc) {
		std::cout
			<< "example_imu version 0.0.1\n"
			<< "\n"
			<< "usage: example_imu input_path\n"
			<< "  input_path: Directory containing QuadDump recording files"
			<< "\n"
			<< std::endl;
		return 0;
	}

	std::string recDirPath = argv[1];
	qs::IMULoader loader;
	loader.open(recDirPath);
	if (!loader.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }

	while(true) {
		auto imu = loader.next();
		if (!imu.has_value()) break;
		qs::IMU& imu_ = imu.value();

		std::cout
			<< "================================\n"
			<< "timestamp      : " << imu_.timestamp           << "\n"
			<< "gravity        : " << imu_.cvGravity()         << "\n"
			<< "userAccleration: " << imu_.cvUserAccleration() << "\n"
			<< "attitude       : " << imu_.cvAttitude()        << std::endl;

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
