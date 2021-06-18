#include <iostream>
#include <fstream>
#include <stdint.h>
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "zlib.h"

struct FrameInfo {
	uint64_t frameNumber;
	double timestamp;
	uint8_t isColorFrameExist, isDepthFrameExist, isConfidenceFrameExist;
	uint64_t depthOffset;
	uint64_t confidenceOffset;
	float intrinsics[3][3];
	float projectionMatrix[4][4];
	float viewMatrix[4][4];
};

std::istream& operator>>(std::istream& left, FrameInfo& right) {
	left.read((char*)&right.frameNumber, sizeof(right.frameNumber));
	left.read((char*)&right.timestamp, sizeof(right.timestamp));
	left.read((char*)&right.isColorFrameExist, sizeof(right.isColorFrameExist));
	left.read((char*)&right.isDepthFrameExist, sizeof(right.isDepthFrameExist));
	left.read((char*)&right.isConfidenceFrameExist, sizeof(right.isConfidenceFrameExist));
	left.read((char*)&right.depthOffset, sizeof(right.depthOffset));
	left.read((char*)&right.confidenceOffset, sizeof(right.confidenceOffset));
	left.read((char*)&right.intrinsics, sizeof(right.intrinsics));
	left.read((char*)&right.projectionMatrix, sizeof(right.projectionMatrix));
	left.read((char*)&right.viewMatrix, sizeof(right.viewMatrix));
	return left;
}

int main(int argc, char* argv[])
{
	if (2 != argc)
	{
		std::cout
			<< "QuadSLAM version 0.0.1\n"
			<< "\n"
			<< "usage: quadslam input_path\n"
			<< "  input_path: Directory containing QuadDump recording files"
			<< "\n"
			<< std::endl;
		return 0;
	}

	std::string quaddump_dir = argv[1];
	std::string camera_frame_info_path = quaddump_dir + "/cameraFrameInfo";
	std::string camera_path = quaddump_dir + "/camera.mp4";
	std::string depth_path = quaddump_dir + "/depth";

	/// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	std::ifstream ffs(camera_frame_info_path, std::ios_base::in | std::ios_base::binary);
	auto get_frame_info = [&ffs]() {
		FrameInfo frameInfo;
		ffs >> frameInfo;
		std::cout
			<< "frame_number: " << frameInfo.frameNumber << "\n"
			<< "timestamp: " << frameInfo.timestamp << "\n"
			<< std::endl;
		return frameInfo;
	};
	std::ifstream dfs(depth_path, std::ios_base::in | std::ios_base::binary);
	auto get_depth = [&dfs]() {
		cv::Mat image = cv::Mat::zeros(192, 256, CV_8UC3);

		uint64_t src_size;
		dfs.read((char*)&src_size, sizeof(src_size));
		std::vector<uint8_t> src(src_size);
		dfs.read((char*)src.data(), src_size);
		cv::Mat dst = cv::Mat(192, 256, CV_32FC1);
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)src.data();
		stream.avail_in = src.size();
		stream.next_out = (Bytef*)dst.data;
		stream.avail_out = 192 * 256 * 4;
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result)
		{
			return image;
		}
		inflateEnd(&stream);

		for (int i = 0; i < 192; i++)
		{
			for (int j = 0; j < 256; j++)
			{
				image.at<cv::Vec3b>(i, j)[0] =
					image.at<cv::Vec3b>(i, j)[1] =
					image.at<cv::Vec3b>(i, j)[2] = (uint8_t)(dst.at<float>(i, j) * 255.0 / 10.0);
			}
		}
		return image;
	};
	/// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

	cv::VideoCapture video;
	video.open(camera_path);
	if (!video.isOpened())
	{
		std::cout << "failed to open the video file" << std::endl;
		return 0;
	}

	cv::Mat image;
	while (true)
	{
		video >> image;
		if (image.empty()) break;
		auto frame = get_frame_info();
		auto depth = get_depth();
		cv::resize(depth, depth, cv::Size(image.cols / 4, image.rows / 4));
		cv::resize(image, image, cv::Size(image.cols / 4, image.rows / 4));
		cv::imshow("preview", image);
		cv::imshow("preview2", depth);
		if ('q' == cv::waitKey(1)) break;
	}

	return 0;
}