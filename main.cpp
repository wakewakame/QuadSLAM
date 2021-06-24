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

		frame_.depth *= 0.1;
		frame_.confidence *= 255 / 2;

		auto resolution = frame_.depth.size() * 2;
		cv::resize(frame_.camera    , frame_.camera    , resolution);
		cv::resize(frame_.depth     , frame_.depth     , resolution);
		cv::resize(frame_.confidence, frame_.confidence, resolution);

		cv::imshow("camera", frame_.camera);
		cv::imshow("depth", frame_.depth);
		cv::imshow("confidence", frame_.confidence);

		cv::Mat intrinsics(3, 3, CV_32FC1);
		cv::Mat projectionMatrix(4, 4, CV_32FC1);
		cv::Mat viewMatrix(4, 4, CV_32FC1);
		std::memcpy(intrinsics.data, frame_.ar.intrinsics, sizeof(frame_.ar.intrinsics));
		std::memcpy(projectionMatrix.data, frame_.ar.projectionMatrix, sizeof(frame_.ar.projectionMatrix));
		std::memcpy(viewMatrix.data, frame_.ar.viewMatrix, sizeof(frame_.ar.viewMatrix));
		std::cout
			<< "================================\n"
			<< intrinsics << "\n"
			<< projectionMatrix << "\n"
			<< viewMatrix << std::endl;

		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}
