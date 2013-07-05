OniExtractor
============

Extract images and real world coordinates from OpenNI oni-files to OpenCV Matrices.

Dependencies
------------
* [OpenNI 1.5.x](https://github.com/OpenNI/OpenNI)
* [OpenCV](http://opencv.org/)

Usage
-----
The OniExtractor class can be used to write frames extracted from a given .oni file to `std::vector<cv::Mat>`.

        OniExtractor oe(filename);
        ok = oe.init();


You can extract the RGB images (`CV_8UC3`), depth images (`CV_16U`),
real world coordinates (`CV_32FU3`) and timestamps (in milliseconds).

        std::vector<cv::Mat> img, depth, world;
        std::vector<unsigned long> ts;
        oe.extract();
        oe.getRGB(img);
        oe.getDepth(depth);
        oe.getWorldCoords(world);
        oe.getTimestamps(ts);


Additionally you can specify *timestamps of interest* and a margin to extract only frames at the specified
timestamps within the margin. Each time value is given in milliseconds.

        std::vector<unsigned long> tsoi = {1000, 2000, 3000};
        margin = 100;
        oe.setTimestampsOfInterest(tsoi, margin);
        
        
If your timestamps of interest are absolute (i.e., Unix) timestamps, you can specify a reference
time, e.g., the creation time of the .oni file.

        unsigned long ref = 946684800000; // year 2000
        oe.setAbsoluteTimestampReference(ref);
        oe.setTimestampsOfInterest({946684801000, 946684802000, 946684803000}, 100);
        
Acknowledgment
--------------
... goes to Pieter Eendebak for [this](https://groups.google.com/forum/#!topic/openni-dev/GqeX5s_nVcY) post,
[Dirk-Jan Kroon](http://www.mathworks.com/matlabcentral/fileexchange/authors/29180) and M. Lang.
