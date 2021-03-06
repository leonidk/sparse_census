#include "sparse_census.h"

#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
#include <iostream>
#include <fstream>
std::vector<float> readBinaryFile(const char * infile)
{
    FILE *fp = fopen (infile, "rb");
    std::vector<float> ret;
    float tmp;
    while (fread (&tmp, 4, 1, fp) == 1)
        ret.push_back(tmp);
    return ret;
 }

int main(int argc,char* argv[])
{
    // get data
    cv::Mat left = cv::imread("left.png");
    cv::Mat right =cv::imread("right.png");
    cv::Mat l,r,outimg;

    //grey
    cvtColor(left,l,cv::COLOR_RGB2GRAY);
    cvtColor(right,r,cv::COLOR_RGB2GRAY);

    // blow out the right image
    for(int i=0; i < l.cols*l.rows;i++) {
        //r.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(r.at<uint8_t>(i)*1.5);
    }

    // get image features
    std::vector<cv::KeyPoint> locs1, locs2;
    std::vector<float> pts1;
    std::vector<cv::DMatch> matches;
    FAST(l,locs1,50,true);

    // get float arrays
    for(const auto & l : locs1){
        pts1.push_back(l.pt.x);
        pts1.push_back(l.pt.y);
    }
    if(true){
        pts1.clear();
        const char* tmp = std::string("points.dat").c_str();
        pts1 = readBinaryFile(tmp);
        if(false){
            pts1.clear();
            for(auto y=0; y < l.rows; y++) {
                for(auto x=0; x < l.cols; x++) {
                    pts1.push_back(x);
                    pts1.push_back(y);
                }
            }
        }
        locs1.clear();
        for(auto i=0; i < pts1.size(); i+=2){
            locs1.push_back(cv::KeyPoint(pts1[i],pts1[i+1],4));
        }

    }
    // perform basic matching
    auto begin = std::chrono::high_resolution_clock::now();
    auto pts2 = match(l.data,r.data,l.cols,l.rows,pts1);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() << " us" << std::endl;
    /// go back to opencv friendly
    for(auto i=0; i < pts2.size(); i+=2){
        // 4 is random
        locs2.push_back(cv::KeyPoint(pts2[i],pts2[i+1],4));
    }
    for(int i=0; i < locs1.size();i++){
        if(pts2[2*i] >= 0)
            matches.push_back({i,i,0});
    }
    printf("%lu %lu\n",locs1.size(),matches.size());
    drawMatches(l,locs1,r,locs2,matches,outimg);

    cv::imshow("window",outimg);
    cv::waitKey(0);
    return 0;
}