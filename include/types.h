#pragma once
#include "opencv2/opencv.hpp"
#include <stdint.h>

namespace qs {
	struct IMU {
		double timestamp = .0;
		double gravity[3] = { .0, .0, .0 };
		double userAccleration[3] = { .0, .0, .0 };
		double attitude[3] = { .0, .0, .0 };

		cv::Vec3d cvGravity();
		cv::Vec3d cvUserAccleration();
		cv::Vec3d cvAttitude();
	};

	struct GPS {
		double timestamp = .0;
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

	struct Camera {
		double timestamp = .0;
		cv::Mat color;
		cv::Mat depth;
		cv::Mat confidence;
		AR ar;

		Camera();
		virtual ~Camera();
		Camera(const Camera& frame);
		Camera(Camera&& frame);
		Camera& operator=(const Camera& frame);
		Camera& operator=(Camera&& frame);
	};

	struct QuadFrame {
		double timestamp = .0;
		Camera camera;
		IMU imu;
		GPS gps;
	};
}
