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
	qs::QuadLoader ql;
	ql.open(recDirPath);
	if (!ql.isOpened()) { std::cout << "failed to open forder" << std::endl; return 1; }

	while(true) {
		auto frame = ql.next();
		if (!frame.has_value()) break;
		qs::QuadLoader::Frame& frame_ = frame.value();
		cv::imshow("camera", frame_.camera);
		if (frame_.depth.has_value()) cv::imshow("depth", frame_.depth.value());
		if (frame_.confidence.has_value()) cv::imshow("confidence", frame_.confidence.value());
		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}

