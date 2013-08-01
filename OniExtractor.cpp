#include "OniExtractor.hpp"

#ifdef __STANDALONE_ONI_EXTRACTOR
	#include <opencv2/highgui/highgui.hpp>
#endif

OniExtractor::OniExtractor(const std::string& _filename, const bool _verbose)
: filename(_filename),
  xContext(),
  xPlayer(),
  xImageGenerator(),
  xDepthGenerator(),
  instanceName(),
  numFrames(0),
  initialized(false),
  RGB2BGR(false),
  append(false),
  absoluteTS(false),
  verbose(_verbose),
  referenceTime(0),
  rgb(),
  realWorldCoords(),
  timestamps(),
  tsOfInterest(),
  timeMargin(0)
{

}

bool OniExtractor::init()
{
	if (filename.empty())
	{
		std::cerr << "No file specified!" << std::endl;
		return false;
	}
	RETURN_ON_ERROR(xContext.Init(), "Unable to initialize Context!");
	RETURN_ON_ERROR(xContext.OpenFileRecording(filename.c_str(), xPlayer), "Unable to open file "<<filename<<"!");
	if (verbose) std::cout << "Opened " << filename << std::endl;
	xPlayer.SetRepeat(false);
	RETURN_ON_ERROR(xDepthGenerator.Create(xContext), "Unable to create DepthGenerator!");
	RETURN_ON_ERROR(xImageGenerator.Create(xContext),  "Unable to create ImageGenerator!");
	xImageGenerator.SetPixelFormat(XN_PIXEL_FORMAT_RGB24);
	RETURN_ON_ERROR(xPlayer.GetNumFrames(xImageGenerator.GetName(), numFrames), "Unable to get number of frames!");
	if (numFrames == 0)
	{
		std::cerr << "No frames found!" << std::endl;
		return false;
	}
	if (verbose) std::cout << "Number of frames: " << numFrames << std::endl;
	RETURN_ON_ERROR(xContext.StartGeneratingAll(), "Unable to start generating images!");
	const xn::NodeInfo info = xImageGenerator.GetInfo();
	instanceName = *info.GetInstanceName();
	initialized = true;
	return true;
}

bool OniExtractor::stop()
{
	xContext.StopGeneratingAll();
	xContext.Release();
	initialized = false;
	return true;
}

bool OniExtractor::extract()
{
	if (!initialized)
	{
		std::cerr << "OniExtractor not initialized! You have to call init() first." << std::endl;
		return false;
	}

	if (!append)
	{
		rgb.clear();
		realWorldCoords.clear();
		timestamps.clear();
	}

	if (verbose)
	{
	    std::cout << "Timestamps of interest: ";
	    for (auto ts : tsOfInterest)
	    	std::cout << ts << " ";
	    std::cout << " (within +/-" << timeMargin << "ms)" << std::endl;
	}

	bool showWarning = true;
	XnUInt64 timestamp;
	for (unsigned long int i = 0; i < numFrames; ++i)
    {
        xImageGenerator.WaitAndUpdateData();
        xDepthGenerator.WaitAndUpdateData();
        xPlayer.TellTimestamp(timestamp);
        milliseconds timeOffset = (milliseconds)timestamp/1000;

        if (tsOfInterest.size() > 0)
        {
        	if (showWarning)
        	{
        		if (absoluteTS && referenceTime == 0)
        			std::cout << "WARNING! Seems like you are using absolute timestamps but did not set a reference time!" << std::endl;
        		if (absoluteTS && tsOfInterest.at(0) < 946684800000)
        			std::cout << "WARNING! Seems like you've specified a reference time, but use relative timestamps!" << std::endl;
        		showWarning = false;
        	}
        	// Use given timestamps to find frames.
	        if (absoluteTS) timeOffset += referenceTime;
	        if (timeOffset > tsOfInterest.back()) break;

	       	for (auto ts : tsOfInterest)
	        {
	        	if (abs(timeOffset - ts) < timeMargin)
	        	{
	        		if (verbose) std::cout << "Found frame (" << i << "/" << numFrames << "), timestamp = " << timeOffset << std::endl;
			        storeCurrentRGBImage();
			        storeCurrentRealWorldCoords();
			        timestamps.push_back(timeOffset);
			        break; // continute with the next frame
		    	}
		    }
		}
		else
		{
			// Else store every frame.
			if (verbose) std::cout << "Frame " << i << "/" << numFrames << ", timestamp = " << timeOffset << std::endl;
			storeCurrentRGBImage();
	        storeCurrentRealWorldCoords();
	        timestamps.push_back((milliseconds)timestamp);
		}
    }

	return true;
}

void OniExtractor::storeCurrentRGBImage()
{
	xn::ImageMetaData xImageMap;
	xImageGenerator.GetMetaData(xImageMap);
	cv::Mat imgRGB;
    ximage2opencv(xImageMap, imgRGB);
    rgb.push_back(imgRGB);
}

