#include <iostream>
#include "quad_loader.h"

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
	qs::QuadLoader loader;
	loader.open(recDirPath);
	if (!loader.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }
	qs::QSStorage& storage = *loader.getStorage();

	for(auto gps : storage.iterate<qs::Gps>(sqlite_orm::order_by(&qs::Gps::timestamp).asc())) {
		std::cout
			<< "================================\n"
			<< "timestamp                      : " << gps.timestamp          << "\n"
			<< "[latitude, longitude, altitude]: " << gps.cvGps()            << "\n"
			<< "horizontalAccuracy             : " << gps.horizontalAccuracy << "\n"
			<< "verticalAccuracy               : " << gps.verticalAccuracy   << std::endl;
	}

	return 0;
}
