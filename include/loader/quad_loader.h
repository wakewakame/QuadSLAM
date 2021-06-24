#pragma once
#include "camera_loader.h"
#include "imu_loader.h"
#include "gps_loader.h"
#include "types.h"

namespace qs {
	struct QuadLoader {
		QuadLoader();
		virtual ~QuadLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened() const;
		std::optional<QuadFrame> next();

	private:
		bool isOpened_ = false;
		CameraLoader cameraLoader;
		IMULoader imuLoader;
		GPSLoader gpsLoader;
	};
}
