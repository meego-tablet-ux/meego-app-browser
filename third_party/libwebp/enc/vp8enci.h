// Copyright 2011 Google Inc.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//   WebP encoder: internal header.
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef WEBP_ENC_VP8ENCI_H_
#define WEBP_ENC_VP8ENCI_H_

#include "string.h"     // for memcpy()
#include "../webp/encode.h"
#include "bit_writer.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Various defines and enums

// intra prediction modes
enum { B_DC_PRED = 0,   // 4x4 modes
       B_TM_PRED = 1,
       B_VE_PRED = 2,
       B_HE_PRED = 3,
       B_RD_PRED = 4,
       B_VR_PRED = 5,
       B_LD_PRED = 6,
       B_VL_PRED = 7,
       B_HD_PRED = 8,
       B_HU_PRED = 9,
       NUM_BMODES = B_HU_PRED + 1 - B_DC_PRED,  // = 10

       // Luma16 or UV modes
       DC_PRED = B_DC_PRED, V_PRED = B_VE_PRED,
       H_PRED = B_HE_PRED, TM_PRED = B_TM_PRED
     };

enum { NUM_MB_SEGMENTS = 4,
       MAX_NUM_PARTITIONS = 8,
       NUM_TYPES = 4,   // 0: i16-AC,  1: i16-DC,  2:chroma-AC,  3:i4-AC
       NUM_BANDS = 8,
       NUM_CTX = 3,
       NUM_PROBAS = 11,
       MAX_LF_LEVELS = 64,      // Maximum loop filter level
       MAX_VARIABLE_LEVEL = 67  // last (inclusive) level with variable cost
     };

// YUV-cache parameters. Cache is 16-pixels wide.
// The original or reconstructed samples can be accessed using VP8Scan[]
// The predicted blocks can be accessed using offsets to yuv_p_ and
// the arrays VP8*ModeOffsets[];
//         +----+      YUV Samples area. See VP8Scan[] for accessing the blocks.
//  Y_OFF  |YYYY| <- original samples  (enc->yuv_in_)
//         |YYYY|
//         |YYYY|
//         |YYYY|
//  U_OFF  |UUVV| V_OFF  (=U_OFF + 8)
//         |UUVV|
//         +----+
//  Y_OFF  |YYYY| <- compressed/decoded samples  ('yuv_out_')
//         |YYYY|    There are two buffers like this ('yuv_out_'/'yuv_out2_')
//         |YYYY|
//         |YYYY|
//  U_OFF  |UUVV| V_OFF
//         |UUVV|
//          x2 (for yuv_out2_)
//         +----+     Prediction area ('yuv_p_', size = PRED_SIZE)
// I16DC16 |YYYY|  Intra16 predictions (16x16 block each)
//         |YYYY|
//         |YYYY|
//         |YYYY|
// I16TM16 |YYYY|
//         |YYYY|
//         |YYYY|
//         |YYYY|
// I16VE16 |YYYY|
//         |YYYY|
//         |YYYY|
//         |YYYY|
// I16HE16 |YYYY|
//         |YYYY|
//         |YYYY|
//         |YYYY|
//         +----+  Chroma U/V predictions (16x8 block each)
// C8DC8   |UUVV|
//         |UUVV|
// C8TM8   |UUVV|
//         |UUVV|
// C8VE8   |UUVV|
//         |UUVV|
// C8HE8   |UUVV|
//         |UUVV|
//         +----+  Intra 4x4 predictions (4x4 block each)
//         |YYYY| I4DC4 I4TM4 I4VE4 I4HE4
//         |YYYY| I4RD4 I4VR4 I4LD4 I4VL4
//         |YY..| I4HD4 I4HU4 I4TMP
//         +----+
#define BPS       16   // this is the common stride
#define Y_SIZE   (BPS * 16)
#define UV_SIZE  (BPS * 8)
#define YUV_SIZE (Y_SIZE + UV_SIZE)
#define PRED_SIZE (6 * 16 * BPS + 12 * BPS)
#define Y_OFF    (0)
#define U_OFF    (Y_SIZE)
#define V_OFF    (U_OFF + 8)
#define ALIGN_CST 15
#define DO_ALIGN(PTR) ((uintptr_t)((PTR) + ALIGN_CST) & ~ALIGN_CST)

