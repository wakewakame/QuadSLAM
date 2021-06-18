#pragma once
#include <iostream>
#include <fstream>
#include <stdint.h>
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "zlib.h"
#include "nlohmann/json.hpp"

namespace qs {
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

	class QuadSLAM {
	public:
		QuadSLAM(const std::string& recDirPath) {
			const std::string
				infoJsonPath        = recDirPath + "/info.json",
				cameraFrameInfoPath = recDirPath + "/cameraFrameInfo",
				cameraPath          = recDirPath + "/camera.mp4",
				depthPath           = recDirPath + "/depth",
				confidencePath      = recDirPath + "/confidence",
				imuPath             = recDirPath + "/imu",
				gpsPath             = recDirPath + "/gps";

			std::ifstream infoJsonFs(infoJsonPath, std::ios_base::in | std::ios_base::binary);
			if (infoJsonFs.is_open()) {
				std::string infoJsonString((std::istreambuf_iterator<char>(infoJsonFs)), std::istreambuf_iterator<char>());
				nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);
				std::cout << infoJson.dump(4) << std::endl;
				infoJsonFs.close();
			}
		}
		virtual ~QuadSLAM() {}

		void next() {
		
		}
	};
}
