#include <iostream>
#include "quad_loader.h"

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
	qs::QuadLoader loader;
	loader.open(recDirPath);
	if (!loader.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }
	qs::QSStorage& storage = *loader.getStorage();

	for(auto imu : storage.iterate<qs::Imu>(sqlite_orm::order_by(&qs::Imu::timestamp).asc())) {
		std::cout
			<< "================================\n"
			<< "timestamp      : " << imu.timestamp           << "\n"
			<< "gravity        : " << imu.cvGravity()         << "\n"
			<< "userAccleration: " << imu.cvUserAccleration() << "\n"
			<< "rotationRate   : " << imu.cvRotationRate()    << "\n"
			<< "attitude       : " << imu.cvAttitude()        << std::endl;
	}

	return 0;
}