extern const int VP8Scan[16 + 4 + 4];           // in quant.c
extern const int VP8UVModeOffsets[4];           // in analyze.c
extern const int VP8I16ModeOffsets[4];
extern const int VP8I4ModeOffsets[NUM_BMODES];

// Layout of prediction blocks
// intra 16x16
#define I16DC16 (0 * 16 * BPS)
#define I16TM16 (1 * 16 * BPS)
#define I16VE16 (2 * 16 * BPS)
#define I16HE16 (3 * 16 * BPS)
// chroma 8x8, two U/V blocks side by side (hence: 16x8 each)
#define C8DC8 (4 * 16 * BPS)
#define C8TM8 (4 * 16 * BPS + 8 * BPS)
#define C8VE8 (5 * 16 * BPS)
#define C8HE8 (5 * 16 * BPS + 8 * BPS)
// intra 4x4
#define I4DC4 (6 * 16 * BPS +  0)
#define I4TM4 (6 * 16 * BPS +  4)
#define I4VE4 (6 * 16 * BPS +  8)
#define I4HE4 (6 * 16 * BPS + 12)
#define I4RD4 (6 * 16 * BPS + 4 * BPS +  0)
#define I4VR4 (6 * 16 * BPS + 4 * BPS +  4)
#define I4LD4 (6 * 16 * BPS + 4 * BPS +  8)
#define I4VL4 (6 * 16 * BPS + 4 * BPS + 12)
#define I4HD4 (6 * 16 * BPS + 8 * BPS +  0)
#define I4HU4 (6 * 16 * BPS + 8 * BPS +  4)
#define I4TMP (6 * 16 * BPS + 8 * BPS +  8)

typedef int64_t score_t;     // type used for scores, rate, distortion
#define MAX_COST ((score_t)0x7fffffffffffffLL)

//-----------------------------------------------------------------------------
// Headers

typedef uint8_t ProbaArray[NUM_CTX][NUM_PROBAS];
typedef uint64_t StatsArray[NUM_CTX][NUM_PROBAS][2];
typedef uint16_t CostArray[NUM_CTX][MAX_VARIABLE_LEVEL + 1];
typedef double LFStats[NUM_MB_SEGMENTS][MAX_LF_LEVELS];  // filter stats

typedef struct VP8Encoder VP8Encoder;

// segment features
typedef struct {
  int num_segments_;      // Actual number of segments. 1 segment only = unused.
  int update_map_;        // whether to update the segment map or not.
                          // must be 0 if there's only 1 segment.
  int size_;              // bit-cost for transmitting the segment map
} VP8SegmentHeader;

// Struct collecting all frame-persistent probabilities.
typedef struct {
  uint8_t segments_[3];     // probabilities for segment tree
  uint8_t skip_proba_;      // final probability of being skipped.
  ProbaArray coeffs_[NUM_TYPES][NUM_BANDS];      // 924 bytes
  StatsArray stats_[NUM_TYPES][NUM_BANDS];       // 7.4k
  CostArray level_cost_[NUM_TYPES][NUM_BANDS];   // 11.4k
  int use_skip_proba_;      // Note: we always use skip_proba for now.
  int nb_skip_, nb_i4_, nb_i16_;   // block type counters
} VP8Proba;

// Filter parameters. Not actually used in the code (we don't perform
// the in-loop filtering), but filled from user's config
typedef struct {
  int simple_;             // filtering type: 0=complex, 1=simple
  int level_;              // base filter level [0..63]
  int sharpness_;          // [0..7]
  int i4x4_lf_delta_;      // delta filter level for i4x4 relative to i16x16
} VP8FilterHeader;

//-----------------------------------------------------------------------------
// Informations about the macroblocks.

typedef struct {
  // block type
  uint8_t type_:2;     // 0=i4x4, 1=i16x16
  uint8_t uv_mode_:2;
  uint8_t skip_:1;
  uint8_t segment_:2;
  uint8_t alpha_;      // quantization-susceptibility
} VP8MBInfo;

