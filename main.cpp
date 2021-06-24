#include "quad_loader.h"

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
		qs::Frame& frame_ = frame.value();

		frame_.depth *= 0.1;
		frame_.confidence *= 255 / 2;

		auto resolution = frame_.depth.size() * 2;
		cv::resize(frame_.camera    , frame_.camera    , resolution);
		cv::resize(frame_.depth     , frame_.depth     , resolution);
		cv::resize(frame_.confidence, frame_.confidence, resolution);

		cv::imshow("camera", frame_.camera);
		cv::imshow("depth", frame_.depth);
		cv::imshow("confidence", frame_.confidence);

		std::cout
			<< "================================\n"
			<< frame_.ar.cvIntrinsics()       << "\n"
			<< frame_.ar.cvProjectionMatrix() << "\n"
			<< frame_.ar.cvViewMatrix()       << std::endl;

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
