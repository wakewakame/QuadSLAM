#pragma once
#include "opencv2/opencv.hpp"
#include <stdint.h>

namespace qs {
	struct IMU {
		double timestampe = 0.0;
		double gravity[3] = { .0, .0, .0 };
		double userAccleration[3] = { .0, .0, .0 };
		double attitude[3] = { .0, .0, .0 };

		cv::Vec3d cvGravity();
		cv::Vec3d cvUserAccleration();
		cv::Vec3d cvAttitude();
	};

	struct GPS {
		double timestampe = .0;
		double latitude = .0, longitude = .0, altitude = .0;
		double horizontalAccuracy = .0, verticalAccuracy = .0;

		cv::Vec3d cvGPS();
	};

	struct AR {
		float intrinsics[3][3]       = { { .0f, .0f, .0f }, { .0f, .0f, .0f }, { .0f, .0f, .0f } };
		float projectionMatrix[4][4] = { { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f } };
		float viewMatrix[4][4]       = { { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f }, { .0f, .0f, .0f, .0f } };

		cv::Mat cvIntrinsics();
		cv::Mat cvProjectionMatrix();
		cv::Mat cvViewMatrix();
	};

	struct Frame {
		uint64_t frameNumber = 0;
		double timestamp = 0.0;
		cv::Mat camera;
		cv::Mat depth;
		cv::Mat confidence;
		IMU imu;
		GPS gps;
		AR ar;

		Frame();
		virtual ~Frame();
		Frame(const Frame& frame);
		Frame(Frame&& frame);
		Frame& operator=(const Frame& frame);
		Frame& operator=(Frame&& frame);
	};
}
