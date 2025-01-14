// Inspired by WiiPower's "video toy", but simpler
// Updated for libogc 2.0.0

#include "videopatch.h"

#include <string.h>
#include <types.h>

#define ARRAY_SIZE(a) (sizeof a / sizeof a[0])

// viYOrigin is calculated as (576 - 528)/2 in libogc 2.0.0 for the following render modes.
// But we need to use (574 - 528)/2 so that the render modes match the Revolution SDK.
static GXRModeObj TVPal528Prog_RVL = {
    6,             // viDisplayMode
    640,           // fbWidth
    528,           // efbHeight
    528,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVPal528ProgSoft_RVL = {
    6,             // viDisplayMode
    640,           // fbWidth
    528,           // efbHeight
    528,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        8,  // line n-1
        8,  // line n-1
        10, // line n
        12, // line n
        10, // line n
        8,  // line n+1
        8   // line n+1
    }
};

static GXRModeObj TVPal524ProgAa_RVL = {
    6,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    524,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    524,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        4,  // line n-1
        8,  // line n-1
        12, // line n
        16, // line n
        12, // line n
        8,  // line n+1
        4   // line n+1
    }
};

static GXRModeObj TVPal528Int_RVL = {
    4,             // viDisplayMode
    640,           // fbWidth
    528,           // efbHeight
    528,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_DF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVPal528IntDf_RVL = {
    4,             // viDisplayMode
    640,           // fbWidth
    528,           // efbHeight
    528,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_DF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        8,  // line n-1
        8,  // line n-1
        10, // line n
        12, // line n
        10, // line n
        8,  // line n+1
        8   // line n+1
    }
};

static GXRModeObj TVEurgb60Hz480Prog_RVL = {
    22,            // viDisplayMode
    640,           // fbWidth
    480,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVEurgb60Hz480ProgSoft_RVL = {
    22,            // viDisplayMode
    640,           // fbWidth
    480,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        8,  // line n-1
        8,  // line n-1
        10, // line n
        12, // line n
        10, // line n
        8,  // line n+1
        8   // line n+1
    }
};

static GXRModeObj TVEurgb60Hz480ProgAa_RVL = {
    22,            // viDisplayMode
    640,           // fbWidth
    242,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        4,  // line n-1
        8,  // line n-1
        12, // line n
        16, // line n
        12, // line n
        8,  // line n+1
        4   // line n+1
    }
};

static GXRModeObj TVPal524IntAa_RVL = {
    4,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    524,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2 //574 instead of 576
    640,           // viWidth
    524,           // viHeight
    VI_XFBMODE_DF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        4,  // line n-1
        8,  // line n-1
        12, // line n
        16, // line n
        12, // line n
        8,  // line n+1
        4   // line n+1
    }
};

