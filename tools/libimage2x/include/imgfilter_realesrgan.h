#pragma once

#include "imgprocessor.h"
#include "realesrgan.h"

namespace image2x {

// FilterRealesrgan class definition
class IMGFilterRealesrgan : public IMGFilter {
   public:
    // Constructor
    IMGFilterRealesrgan(
        const StringType model_dir,
        int gpuid = -1,
        int scaling_factor = 4,
        const StringType model_name = STR("realesr-animevideov3"),
        int noise_level = 0,
        bool tta_mode = false
    );

    // Destructor
    virtual ~IMGFilterRealesrgan() override;

    // Initializes the filter with decoder and encoder contexts
    int init() override;

    // Processes an input frame and returns the processed frame
    BGR24Data filter(BGR24Data avdata) override;

   private:
    RealESRGAN* realesrgan_;
    int gpuid_;
    bool tta_mode_;
    int scaling_factor_;
    int noise_level_;
    const StringType model_name_;
    const StringType model_dir;
};

}
