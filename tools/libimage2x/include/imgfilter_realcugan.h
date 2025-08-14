#pragma once

#include "imgprocessor.h"
#include "realcugan.h"

namespace image2x {

// FilterRealcugan class definition
class IMGFilterRealcugan : public IMGFilter {
   public:
    // Constructor
    IMGFilterRealcugan(
        const StringType model_dir = STR("."),
        int gpuid = -1,
        int scaling_factor = 4,
        const StringType model_name = STR("models-pro"),
        int noise_level = -1,
        bool tta_mode = false,
        int num_threads = 1,
        int syncgap = 3
    );

    // Destructor
    virtual ~IMGFilterRealcugan() override;

    int init() override;

    // Processes an input frame and returns the processed frame
    BGR24Data filter(BGR24Data avdata) override;

   private:
    RealCUGAN* realcugan_;
    int gpuid_;
    bool tta_mode_;
    int scaling_factor_;
    int noise_level_;
    int num_threads_;
    int syncgap_;
    const StringType model_name_;
    const StringType model_dir_;
};

}
