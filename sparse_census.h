#include <algorithm>
#include <limits>
#include <vector>

#ifdef _WIN32
#define popcount __popcnt
#else
#define popcount __builtin_popcount
#endif

// Census Radius and Width
#define C_R (3)
#define C_W (2 * C_R + 1)
#define DS (1)
// sampling pattern
// . X . X . X .
// X . X . X . X
// . X . X . X .
// X . X 0 X . X
// . X . X . X .
// X . X . X . X
// . X . X . X .

// y,x
const int samples[] = {
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

static void censusTransform(uint8_t* in, uint32_t* out, int w, int h)
{
    int ns = (int)(sizeof(samples) / sizeof(int)) / 2;
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

static float subpixel(float costLeft, float costMiddle, float costRight)
{
    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}

// left and right images of sized width neight
// locs are n pairs of (y,x) values
//
// returns a vector of (y,x) matches
// y,x < 0 if no valid match found
#define BOX_RADIUS (3)
#define MAX_DISP (32) 

std::vector<float> match(uint8_t * left, uint8_t * right, int32_t width, int32_t height, std::vector<float> & pts1)
{
    std::vector<float>    pts2(pts1.size(),-1);
    std::vector<int32_t>  censusLeft(width * height, 0);
    std::vector<int32_t>  censusRight(width * height, 0);

    for(int i=0; i < pts1.size(); i++){
        pts2[i] = pts1[i];
        // let's do this for 
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
