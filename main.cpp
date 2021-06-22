#include "quadslam.h"

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
	qs::QuadSLAM qs;
	qs.open(recDirPath);

	return 0;
}