typedef struct {
  uint16_t q_[16];        // quantizer steps
  uint16_t iq_[16];       // reciprocals, fixed point.
  uint16_t bias_[16];     // rounding bias
  uint16_t zthresh_[16];  // value under which a coefficient is zeroed
  uint16_t sharpen_[16];  // frequency boosters for slight sharpening
} VP8Matrix;

typedef struct {
  VP8Matrix y1_, y2_, uv_;  // quantization matrices
  int alpha_;      // quant-susceptibility, range [-127,127]. Zero is neutral.
                   // Lower values indicate a lower risk of blurriness.
  int beta_;       // filter-susceptibility, range [0,255].
  int quant_;      // final segment quantizer.
  int fstrength_;  // final in-loop filtering strength
  // reactivities
  int lambda_i16_, lambda_i4_, lambda_uv_;
  int lambda_mode_, lambda_trellis_, tlambda_;
  int lambda_trellis_i16_, lambda_trellis_i4_, lambda_trellis_uv_;
} VP8SegmentInfo;

// Handy transcient struct to accumulate score and info during RD-optimization
// and mode evaluation.
typedef struct {
  score_t D, SD, R, score;    // Distortion, spectral distortion, rate, score.
  int16_t y_dc_levels[16];    // Quantized levels for luma-DC, luma-AC, chroma.
  int16_t y_ac_levels[16][16];
  int16_t uv_levels[4 + 4][16];
  int mode_i16;               // mode number for intra16 prediction
  int modes_i4[16];           // mode numbers for intra4 predictions
  int mode_uv;                // mode number of chroma prediction
  uint32_t nz;                // non-zero blocks
} VP8ModeScore;

// Iterator structure to iterate through macroblocks, pointing to the
// right neighbouring data (samples, predictions, contexts, ...)
typedef struct {
  int x_, y_;                      // current macroblock
  int y_offset_, uv_offset_;       // offset to the luma / chroma planes
  int y_stride_, uv_stride_;       // respective strides
  uint8_t*      yuv_in_;           // borrowed from enc_ (for now)
  uint8_t*      yuv_out_;          // ''
  uint8_t*      yuv_out2_;         // ''
  uint8_t*      yuv_p_;            // ''
  VP8Encoder*   enc_;              // back-pointer
  VP8MBInfo*    mb_;               // current macroblock
  VP8BitWriter* bw_;               // current bit-writer
  uint8_t*      preds_;            // intra mode predictors (4x4 blocks)
  uint32_t*     nz_;               // non-zero pattern
  uint8_t       i4_boundary_[37];  // 32+5 boundary samples needed by intra4x4
  uint8_t*      i4_top_;           // pointer to the current *top boundary sample
  int           i4_;               // current intra4x4 mode being tested
  int           top_nz_[9];        // top-non-zero context.
  int           left_nz_[9];       // left-non-zero. left_nz[8] is independent.
  uint64_t      bit_count_[4][3];  // bit counters for coded levels.
  uint64_t      luma_bits_;        // macroblock bit-cost for luma
  uint64_t      uv_bits_;          // macroblock bit-cost for chroma
  LFStats*      lf_stats_;         // filter stats (borrowed from enc_)
  int           do_trellis_;       // if true, perform extra level optimisation
  int           done_;             // true when scan is finished
} VP8EncIterator;

  // in iterator.c
// must be called first.
void VP8IteratorInit(VP8Encoder* const enc, VP8EncIterator* const it);
// restart a scan.
void VP8IteratorReset(VP8EncIterator* const it);
// import samples from source
void VP8IteratorImport(const VP8EncIterator* const it);
// export decimated samples
void VP8IteratorExport(const VP8EncIterator* const it);
// go to next macroblock. Returns !done_. If *block_to_save is non-null, will
// save the boundary values to top_/left_ arrays. block_to_save can be
// it->yuv_out_ or it->yuv_in_.
int VP8IteratorNext(VP8EncIterator* const it,
                    const uint8_t* const block_to_save);
