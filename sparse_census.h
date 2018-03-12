#include <algorithm>
#include <limits>
#include <vector>
#include <cmath>

#ifdef _WIN32
#define popcount __popcnt
#else
#define popcount __builtin_popcount
#endif

// Census Radius
#define C_R (3)
// Census Width
#define C_W (2 * C_R + 1)
// Disparity Shift
// 1 allows subpixel at infinity
#define DS (1)
// Numer of pixels
#define NUM_SAMPLES (24)
// matching box size
#ifndef BOX_RADIUS
#define BOX_RADIUS (4)
#endif
// matching search range
#ifndef MAX_DISP
#define MAX_DISP (128)
#endif
// maximum number of bits wrong per pixel
// scaled to box size
#ifndef MAX_COST
#define MAX_COST (9*(2*BOX_RADIUS+1)*(2*BOX_RADIUS+1))
#endif
// minimum number of bits wrong
#ifndef MIN_COST
#define MIN_COST (10)
#endif
// the second best has to be AT LEAST this much 
// set to zero to disable
#ifndef SECOND_PEAK
#define SECOND_PEAK (50)
#endif

// y,x
const int samples[NUM_SAMPLES*2] = {
    -3, -2,
    -3, 0,
    -3, 2,
    -2, -3,
    -2, -1,
    -2, 1,
    -2, 3,
    -1, -2,
    -1, 0,
    -1, 2,
    0, -3,
    0, -1,
    0, 1,
    0, 3,
    1, -2,
    1, 0,
    1, 2,
    2, -3,
    2, -1,
    2, 1,
    2, 3,
    3, -2,
    3, 0,
    3, 2
};

static void censusTransform(uint8_t* in, uint32_t* out, int width, int height,
                         int u,int v,int w,int e, int n, int s)
{
    for (int y = std::max(v-n,C_R); y <= std::min(v+s,height - C_R-1); y++) {
        for (int x = std::max(u-w,C_R); x <= std::min(u+e,width - C_R-1); x++) {
            // my benchmark suggests this sparse skip is helpful
            if( out[y * width + x] != 0) continue;
            uint32_t px = 0;
            auto center = in[y * width + x];
            for (int p = 0; p < NUM_SAMPLES; p++) {
                auto yp = (y + samples[2 * p]);
                auto xp = (x + samples[2 * p + 1]);
                px |= (in[yp * width + xp] > center) << p;
            }
            out[y * width + x] = px;
        }
    }
}

static float subpixel(float costLeft, float costMiddle, float costRight)
{
    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}

// left and right images of sized width neight
// locs are n pairs of (x,y) values
//
// returns a vector of (x,y) matches
// y,x < 0 if no valid match found
std::vector<float> match(uint8_t * left, uint8_t * right, int32_t width, int32_t height, std::vector<float> & pts1)
{
    std::vector<float>    pts2(pts1.size(),-1);
    std::vector<int>      pts1_int(pts1.size(),-1);

    std::vector<uint32_t>  censusLeft(width * height, 0);
    std::vector<uint32_t>  censusRight(width * height, 0);

    // fill out census windows, sparsely
    for(int i=0; i < pts1.size(); i+=2){
        int x = std::round(pts1[i]);
        int y = std::round(pts1[i+1]);

        // skip borders
        if(x < (C_R+BOX_RADIUS) || x >= width-(C_R+BOX_RADIUS)
            || y < (C_R+BOX_RADIUS) || y >= width-(C_R+BOX_RADIUS)) {
                continue;
        } else {}
        pts1_int[i] = x;
        pts1_int[i+1] = y;
        censusTransform(left,censusLeft.data(),width,height,x,y,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS);
        censusTransform(right,censusRight.data(),width,height,x,y,MAX_DISP+1+BOX_RADIUS,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS);
    }
    // find matches
    std::vector<int> costs(MAX_DISP);
    for(int i=0; i < pts2.size(); i+=2){
        if(pts1_int[i] < 0) {pts2[i] = -1;pts2[i+1] = -1; continue;}
        auto x = pts1_int[i];
        auto y = pts1_int[i+1];
        std::fill(costs.begin(), costs.end(),0xffff);

        auto lb = std::max(BOX_RADIUS, x - MAX_DISP);
        auto search_limit = x - lb;
        for (int d = 0; d < search_limit; d++) {
            int cost = 0;
            for (int i = -BOX_RADIUS; i <= BOX_RADIUS; i++) {
                for (int j = -BOX_RADIUS; j <= BOX_RADIUS; j++) {
                    auto pl = censusLeft[(y + i) * width + (x + j)];
                    auto pr = censusRight[(y + i) * width + (x + j - (d - DS))];

                    cost += popcount(pl ^ pr);
                }
            }
            costs[d] = cost;
        }
  
        auto minLVal = std::numeric_limits<uint16_t>::max();
        auto minLIdx = 0;
        for (int d = 0; d < MAX_DISP; d++) {
            auto cost = costs[d];
            if (cost < minLVal) {
                minLVal = cost;
                minLIdx = d;
            }
        } 
        auto nL = costs[std::max(minLIdx - 1, 0)];
        auto nC = costs[minLIdx];
        auto nR = costs[std::min(minLIdx + 1, MAX_DISP - 1)];
        auto spL =  subpixel(nL, nC, nR);
        pts2[i+1] = pts1[i+1];
        pts2[i]   = pts1[i] - minLIdx-DS+spL;

        auto valid = true;
        #if SECOND_PEAK != 0
            auto minL2Val = std::numeric_limits<uint16_t>::max();
            for (int d = 0; d < MAX_DISP; d++) {
                auto cost = costs[d];
                auto costNext = (d == MAX_DISP - 1) ? cost : costs[d+1];
                auto costPrev = (d == 0) ? cost : costs[d - 1];

                if (cost <= costNext && cost <= costPrev) {
                    if (d == minLIdx)
                        continue;
                    if (cost < minL2Val)
                        minL2Val = cost;
                }
            }
            auto diffSP = minL2Val - minLVal;
            if(diffSP <= SECOND_PEAK) {
                valid = false;

            }
        #endif
        if(nC < MIN_COST) {
            valid = false;
        }
        if(nC > MAX_COST) {
            valid = false;
        }
        // it's clear to me that..
        // this is probably useless
        // for sparse matches based on corner features 
        //auto diffL = (int)nL - (int)nC;
        //auto diffR = (int)nR - (int)nC;
        //if(diffL >= NEIGHBOR_COST || diffR >= NEIGHBOR_COST) {
        //    valid = false;
        //}

        // if data is not valid
        // or if it matched to self
        if(minLIdx==0 || !valid){
            pts2[i] = -1;
            pts2[i+1] = -1;
        } else {  }
    }

    return pts2;
}
