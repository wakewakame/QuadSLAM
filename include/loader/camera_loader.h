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
	struct CameraLoader {
		CameraLoader();
		virtual ~CameraLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened();
		std::optional<Camera> next();

	private:
		std::optional<Camera> camera;
		cv::VideoCapture colorCap;
		std::ifstream arFs;
		std::ifstream depthFs, confidenceFs;
	};
}