// Intra4x4 iterations
void VP8IteratorStartI4(VP8EncIterator* const it);
// returns true if not done.
int VP8IteratorRotateI4(VP8EncIterator* const it,
                        const uint8_t* const yuv_out);

// Non-zero context setup/teardown
void VP8IteratorNzToBytes(VP8EncIterator* const it);
void VP8IteratorBytesToNz(VP8EncIterator* const it);

// Helper functions to set mode properties
void VP8SetIntra16Mode(const VP8EncIterator* const it, int mode);
void VP8SetIntra4Mode(const VP8EncIterator* const it, int modes[16]);
void VP8SetIntraUVMode(const VP8EncIterator* const it, int mode);
void VP8SetSkip(const VP8EncIterator* const it, int skip);
void VP8SetSegment(const VP8EncIterator* const it, int segment);
void VP8IteratorResetCosts(VP8EncIterator* const it);

//-----------------------------------------------------------------------------
// VP8Encoder

struct VP8Encoder {
  const WebPConfig* config_;    // user configuration and parameters
  WebPPicture* pic_;            // input / output picture

  // headers
  VP8FilterHeader   filter_hdr_;     // filtering information
  VP8SegmentHeader  segment_hdr_;    // segment information

  int profile_;                      // VP8's profile, deduced from Config.

  // dimension, in macroblock units.
  int mb_w_, mb_h_;
  int preds_w_;   // stride of the *preds_ prediction plane (=4*mb_w + 1)

  // number of partitions (1, 2, 4 or 8 = MAX_NUM_PARTITIONS)
  int num_parts_;

  // per-partition boolean decoders.
  VP8BitWriter bw_;                         // part0
  VP8BitWriter parts_[MAX_NUM_PARTITIONS];  // token partitions

  // quantization info (one set of DC/AC dequant factor per segment)
  VP8SegmentInfo dqm_[NUM_MB_SEGMENTS];
  int base_quant_;                 // nominal quantizer value. Only used
                                   // for relative coding of segments' quant.
  int uv_alpha_;                   // U/V quantization susceptibility
  // global offset of quantizers, shared by all segments
  int dq_y1_dc_;
  int dq_y2_dc_, dq_y2_ac_;
  int dq_uv_dc_, dq_uv_ac_;

  // probabilities and statistics
  VP8Proba proba_;
  uint64_t sse_[3];        // sum of Y/U/V squared errors for all macroblocks
  uint64_t sse_count_;     // pixel count for the sse_[] stats
  int      coded_size_;
  int      residual_bytes_[3][4];
  int      block_count_[3];

  // quality/speed settings
  int method_;             // 0=fastest, 6=best/slowest.
  int rd_opt_level_;       // Deduced from method_.

  // Memory
  VP8MBInfo* mb_info_;   // contextual macroblock infos (mb_w_ + 1)
  uint8_t*   preds_;     // predictions modes: (4*mb_w+1) * (4*mb_h+1)
  uint32_t*  nz_;        // non-zero bit context: mb_w+1
  uint8_t*   yuv_in_;    // input samples
  uint8_t*   yuv_out_;   // output samples
  uint8_t*   yuv_out2_;  // secondary scratch out-buffer. swapped with yuv_out_.
  uint8_t*   yuv_p_;     // scratch buffer for prediction
  uint8_t   *y_top_;     // top luma samples.
  uint8_t   *uv_top_;    // top u/v samples.
                         // U and V are packed into 16 pixels (8 U + 8 V)
  uint8_t   *y_left_;    // left luma samples (adressable from index -1 to 15).
  uint8_t   *u_left_;    // left u samples (adressable from index -1 to 7)
  uint8_t   *v_left_;    // left v samples (adressable from index -1 to 7)

  LFStats   *lf_stats_;  // autofilter stats (if NULL, autofilter is off)
};

//-----------------------------------------------------------------------------
// internal functions. Not public.

  // in tree.c
extern const uint8_t VP8CoeffsProba0[NUM_TYPES][NUM_BANDS][NUM_CTX][NUM_PROBAS];
extern const uint8_t
    VP8CoeffsUpdateProba[NUM_TYPES][NUM_BANDS][NUM_CTX][NUM_PROBAS];
