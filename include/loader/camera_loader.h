#pragma once
#include <optional>
#include <fstream>
#include <stdint.h>
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "zlib.h"
#include "nlohmann/json.hpp"
#include "types.h"

namespace qs {
	struct CameraLoader {
		CameraLoader();
		virtual ~CameraLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened() const;
		std::optional<Camera> next();

	private:
		bool isOpened_ = false;
		cv::VideoCapture colorCap;
		std::ifstream arFs;
		struct ZlibImage {
			std::ifstream fs;
			std::vector<uint8_t> src, dst;
			int width, height;
		} depthZlib, confidenceZlib;
	};
}
