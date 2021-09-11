#pragma once
#include <optional>
#include <filesystem>
#include "types.h"
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "qs_zlib/zlib.h"

namespace qs {
	struct QuadLoader {
		QuadLoader();
		virtual ~QuadLoader();
		void open(const std::filesystem::path& recDir);
		void close();
		bool isOpened() const;
		std::optional<QuadFrame> next(bool withImu = true, bool withGps = true);
		void seek(const uint64_t frameNumber);
		const std::unique_ptr<QSStorage>& getStorage() const;

	private:
		Description description;
		cv::VideoCapture video;
		std::unique_ptr<QSStorage> storagePtr;

		double preTimestamp;
	};
}