static GXRModeObj TVPal264Int_RVL = {
    4,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    264,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_TRUE,       // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVPal264IntAa_RVL = {
    4,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    264,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    23,            // viYOrigin (574 - 528)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_TRUE,       // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVPal264Ds_RVL = {
    5,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    264,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    11,            // viYOrigin (572/2 - 528/2)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVPal264DsAa_RVL = {
    5,             // viDisplayMode
    640,           // fbWidth
    264,           // efbHeight
    264,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    11,            // viYOrigin (572/2 - 528/2)/2
    640,           // viWidth
    528,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVMpal240Int_RVL = {
    8,             // viDisplayMode
    640,           // fbWidth
    240,           // efbHeight
    240,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_TRUE,       // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};


static GXRModeObj TVMpal240IntAa_RVL = {
    8,             // viDisplayMode
    640,           // fbWidth
    240,           // efbHeight
    240,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_TRUE,       // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVMpal480Int_RVL = {
    8,             // viDisplayMode
    640,           // fbWidth
    480,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_DF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        0,  // line n-1
        0,  // line n-1
        21, // line n
        22, // line n
        21, // line n
        0,  // line n+1
        0   // line n+1
    }
};

static GXRModeObj TVMpal480ProgSoft_RVL = {
    10,             // viDisplayMode
    640,           // fbWidth
    480,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_FALSE,      // aa

    // sample points arranged in increasing Y order
    {
        {6, 6}, {6, 6}, {6, 6}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {6, 6}, {6, 6}, {6, 6}, // pix 1
        {6, 6}, {6, 6}, {6, 6}, // pix 2
        {6, 6}, {6, 6}, {6, 6}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        8,  // line n-1
        8,  // line n-1
        10, // line n
        12, // line n
        10, // line n
        8,  // line n+1
        8   // line n+1
    }
};

static GXRModeObj TVMpal480ProgAa_RVL = {
    10,             // viDisplayMode
    640,           // fbWidth
    242,           // efbHeight
    480,           // xfbHeight
    40,            // viXOrigin (720 - 640)/2
    0,             // viYOrigin (480 - 480)/2
    640,           // viWidth
    480,           // viHeight
    VI_XFBMODE_SF, // xFBmode
    GX_FALSE,      // field_rendering
    GX_TRUE,       // aa

    // sample points arranged in increasing Y order
    {
        {3, 2}, {9, 6}, {3, 10}, // pix 0, 3 sample points, 1/12 units, 4 bits each
        {3, 2}, {9, 6}, {3, 10}, // pix 1
        {9, 2}, {3, 6}, {9, 10}, // pix 2
        {9, 2}, {3, 6}, {9, 10}  // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
        4,  // line n-1
        8,  // line n-1
        12, // line n
        16, // line n
        12, // line n
        8,  // line n+1
        4   // line n+1
    }
};

static GXRModeObj *vmodes[] = {
    &TVNtsc240Ds,
    &TVNtsc240DsAa,
    &TVNtsc240Int,
    &TVNtsc240IntAa,
    &TVNtsc480Int,
    &TVNtsc480IntAa,
    &TVNtsc480IntDf,
    &TVNtsc480Prog,
    &TVNtsc480ProgSoft,
    &TVNtsc480ProgAa,
    &TVMpal240Int_RVL,
    &TVMpal240IntAa_RVL,
    &TVMpal240Ds,
    &TVMpal240DsAa,
    &TVMpal480Int_RVL,
    &TVMpal480IntAa,
    &TVMpal480IntDf,
    &TVMpal480Prog,
    &TVMpal480ProgSoft_RVL,
    &TVMpal480ProgAa_RVL,
    &TVPal264Ds_RVL,
    &TVPal264DsAa_RVL,
    &TVPal264Int_RVL,
    &TVPal264IntAa_RVL,
    &TVPal524IntAa_RVL,
    &TVPal524ProgAa_RVL,
    &TVPal528Int_RVL,
    &TVPal528IntDf_RVL,
    &TVPal528Prog_RVL,
    &TVPal528ProgSoft_RVL,
    &TVPal576IntDfScale,
    &TVEurgb60Hz240Ds,
    &TVEurgb60Hz240DsAa,
    &TVEurgb60Hz240Int,
    &TVEurgb60Hz240IntAa,
    &TVEurgb60Hz480Int,
    &TVEurgb60Hz480IntDf,
    &TVEurgb60Hz480IntAa,
    &TVEurgb60Hz480Prog_RVL,
    &TVEurgb60Hz480ProgSoft_RVL,
    &TVEurgb60Hz480ProgAa_RVL
};

static bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2)
{
    return memcmp(mode1, mode2, sizeof *mode1) == 0; // padding seems to always be 0
}

static void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2)
{
    memcpy(mode1, mode2, sizeof *mode1);
}

