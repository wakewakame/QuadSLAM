#pragma once
#include <iostream>
#include <optional>
#include <fstream>
#include <stdint.h>
#include <exception>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "zlib.h"
#include "nlohmann/json.hpp"
#include "types.h"

namespace qs {
	struct QuadLoader {
		QuadLoader();
		virtual ~QuadLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened();
		std::optional<Frame> next();

	private:
		std::optional<Frame> frame;
		cv::VideoCapture cameraCap;
		std::ifstream arFs;
		std::ifstream depthFs, confidenceFs;
		std::ifstream imuFs, gpsFs;
	};
}