// Reset the token probabilities to their initial (default) values
void VP8DefaultProbas(VP8Encoder* const enc);
// Write the token probabilities
void VP8WriteProbas(VP8BitWriter* const bw, const VP8Proba* const probas);
// Writes the partition #0 modes (that is: all intra modes)
void VP8CodeIntraModes(VP8Encoder* const enc);

  // in syntax.c
// Generates the final bitstream by coding the partition0 and headers,
// and appending an assembly of all the pre-coded token partitions.
// Return true if everything is ok.
int VP8EncWrite(VP8Encoder* const enc);

  // in frame.c
extern const uint8_t VP8EncBands[16 + 1];
// Form all the four Intra16x16 predictions in the yuv_p_ cache
void VP8MakeLuma16Preds(const VP8EncIterator* const it);
// Form all the four Chroma8x8 predictions in the yuv_p_ cache
void VP8MakeChroma8Preds(const VP8EncIterator* const it);
// Form all the ten Intra4x4 predictions in the yuv_p_ cache
// for the 4x4 block it->i4_
void VP8MakeIntra4Preds(const VP8EncIterator* const it);
// Rate calculation
int VP8GetCostLuma16(VP8EncIterator* const it, const VP8ModeScore* const rd);
int VP8GetCostLuma4(VP8EncIterator* const it, const int16_t levels[16]);
int VP8GetCostUV(VP8EncIterator* const it, const VP8ModeScore* const rd);
// Main stat / coding passes
int VP8EncLoop(VP8Encoder* const enc);
int VP8StatLoop(VP8Encoder* const enc);

  // in analysis.c
// Main analysis loop. Decides the segmentations and complexity.
// Assigns a first guess for Intra16 and uvmode_ prediction modes.
int VP8EncAnalyze(VP8Encoder* const enc);

  // in quant.c
// Sets up segment's quantization values, base_quant_ and filter strengths.
void VP8SetSegmentParams(VP8Encoder* const enc, float quality);
// Pick best modes and fills the levels. Returns true if skipped.
int VP8Decimate(VP8EncIterator* const it, VP8ModeScore* const rd, int rd_opt);

  // in dsp.c
// Transforms
typedef void (*VP8Idct)(const uint8_t* ref, const int16_t* in, uint8_t* dst);
typedef void (*VP8Fdct)(const uint8_t* src, const uint8_t* ref, int16_t* out);
typedef void (*VP8WHT)(const int16_t* in, int16_t* out);
extern VP8Idct VP8ITransform;
extern VP8Fdct VP8FTransform;
extern VP8WHT VP8ITransformWHT;
extern VP8WHT VP8FTransformWHT;
// Predictions
// *dst is the destination block. *top, *top_right and *left can be NULL.
typedef void (*VP8IntraPreds)(uint8_t *dst, const uint8_t* left,
                              const uint8_t* top);
typedef void (*VP8Intra4Preds)(uint8_t *dst, const uint8_t* top);
extern VP8Intra4Preds VP8EncPredLuma4;
extern VP8IntraPreds VP8EncPredLuma16;
extern VP8IntraPreds VP8EncPredChroma8;

typedef int (*VP8Metric)(const uint8_t* pix, const uint8_t* ref);
extern VP8Metric VP8SSE16x16, VP8SSE16x8, VP8SSE8x8, VP8SSE4x4;
typedef int (*VP8WMetric)(const uint8_t* pix, const uint8_t* ref,
                          const uint16_t* const weights);
extern VP8WMetric VP8TDisto4x4, VP8TDisto16x16;

typedef void (*VP8BlockCopy)(const uint8_t* src, uint8_t* dst);
extern VP8BlockCopy VP8Copy4x4;
extern VP8BlockCopy VP8Copy8x8;
extern VP8BlockCopy VP8Copy16x16;

void VP8EncDspInit();   // must be called before using anything from the above.

  // in filter.c
extern void VP8InitFilter(VP8EncIterator* const it);
extern void VP8StoreFilterStats(VP8EncIterator* const it);
extern void VP8AdjustFilterStrength(VP8EncIterator* const it);

//-----------------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // WEBP_ENC_VP8ENCI_H_
