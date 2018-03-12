#include <algorithm>
#include <limits>
#include <vector>

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
#define DS (0)
// Numer of pixels
#define NUM_SAMPLES (24)
// matching box size
#define BOX_RADIUS (3)
// matching search range
#define MAX_DISP (32) 
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

static void censusTransform(uint8_t* in, uint32_t* out, int width, int height)
                           // int w,int e, int n, int s)
{
    for (int y = C_R; y < height - C_R; y++) {
        for (int x = C_R; x < width - C_R; x++) {
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
    std::vector<uint32_t>  censusLeft(width * height, 0);
    std::vector<uint32_t>  censusRight(width * height, 0);

    // fill out census windows
    censusTransform(left,censusLeft.data(),width,height);
    censusTransform(right,censusRight.data(),width,height);

    for(int i=0; i < pts1.size(); i+=2){
        auto x = pts2[i] = pts1[i];
        auto y = pts2[i+1] = pts1[i+1];

        // skip borders
        if(x < C_R || x >= width-C_R
            || y < C_R || y >= width-C_R) {
                pts2[i] = -1;
                pts2[i+1] = -1;
                continue;
        }
    }
    for (int y = BOX_RADIUS; y < height - BOX_RADIUS; y++) {
        for (int x = BOX_RADIUS; x < width - BOX_RADIUS - DS; x++) {
            auto lb = std::max(BOX_RADIUS, x - MAX_DISP);
            auto search_limit = x - lb;
            for (int d = 0; d < search_limit; d++) {
                uint16_t cost = 0;
                for (int i = -BOX_RADIUS; i <= BOX_RADIUS; i++) {
                    for (int j = -BOX_RADIUS; j <= BOX_RADIUS; j++) {
                        auto pl = censusLeft[(y + i) * width + (x + j)];
                        auto pr = censusRight[(y + i) * width + (x + j - (d - DS))];

                        cost += popcount(pl ^ pr);
                    }
                }
                //costX[x * maxdisp + d] = cost;
            }
        }
    }

    return pts2;
}