void OniExtractor::storeCurrentRealWorldCoords()
{
	xn::DepthMetaData xDepthMap;
	xDepthGenerator.GetMetaData(xDepthMap);
	int xRes = xDepthMap.FullXRes();
	int yRes = xDepthMap.FullYRes();
	const XnDepthPixel* pDepth = xDepthMap.Data();
	int numPx = xRes * yRes;
	XnPoint3D* ptProj = new XnPoint3D[numPx];
	XnPoint3D* ptWorld = new XnPoint3D[numPx];

	for (int y = 0; y < yRes; ++y)
	{
		for (int x = 0; x < xRes; ++x)
		{
			int index = x + y * xRes;
			ptProj[index].X = (XnFloat)x;
			ptProj[index].Y = (XnFloat)y;
			ptProj[index].Z = pDepth[index];
		}
	}
	xDepthGenerator.ConvertProjectiveToRealWorld((XnUInt32)numPx, ptProj, ptWorld);
	cv::Mat imgRW;
	xworld2opencv(ptWorld, cv::Size(xRes,yRes), imgRW);
	realWorldCoords.push_back(imgRW);
}

void OniExtractor::ximage2opencv(const xn::ImageMetaData& xImageMap, cv::Mat& im)
{
    int h = xImageMap.YRes();
    int w = xImageMap.XRes();
    const XnRGB24Pixel* xx = xImageMap.RGB24Data();
    const cv::Mat tmp(h, w, CV_8UC3, (void*)xx);
    if (RGB2BGR) cv::cvtColor(tmp, tmp, CV_BGR2RGB);
    tmp.copyTo(im);
}

// void OniExtractor::xdepth2opencv(const xn::DepthMetaData& xDepthMap, cv::Mat& im)
// {
//     int h = xDepthMap.YRes();
//     int w = xDepthMap.XRes();
//     const cv::Mat tmp(h, w, CV_16U, (void*)xDepthMap.Data());
//     tmp.copyTo(im);
// }

void OniExtractor::xworld2opencv(const XnPoint3D* rw, const cv::Size& res, cv::Mat& im)
{
	const cv::Mat tmp(res.height, res.width, CV_32FC3, (void*)rw);
	tmp.copyTo(im);
}

void OniExtractor::getDepth(std::vector<cv::Mat>& _depth)
{
	_depth.clear();
	for (cv::Mat d : realWorldCoords)
	{
		std::vector<cv::Mat> xyd;
		cv::split(d, xyd);
		cv::Mat depth = xyd.at(2);
		depth.convertTo(depth, CV_16U);		
		_depth.push_back(depth);
	}
}

void OniExtractor::setTimestampsOfInterest(const std::vector<milliseconds>& _tsoi, const milliseconds& _timeMargin)
{
	tsOfInterest = _tsoi;
	timeMargin = _timeMargin;
	sortTimestamps();
	if (tsOfInterest.at(0) > 946684800000) // year 2000 in milliseconds
		absoluteTS = true;
}

void OniExtractor::setTimestampsOfInterest(const std::vector<std::string>& _tsoi, const milliseconds& _timeMargin)
{
	std::vector<milliseconds> tsoi;
	for (auto s : _tsoi)
		tsoi.push_back(string2timestamp(s));
	setTimestampsOfInterest(tsoi, _timeMargin);
}

void OniExtractor::setAbsoluteTimestampReference(const milliseconds& _referenceTime)
{
	absoluteTS = true;
	referenceTime = _referenceTime;
	if (verbose) std::cout << "Reference time set. I'm expecting absolute timestamps now." << std::endl;
}

//
// Example
//

#ifdef __STANDALONE_ONI_EXTRACTOR
int main(int argc, char** argv)
{
    if( argc == 1 )
    {
        std::cout << "Please give an ONI file to open" << std::endl;
        return 1;
    }

    std::vector<cv::Mat> img, depth;
    std::vector<milliseconds> ts;
    std::string fn(argv[1]);

    OniExtractor oe(fn, true);
    if (oe.init())
    {
        oe.convertRGB2BGR(true);
        oe.setTimestampsOfInterest({1000, 2000, 3000, 4000}, 100);

        oe.extract();
        oe.stop();
        
        oe.getRGB(img);
        oe.getDepth(depth);
        oe.getTimestamps(ts);
    }

    cv::namedWindow( "RGB", CV_WINDOW_AUTOSIZE);
    cv::namedWindow( "Depth", CV_WINDOW_AUTOSIZE);

    for (int i = 1; i < img.size(); ++i)
    {
        std::string msg = std::to_string(ts.at(i)) + ", " + std::to_string(i+1) + "/" + std::to_string(img.size());
        cv::putText(img.at(i), msg, cv::Point(20,20), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 0.8, cv::Scalar::all(0), 2, 8);
        cv::imshow("RGB", img.at(i));
        cv::imshow("Depth", depth.at(i));
        cv::waitKey(0);
    }

    return 0; 
}
#endif