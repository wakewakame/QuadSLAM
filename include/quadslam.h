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

namespace qs {
	struct QuadLoader {
		struct IMU {
		};
		struct GPS {
		};
		struct AR {
			float intrinsics[3][3];
			float projectionMatrix[4][4];
			float viewMatrix[4][4];
		};
		struct Frame {
			uint64_t timestamp;
			cv::Mat camera;
			std::optional<cv::Mat> depth;
			std::optional<cv::Mat> confidence;
			std::optional<IMU> imu;
			std::optional<GPS> gps;
			std::optional<AR> ar;
		};

		std::optional<Frame> frame;
		cv::VideoCapture cameraCap;
		std::ifstream depthFs, confidenceFs;

		QuadLoader() : frame(std::nullopt) {}
		virtual ~QuadLoader() {}
		void open(std::string recDirPath) {
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
			if ((!infoJsonFs.is_open()) || (!infoJsonFs.good())) { close(); return; }
			std::string infoJsonString((std::istreambuf_iterator<char>(infoJsonFs)), std::istreambuf_iterator<char>());
			infoJsonFs.close();
			try {
				nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);

				int cameraWidth = infoJson.at("camera.mp4").at("width");
				int cameraHeight = infoJson.at("camera.mp4").at("height");
				frame = Frame();
				Frame& frame_ = frame.value();
				frame_.camera = cv::Mat::zeros(cameraHeight, cameraWidth, CV_8UC3);

				int depthWidth = infoJson.at("depth").at("width");
				int depthHeight = infoJson.at("depth").at("height");
				frame_.depth = cv::Mat::zeros(depthHeight, depthWidth, CV_32FC1);

				int confidenceWidth = infoJson.at("confidence").at("width");
				int confidenceHeight = infoJson.at("confidence").at("height");
				frame_.confidence = cv::Mat::zeros(confidenceHeight, confidenceWidth, CV_8UC1);
			}
			catch(const nlohmann::detail::out_of_range& e) { close(); return; }

			// カメラの映像を取得
			cameraCap.open(cameraPath);
			if (!cameraCap.isOpened()) { close(); return; }

			// デプスの映像を取得
			depthFs.open(depthPath, std::ios_base::in | std::ios_base::binary);
			if (!depthFs.is_open()) { close(); frame.value().depth = std::nullopt; return; }

			// 信頼度の映像を取得
			confidenceFs.open(confidencePath, std::ios_base::in | std::ios_base::binary);
			if (!confidenceFs.is_open()) { close(); frame.value().confidence = std::nullopt; return; }

			return;
		}
		void close() {
			if (cameraCap.isOpened()) cameraCap.release();
			if (depthFs.is_open()) depthFs.close();
			if (confidenceFs.is_open()) confidenceFs.close();
			frame = std::nullopt;
		}
		bool isOpened() {
			return frame.has_value();
		}
		std::optional<Frame> next() {
			if (!frame.has_value()) { return std::nullopt; }
			Frame& frame_ = frame.value();

			// カメラの取得
			cameraCap >> frame_.camera;
			cv::resize(frame_.camera, frame_.camera, cv::Size(256, 192));

			// デプスの取得
			if (!frame_.depth.has_value()) { return frame; }
			cv::Mat &depth = frame_.depth.value();
			int64_t src_size;
			depthFs.read((char*)&src_size, sizeof(src_size));
			std::vector<uint8_t> src(src_size);
			depthFs.read((char*)src.data(), src_size);
			z_stream stream;
			memset(&stream, 0, sizeof(z_stream));
			stream.next_in = (Bytef*)src.data();
			stream.avail_in = src.size();
			stream.next_out = (Bytef*)depth.data;
			stream.avail_out = 192 * 256 * 4;
			inflateInit2(&stream, -15);
			int result = inflate(&stream, Z_FINISH);
			if (Z_STREAM_END != result && Z_OK != result) { return frame; }
			inflateEnd(&stream);

			// 信頼度の取得
			if (!frame_.confidence.has_value()) { return frame; }
			cv::Mat &confidence = frame_.confidence.value();
			confidenceFs.read((char*)&src_size, sizeof(src_size));
			src.resize(src_size);
			confidenceFs.read((char*)src.data(), src_size);
			memset(&stream, 0, sizeof(z_stream));
			stream.next_in = (Bytef*)src.data();
			stream.avail_in = src.size();
			stream.next_out = (Bytef*)confidence.data;
			stream.avail_out = 192 * 256 * 4;
			inflateInit2(&stream, -15);
			result = inflate(&stream, Z_FINISH);
			if (Z_STREAM_END != result && Z_OK != result) { return frame; }
			inflateEnd(&stream);

			return frame;
		}
	};
}
