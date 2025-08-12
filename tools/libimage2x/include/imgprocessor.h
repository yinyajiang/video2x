#pragma once

#include <variant>
#include <vector>

#include "fileutils.h"
#include "libimage2x.h"

namespace image2x {

struct RealESRGANConfig {
    bool tta_mode = false;
    StringType model_name;
};

struct RealCUGANConfig {
    bool tta_mode = false;
    int num_threads = 1;
    int syncgap = 3;
    StringType model_name;
};


// Unified filter configuration
struct IMGProcessorConfig {
    int width = 0;
    int height = 0;
    int scaling_factor = 0;
    int noise_level = -1;
    int frm_rate_mul = 0;
    float scn_det_thresh = 0.0f;
    std::variant<RealESRGANConfig, RealCUGANConfig> config;
};


// Abstract base class for filters
class IMGFilter{
   public:
    virtual ~IMGFilter() = default;
    virtual int init() = 0;
    virtual BGR24Data filter(BGR24Data avdata) = 0;
};


}