static GXRModeObj* PAL2NTSC[] = {
    &TVMpal240Int_RVL,           &TVNtsc240Int,
    &TVMpal240IntAa_RVL,         &TVNtsc240IntAa,
    &TVMpal240Ds,                &TVNtsc240Ds,
    &TVMpal240DsAa,              &TVNtsc240DsAa,
    &TVMpal480Int_RVL,           &TVNtsc480Int,
    &TVMpal480IntAa,             &TVNtsc480IntAa,
    &TVMpal480IntDf,             &TVNtsc480IntDf,
    &TVMpal480Prog,              &TVNtsc480Prog,
    &TVMpal480ProgSoft_RVL,      &TVNtsc480ProgSoft,
    &TVMpal480ProgAa_RVL,        &TVNtsc480ProgAa,
    &TVPal264Ds_RVL,             &TVNtsc240Ds,
    &TVPal264DsAa_RVL,           &TVNtsc240DsAa,
    &TVPal264Int_RVL,            &TVNtsc240Int,
    &TVPal264IntAa_RVL,          &TVNtsc240IntAa,
    &TVPal524IntAa_RVL,          &TVNtsc480IntAa,
    &TVPal524ProgAa_RVL,         &TVNtsc480ProgAa,
    &TVPal528Int_RVL,            &TVNtsc480Int,
    &TVPal528IntDf_RVL,          &TVNtsc480IntDf,
    &TVPal528Prog_RVL,           &TVNtsc480Prog,
    &TVPal528ProgSoft_RVL,       &TVNtsc480ProgSoft,
    &TVPal576IntDfScale,         &TVNtsc480IntDf,
    &TVEurgb60Hz240Ds,           &TVNtsc240Ds,
    &TVEurgb60Hz240DsAa,         &TVNtsc240DsAa,
    &TVEurgb60Hz240Int,          &TVNtsc240Int,
    &TVEurgb60Hz240IntAa,        &TVNtsc240IntAa,
    &TVEurgb60Hz480Int,          &TVNtsc480Int,
    &TVEurgb60Hz480IntDf,        &TVNtsc480IntDf,
    &TVEurgb60Hz480IntAa,        &TVNtsc480IntAa,
    &TVEurgb60Hz480Prog_RVL,     &TVNtsc480Prog,
    &TVEurgb60Hz480ProgSoft_RVL, &TVNtsc480ProgSoft,
    &TVEurgb60Hz480ProgAa_RVL,   &TVNtsc480ProgAa,
    0,0
};

static GXRModeObj* NTSC2PAL[] = {
    &TVNtsc240Ds,                &TVPal264Ds_RVL,
    &TVNtsc240DsAa,              &TVPal264DsAa_RVL,
    &TVNtsc240Int,               &TVPal264Int_RVL,
    &TVNtsc240IntAa,             &TVPal264IntAa_RVL,
    &TVNtsc480Int,               &TVPal528Int_RVL,
    &TVNtsc480IntAa,             &TVPal524IntAa_RVL,
    &TVNtsc480IntDf,             &TVPal528IntDf_RVL,
    &TVNtsc480Prog,              &TVPal528Prog_RVL,
    &TVNtsc480ProgSoft,          &TVPal528ProgSoft_RVL,
    &TVNtsc480ProgAa,            &TVPal524ProgAa_RVL,
    0,0
};

static GXRModeObj* NTSC2PAL60[] = {
    &TVNtsc240Ds,                &TVEurgb60Hz240Ds,
    &TVNtsc240DsAa,              &TVEurgb60Hz240DsAa,
    &TVNtsc240Int,               &TVEurgb60Hz240Int,
    &TVNtsc240IntAa,             &TVEurgb60Hz240IntAa,
    &TVNtsc480Int,               &TVEurgb60Hz480Int,
    &TVNtsc480IntAa,             &TVEurgb60Hz480IntAa,
    &TVNtsc480IntDf,             &TVEurgb60Hz480IntDf,
    &TVNtsc480Prog,              &TVEurgb60Hz480Prog_RVL,
    &TVNtsc480ProgSoft,          &TVEurgb60Hz480ProgSoft_RVL,
    &TVNtsc480ProgAa,            &TVEurgb60Hz480ProgAa_RVL,
    0,0
};

