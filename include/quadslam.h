#pragma once
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <exception>
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "zlib.h"
#include "nlohmann/json.hpp"
#include "utils.h"

namespace qs {
	struct InfoJson {
		struct Resolution { int width, height; } camera, depth, confidence;
	};

	struct CameraFrameInfo {
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
	private:
		bool isOpen_ = false;

	public:
		QuadSLAM() {}
		virtual ~QuadSLAM() { close(); }

		Result<void, const char*> open(const std::string& recDirPath) {
			// 今まで開いていたファイルをすべて閉じる
			close();

			// 各ファイルのファイルパス
			const std::string
				infoJsonPath        = recDirPath + "/info.json",
				cameraFrameInfoPath = recDirPath + "/cameraFrameInfo",
				cameraPath          = recDirPath + "/camera.mp4",
				depthPath           = recDirPath + "/depth",
				confidencePath      = recDirPath + "/confidence",
				imuPath             = recDirPath + "/imu",
				gpsPath             = recDirPath + "/gps";

			// 録画データ全体の情報を取得
			std::ifstream infoJsonFs(infoJsonPath, std::ios_base::in | std::ios_base::binary);
			if ((!infoJsonFs.is_open()) || (!infoJsonFs.good())) { close(); return Err("failed to open info.json"); }
			std::string infoJsonString((std::istreambuf_iterator<char>(infoJsonFs)), std::istreambuf_iterator<char>());
			infoJsonFs.close();
			try{
				nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);
				std::cout << infoJson.at("camera.mp4") << std::endl;
			}
			catch(const nlohmann::detail::out_of_range& e) { close(); return Err("failed to parse info.json"); }

			isOpen_ = true;
			return Ok();
		}

		void close() {
			isOpen_ = false;
		}

		void next() {
		}
	};
}
