#include <iostream>
#include "loader/gps_loader.h"

int main(int argc, char* argv[]) {
	if (2 != argc) {
		std::cout
			<< "example_gps version 0.0.1\n"
			<< "\n"
			<< "usage: example_gps input_path\n"
			<< "  input_path: Directory containing QuadDump recording files"
			<< "\n"
			<< std::endl;
		return 0;
	}

	std::string recDirPath = argv[1];
	qs::GPSLoader loader;
	loader.open(recDirPath);
	if (!loader.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }

	while(true) {
		auto gps = loader.next();
		if (!gps.has_value()) break;
		qs::GPS& gps_ = gps.value();

		std::cout
			<< "================================\n"
			<< "timestamp                      : " << gps_.timestamp          << "\n"
			<< "[latitude, longitude, altitude]: " << gps_.cvGPS()            << "\n"
			<< "horizontalAccuracy             : " << gps_.horizontalAccuracy << "\n"
			<< "verticalAccuracy               : " << gps_.verticalAccuracy   << std::endl;

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
