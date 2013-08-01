#ifndef _ONI_EXTRACTOR_HPP_
#define _ONI_EXTRACTOR_HPP_

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>

#include <XnCppWrapper.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define RETURN_ON_ERROR(status, errmsg) {if (status != XN_STATUS_OK) {std::cerr<<errmsg<<std::endl; return false;}}

typedef unsigned long milliseconds;

class OniExtractor
{
	public:
		OniExtractor(const std::string& filename = "", const bool verbose = false);

		// Initialize everything what is needed to extract images from the given .oni file.
		bool init();

		// Stop generating and release the context. Always use this to clean up after
		// you are finished.
		bool stop();

		// Extract the specified frames (specified through, e.g., the given timestamps)
		// and save them in 'rgb' and 'depth', the corresponding timestamps in 'timestamps'.
		bool extract();

		// You can set timestamps of interest, so that only the frames within the time margin (+/- timeMargin)
		// to the given timestamps are returned. 
		void setTimestampsOfInterest(const std::vector<milliseconds>& tsoi, const milliseconds& timeMargin = 0);
		void setTimestampsOfInterest(const std::vector<std::string>& tsoi, const milliseconds& timeMargin = 0);
		// Also you can specify whether the given timestamps are relative timestamps
		// (i.e., beginning the beginning of the recording has timestamp 0, this is the default)
		// or else absolute timestamps (common Unix timestamps starting from 1.1.1970).
		// In case you want to set absolute timestamps of interest, you have to specify the reference time,
		// i.e., the beginning of the recording or creation time of the .oni file, since the
		// timestamps extracted from the .oni are all relative.
		void setAbsoluteTimestampReference(const milliseconds& _referenceTime);

		// Specify the .oni file.
		void setFile(const std::string& fn) {filename = fn;};

		// Specify whether the blue and red channels shall be switched for the extracted RGB images
		// (e.g., for post-processing in OpenCV).
		void convertRGB2BGR(const bool yn) {RGB2BGR = yn;};

		// Get data.
		void getRGB(std::vector<cv::Mat>& _rgb) {_rgb = rgb;};
		void getDepth(std::vector<cv::Mat>& _depth);
		void getTimestamps(std::vector<milliseconds>& _ts) {_ts = timestamps;};
		void getRealWorldCoords(std::vector<cv::Mat>& _rw) {_rw = realWorldCoords;};
		inline unsigned int getNumFrames() {return numFrames;};
		inline void setVerbose(const bool v) {verbose=v;};

	private:
		std::string filename;

		xn::Context xContext;
		xn::Player xPlayer;
		xn::ImageGenerator xImageGenerator;
		xn::DepthGenerator xDepthGenerator;
		XnChar instanceName;

		unsigned int numFrames;
		bool initialized;
		bool RGB2BGR;
		bool append;
		bool absoluteTS;
		bool verbose;
		milliseconds referenceTime;

		std::vector<cv::Mat> rgb;
		std::vector<cv::Mat> realWorldCoords;
		std::vector<milliseconds> timestamps;
		std::vector<milliseconds> tsOfInterest;
		milliseconds timeMargin;

		void storeCurrentRGBImage();
		void storeCurrentRealWorldCoords();
		void ximage2opencv(const xn::ImageMetaData& xImageMap, cv::Mat& im);
		void xworld2opencv(const XnPoint3D* worldCoords, const cv::Size& resolution, cv::Mat& im);
		inline milliseconds string2timestamp(const std::string& s) {return strtoul(s.c_str(),NULL,0);};
		void sortTimestamps() {std::sort(tsOfInterest.begin(), tsOfInterest.end());};
};

#endif