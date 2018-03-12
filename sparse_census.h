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
#define BOX_RADIUS (5)
// matching search range
#define MAX_DISP (96) 
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

static void censusTransformOG(uint8_t* in, uint32_t* out, int w, int h)
{
    int ns = NUM_SAMPLES;//(int)(sizeof(samples) / sizeof(int)) / 2;
    for (int y = C_R; y < h - C_R; y++) {
        for (int x = C_R; x < w - C_R; x++) {
            uint32_t px = 0;
            auto center = in[y * w + x];
            for (int p = 0; p < ns; p++) {
                auto yp = (y + samples[2 * p]);
                auto xp = (x + samples[2 * p + 1]);
                px |= (in[yp * w + xp] > center) << p;
            }
            out[y * w + x] = px;
        }
    }
}
static void censusTransform(uint8_t* in, uint32_t* out, int width, int height,
                         int u,int v,int w,int e, int n, int s)
{
    for (int y = std::max(v-n,C_R); y <= std::min(v+s,height - C_R-1); y++) {
        for (int x = std::max(u-w,C_R); x <= std::min(u+e,width - C_R-1); x++) {
            // my benchmark suggests this sparse skip is helpful
            //if( out[y * width + x] != 0) continue;
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
    std::vector<int>    pts1_int(pts1.size(),-1);

    std::vector<uint32_t>  censusLeft(width * height, 0);
    std::vector<uint32_t>  censusRight(width * height, 0);
    //censusTransformOG(left,censusLeft.data(),width,height);
    //censusTransformOG(right,censusRight.data(),width,height);

    // fill out census windows, sparsely
    for(int i=0; i < pts1.size(); i+=2){
        int x = std::round(pts1[i]);
        int y = std::round(pts1[i+1]);


        // skip borders
        if(x < C_R || x >= width-C_R
            || y < C_R || y >= width-C_R) {
                continue;
        }
        pts1_int[i] = x;
        pts1_int[i+1] = y;
        censusTransform(left,censusLeft.data(),width,height,x,y,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS);
        censusTransform(right,censusRight.data(),width,height,x,y,MAX_DISP+1+BOX_RADIUS,BOX_RADIUS,BOX_RADIUS,BOX_RADIUS);
    }

    // find matches
    std::vector<int> costs(MAX_DISP);
    for(int i=0; i < pts2.size(); i+=2){
        if(pts1_int[i] < 0) {pts2[i] = -1; continue;}
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
            //printf("%d %d %d\n",i,d,cost);
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
        //printf("%d\t%d\t%f\n",i,minLIdx,spL);
        pts2[i+1] = pts1[i+1];
        pts2[i]   = pts1[i] - minLIdx-DS+spL;
        if(minLIdx==0){
            pts2[i] = -1;
            pts2[i+1] = -1;
        }
    }

    return pts2;
}
