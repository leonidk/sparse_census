#include "sparse_census.h"

#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

int main(int argc,char* argv[])
{

    // get data
    cv::Mat left = cv::imread("im2.png");
    cv::Mat right =cv::imread("im6.png");
    cv::Mat l,r,outimg;

    //grey
    cvtColor(left,l,cv::COLOR_RGB2GRAY);
    cvtColor(right,r,cv::COLOR_RGB2GRAY);

    // blow out the right image
    for(int i=0; i < l.cols*l.rows;i++) {
        r.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(r.at<uint8_t>(i)*3/2);
    }

    // get image features
    std::vector<cv::KeyPoint> locs1, locs2;
    std::vector<float> pts1;
    std::vector<cv::DMatch> matches;
    FAST(l,locs1,80,true);

    // get float arrays
    for(const auto & l : locs1){
        pts1.push_back(l.pt.x);
        pts1.push_back(l.pt.y);
    }
    
    // perform basic matching
    auto pts2 = match(l.data,r.data,l.cols,l.rows,pts1);


    /// go back to opencv friendly
    for(auto i=0; i < pts2.size(); i+=2){
        // 4 is random
        locs2.push_back(cv::KeyPoint(pts2[i],pts2[i+1],4));
    }

    for(int i=0; i < locs1.size();i++){
        matches.push_back({i,i,0});
    }
    drawMatches(l,locs1,r,locs2,matches,outimg);

    cv::imshow("window",outimg);
    cv::waitKey(0);
    return 0;
}