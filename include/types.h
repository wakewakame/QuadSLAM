#pragma once
#include <stdint.h>
#include <optional>
#include "opencv2/opencv.hpp"
#include "sqlite_orm/sqlite_orm.h"

namespace qs {
	struct Description {
		std::string date;
		uint64_t colorWidth;
		uint64_t colorHeight;
		std::optional<uint64_t> depthWidth, depthHeight;
		std::optional<uint64_t> confidenceWidth, confidenceHeight;
	};

	struct Camera {
		/*
			メモ
			OpenCV 4.x より、cv::Matにムーブコンストラクタとムーブ代入演算子が実装された。
			そのため、Cameraには暗黙的にコピーコンストラクタ、ムーブコンストラクタ、コピー代入演算子、ムーブ代入演算子が定義される。
			また、cv::Matは内部で参照カウンタを持っており、通常のコピーではデータの参照先は同じになる。
			そのため、深いコピーを行うときにはCamera::clone()メソッドを使用する。
		*/

		double timestamp;
		cv::Mat color;
		cv::Mat depth;
		cv::Mat confidence;
		cv::Mat intrinsicsMatrix;
		cv::Mat projectionMatrix;
		cv::Mat viewMatrix;

		Camera clone() const;
	};

	struct Gps {
		uint64_t id;
		double timestamp;
		double latitude;
		double longitude;
		double altitude;
		double horizontalAccuracy;
		double verticalAccuracy;

		cv::Vec3d cvGps() const;
	};

	struct Imu {
		uint64_t id;
		double timestamp;
		double gravityX;
		double gravityY;
		double gravityZ;
		double userAcclerationX;
		double userAcclerationY;
		double userAcclerationZ;
		double attitudeX;
		double attitudeY;
		double attitudeZ;

		cv::Vec3d cvGravity() const;
		cv::Vec3d cvUserAccleration() const;
		cv::Vec3d cvAttitude() const;
	};

	struct QuadFrame {
		double timestamp = .0;
		Camera camera;
		Imu imu;
		Gps gps;

		QuadFrame clone() const;
	};
}