static void applyVideoPatch(void *Address, u32 Size, GXRModeObj* rmode, bool patchAll)
{
    u8 *Addr = (u8 *)Address;
    bool found = 0;
    u32 i;

    while(Size >= sizeof(GXRModeObj))
    {
        // Video mode pattern found
        if( (((GXRModeObj*)Addr)->fbWidth == 0x0280 && ((GXRModeObj*)Addr)->viWidth == 0x02c4) || // TVEurgb60Hz480Prog
            (((GXRModeObj*)Addr)->fbWidth == 0x0280 && ((GXRModeObj*)Addr)->viWidth == 0x0280) )  // All other video modes
        {
            found = 0;
            for(i = 0; i < sizeof(vmodes)/sizeof(vmodes[0]); i++)
            {
                if(compare_videomodes(vmodes[i], (GXRModeObj*)Addr))
                {
                    found = 1;
                    patch_videomode((GXRModeObj*)Addr, rmode);
                    Addr += (sizeof(GXRModeObj)-4);
                    Size -= (sizeof(GXRModeObj)-4);
                    break;
                }
            }
            if(patchAll && !found)
            {
                patch_videomode((GXRModeObj*)Addr, rmode);
                Addr += (sizeof(GXRModeObj)-4);
                Size -= (sizeof(GXRModeObj)-4);
            }
        }
        Addr += 4;
        Size -= 4;
    }
}

static bool Search_and_patch_Video_Modes(void *Address, u32 Size, GXRModeObj* Table[])
{
    u8 *Addr = (u8 *)Address;
    bool found = 0;
    u32 i;

    while(Size >= sizeof(GXRModeObj))
    {
        for(i = 0; Table[i]; i+=2)
        {
            if(compare_videomodes(Table[i], (GXRModeObj*)Addr))
            {
                found = 1;
                patch_videomode((GXRModeObj*)Addr, Table[i+1]);
                Addr += (sizeof(GXRModeObj)-4);
                Size -= (sizeof(GXRModeObj)-4);
                break;
            }
        }
        Addr += 4;
        Size -= 4;
    }
    return found;
}

void patchVideoModes(void *dst, u32 len, int vidMode, GXRModeObj *vmode, int patchVidModes, u8 bootType)
{
    GXRModeObj **table = 0;
    char region = *(char *)0x80000003;

    // Video patch set to "all" or the video mode is progressive and it's a PAL Wii game
    if((patchVidModes == 3 || (vidMode == 5 && region == 'P' && bootType == TYPE_WII_GAME)) && vmode != 0)
        applyVideoPatch(dst, len, vmode, true);
    // Video patch set to "more"
    else if(patchVidModes == 2 && vmode != 0)
        applyVideoPatch(dst, len, vmode, false);
    else
    {
        switch(vidMode)
        {
            case 1: // SYSTEM
                switch(CONF_GetVideo())
                {
                    case CONF_VIDEO_PAL:
                        table = CONF_GetEuRGB60() > 0 ? NTSC2PAL60 : NTSC2PAL;
                        break;
                    case CONF_VIDEO_MPAL:
                        table = NTSC2PAL;
                        break;
                    default:
                        table = PAL2NTSC;
                        break;
                }
                Search_and_patch_Video_Modes(dst, len, table);
                break;
            case 2: // PAL50
                Search_and_patch_Video_Modes(dst, len, NTSC2PAL);
                break;
            case 3: // PAL60
                Search_and_patch_Video_Modes(dst, len, NTSC2PAL60);
                break;
            case 4: // NTSC
                Search_and_patch_Video_Modes(dst, len, PAL2NTSC);
                break;
            default:
                break;
        }
    }
}
