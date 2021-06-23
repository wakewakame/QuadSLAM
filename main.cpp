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
	auto result = qs.open(recDirPath);
	if (result.is_err()) { std::cout << result.unwrap_err() << std::endl; return 1; }

	while(true) {
		qs.next();
		if (qs.camera.empty()) break;
		cv::imshow("camera", qs.camera);
		cv::imshow("depth", qs.depth);
		cv::imshow("confidence", qs.confidence);
		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}

