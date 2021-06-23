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
		InfoJson infoJson;

		cv::VideoCapture cameraCap;
		std::ifstream depthFs, confidenceFs;

		cv::Mat camera, depth, confidence;

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
			try {
				nlohmann::json infoJson_ = nlohmann::json::parse(infoJsonString);
				infoJson.camera.width = infoJson_.at("camera.mp4").at("width");
				infoJson.camera.height = infoJson_.at("camera.mp4").at("height");
				infoJson.depth.width = infoJson_.at("depth").at("width");
				infoJson.depth.height = infoJson_.at("depth").at("height");
				infoJson.confidence.width = infoJson_.at("confidence").at("width");
				infoJson.confidence.height = infoJson_.at("confidence").at("height");
			}
			catch(const nlohmann::detail::out_of_range& e) { close(); return Err("failed to parse info.json"); }

			// カメラの映像を取得
			cameraCap.open(cameraPath);
			if (!cameraCap.isOpened()) { close(); return Err("failed to open camera.mp4"); }

			// デプスの映像を取得
			depthFs.open(depthPath, std::ios_base::in | std::ios_base::binary);
			if (!depthFs.is_open()) { close(); return Err("failed to open depth"); }

			// 信頼度の映像を取得
			confidenceFs.open(confidencePath, std::ios_base::in | std::ios_base::binary);
			if (!confidenceFs.is_open()) { close(); return Err("failed to open confidence"); }

			isOpen_ = true;
			return Ok();
		}

		void close() {
			isOpen_ = false;
		}

		Result<void, const char*> next() {
			if (!isOpen_) { return Err("already closed"); }

			// カメラの取得
			cameraCap >> camera;
			cv::resize(camera, camera, cv::Size(infoJson.depth.width, infoJson.depth.height));

			// デプスの取得
			depth = cv::Mat::zeros(infoJson.depth.height, infoJson.depth.width, CV_8UC3);
			int64_t src_size;
			depthFs.read((char*)&src_size, sizeof(src_size));
			std::vector<uint8_t> src(src_size);
			depthFs.read((char*)src.data(), src_size);
			cv::Mat dst = cv::Mat(192, 256, CV_32FC1);
			z_stream stream;
			memset(&stream, 0, sizeof(z_stream));
			stream.next_in = (Bytef*)src.data();
			stream.avail_in = src.size();
			stream.next_out = (Bytef*)dst.data;
			stream.avail_out = 192 * 256 * 4;
			inflateInit2(&stream, -15);
			int result = inflate(&stream, Z_FINISH);
			if (Z_STREAM_END != result && Z_OK != result) { return Err("failed to read depth frame"); }
			inflateEnd(&stream);
			for (int i = 0; i < 192; i++) {
				for (int j = 0; j < 256; j++) {
					depth.at<cv::Vec3b>(i, j)[0] =
						depth.at<cv::Vec3b>(i, j)[1] =
						depth.at<cv::Vec3b>(i, j)[2] = (uint8_t)(dst.at<float>(i, j) * 255.0 / 10.0);
				}
			}

			// 信頼度の取得
			confidence = cv::Mat::zeros(infoJson.confidence.height, infoJson.confidence.width, CV_8UC3);
			confidenceFs.read((char*)&src_size, sizeof(src_size));
			src.resize(src_size);
			confidenceFs.read((char*)src.data(), src_size);
			dst = cv::Mat(192, 256, CV_8UC1);
			memset(&stream, 0, sizeof(z_stream));
			stream.next_in = (Bytef*)src.data();
			stream.avail_in = src.size();
			stream.next_out = (Bytef*)dst.data;
			stream.avail_out = 192 * 256 * 4;
			inflateInit2(&stream, -15);
			result = inflate(&stream, Z_FINISH);
			if (Z_STREAM_END != result && Z_OK != result) { return Err("failed to read confidence frame"); }
			inflateEnd(&stream);
			for (int i = 0; i < 192; i++) {
				for (int j = 0; j < 256; j++) {
					confidence.at<cv::Vec3b>(i, j)[0] =
						confidence.at<cv::Vec3b>(i, j)[1] =
						confidence.at<cv::Vec3b>(i, j)[2] = (uint8_t)((double)dst.at<uint8_t>(i, j) * 255.0 / 3.0);
				}
			}

			return Ok();
		}
	};
}
