// jpge.cpp - C++ class for JPEG compression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// v1.01, Dec. 18, 2010 - Initial release
// v1.02, Apr. 6, 2011 - Removed 2x2 ordered dither in H2V1 chroma subsampling method load_block_16_8_8(). (The rounding factor was 2, when it should have been 1. Either way, it wasn't helping.)
// v1.03, Apr. 16, 2011 - Added support for optimized Huffman code tables, optimized dynamic memory allocation down to only 1 alloc.
//                        Also from Alex Evans: Added RGBA support, linear memory allocator (no longer needed in v1.03).
// v1.04, May. 19, 2012: Forgot to set m_pFile ptr to NULL in cfile_stream::close(). Thanks to Owen Kaluza for reporting this bug.
//                       Code tweaks to fix VS2008 static code analysis warnings (all looked harmless).
//                       Code review revealed method load_block_16_8_8() (used for the non-default H2V1 sampling mode to downsample chroma) somehow didn't get the rounding factor fix from v1.02.

#define INLINE static __inline__ __attribute__((always_inline))
#define _INLINE __inline__ __attribute__((always_inline))
#define NOINLINE __attribute__ ((noinline))


#include "jpge.h"
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>


#define JPGE_MAX(a,b) (((a)>(b))?(a):(b))
#define JPGE_MIN(a,b) (((a)<(b))?(a):(b))

namespace jpge {

static inline void *jpge_malloc(size_t nSize) { return malloc(nSize); }
static inline void jpge_free(void *p) { free(p); }

// Various JPEG enums and tables.
enum { M_SOF0 = 0xC0, M_DHT = 0xC4, M_SOI = 0xD8, M_EOI = 0xD9, M_SOS = 0xDA, M_DQT = 0xDB, M_APP0 = 0xE0, M_COM = 0xFE };
enum { DC_LUM_CODES = 12, AC_LUM_CODES = 256, DC_CHROMA_CODES = 12, AC_CHROMA_CODES = 256, MAX_HUFF_SYMBOLS = 257, MAX_HUFF_CODESIZE = 32 };

static uint8 s_zag[64] = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
static int16 s_std_lum_quant[64] = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
static int16 s_std_croma_quant[64] = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };
static uint8 s_dc_lum_bits[17] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static uint8 s_dc_lum_val[DC_LUM_CODES] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static uint8 s_ac_lum_bits[17] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static uint8 s_ac_lum_val[AC_LUM_CODES]  =
{
  0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
  0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
  0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
  0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
  0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};
static uint8 s_dc_chroma_bits[17] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static uint8 s_dc_chroma_val[DC_CHROMA_CODES]  = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static uint8 s_ac_chroma_bits[17] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
static uint8 s_ac_chroma_val[AC_CHROMA_CODES] =
{
  0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
  0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
  0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
  0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
  0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};

// Low-level helper functions.
template <class T> inline void clear_obj(T &obj) { memset(&obj, 0, sizeof(obj)); }

#define YR 19595
#define YG 38470
#define YB 7471
#define CB_R -11059
#define CB_G -21709
#define CB_B 32768
#define CR_R 32768
#define CR_G -27439
#define CR_B -5329

static inline uint8 clamp(int i) { if (static_cast<uint>(i) > 255U) { if (i < 0) i = 0; else if (i > 255) i = 255; } return static_cast<uint8>(i); }

INLINE void RGB_to_YCC(uint8* pDst, const uint8 *pSrc, int num_pixels)
{
  for ( ; num_pixels; pDst += 3, pSrc += 3, num_pixels--)
  {
    const int r = pSrc[0], g = pSrc[1], b = pSrc[2];
    pDst[0] = static_cast<uint8>((r * YR + g * YG + b * YB + 32768) >> 16);
    pDst[1] = clamp(128 + ((r * CB_R + g * CB_G + b * CB_B + 32768) >> 16));
    pDst[2] = clamp(128 + ((r * CR_R + g * CR_G + b * CR_B + 32768) >> 16));
  }
}

INLINE void RGB_to_Y(uint8* pDst, const uint8 *pSrc, int num_pixels)
{
  for ( ; num_pixels; pDst++, pSrc += 3, num_pixels--)
    pDst[0] = static_cast<uint8>((pSrc[0] * YR + pSrc[1] * YG + pSrc[2] * YB + 32768) >> 16);
}

INLINE void RGBA_to_YCC(uint8* pDst, const uint8 *pSrc, int num_pixels)
{
  for ( ; num_pixels; pDst += 3, pSrc += 4, num_pixels--)
  {
    const int r = pSrc[0], g = pSrc[1], b = pSrc[2];
    pDst[0] = static_cast<uint8>((r * YR + g * YG + b * YB + 32768) >> 16);
    pDst[1] = clamp(128 + ((r * CB_R + g * CB_G + b * CB_B + 32768) >> 16));
    pDst[2] = clamp(128 + ((r * CR_R + g * CR_G + b * CR_B + 32768) >> 16));
  }
}

INLINE void RGBA_to_Y(uint8* pDst, const uint8 *pSrc, int num_pixels)
{
  for ( ; num_pixels; pDst++, pSrc += 4, num_pixels--)
    pDst[0] = static_cast<uint8>((pSrc[0] * YR + pSrc[1] * YG + pSrc[2] * YB + 32768) >> 16);
}

INLINE void Y_to_YCC(uint8* pDst, const uint8* pSrc, int num_pixels)
{
  for( ; num_pixels; pDst += 3, pSrc++, num_pixels--) { pDst[0] = pSrc[0]; pDst[1] = 128; pDst[2] = 128; }
}

// Forward DCT - DCT derived from jfdctint.
enum { CONST_BITS = 13, ROW_BITS = 2 };
#define DCT_DESCALE(x, n) (((x) + (((int32)1) << ((n) - 1))) >> (n))
#define DCT_MUL(var, c) (static_cast<int16>(var) * static_cast<int32>(c))
#define DCT1D(s0, s1, s2, s3, s4, s5, s6, s7) \
  int32 t0 = s0 + s7, t7 = s0 - s7, t1 = s1 + s6, t6 = s1 - s6, t2 = s2 + s5, t5 = s2 - s5, t3 = s3 + s4, t4 = s3 - s4; \
  int32 t10 = t0 + t3, t13 = t0 - t3, t11 = t1 + t2, t12 = t1 - t2; \
  int32 u1 = DCT_MUL(t12 + t13, 4433); \
  s2 = u1 + DCT_MUL(t13, 6270); \
  s6 = u1 + DCT_MUL(t12, -15137); \
  u1 = t4 + t7; \
  int32 u2 = t5 + t6, u3 = t4 + t6, u4 = t5 + t7; \
  int32 z5 = DCT_MUL(u3 + u4, 9633); \
  t4 = DCT_MUL(t4, 2446); t5 = DCT_MUL(t5, 16819); \
  t6 = DCT_MUL(t6, 25172); t7 = DCT_MUL(t7, 12299); \
  u1 = DCT_MUL(u1, -7373); u2 = DCT_MUL(u2, -20995); \
  u3 = DCT_MUL(u3, -16069); u4 = DCT_MUL(u4, -3196); \
  u3 += z5; u4 += z5; \
  s0 = t10 + t11; s1 = t7 + u1 + u4; s3 = t6 + u2 + u3; s4 = t10 - t11; s5 = t5 + u2 + u4; s7 = t4 + u1 + u3;

INLINE void DCT2D(int32 *p)
{
  int32 c, *q = p;
  for (c = 7; c >= 0; c--, q += 8)
  {
    int32 s0 = q[0], s1 = q[1], s2 = q[2], s3 = q[3], s4 = q[4], s5 = q[5], s6 = q[6], s7 = q[7];
    DCT1D(s0, s1, s2, s3, s4, s5, s6, s7);
    q[0] = s0 << ROW_BITS; q[1] = DCT_DESCALE(s1, CONST_BITS-ROW_BITS); q[2] = DCT_DESCALE(s2, CONST_BITS-ROW_BITS); q[3] = DCT_DESCALE(s3, CONST_BITS-ROW_BITS);
    q[4] = s4 << ROW_BITS; q[5] = DCT_DESCALE(s5, CONST_BITS-ROW_BITS); q[6] = DCT_DESCALE(s6, CONST_BITS-ROW_BITS); q[7] = DCT_DESCALE(s7, CONST_BITS-ROW_BITS);
  }
  for (q = p, c = 7; c >= 0; c--, q++)
  {
    int32 s0 = q[0*8], s1 = q[1*8], s2 = q[2*8], s3 = q[3*8], s4 = q[4*8], s5 = q[5*8], s6 = q[6*8], s7 = q[7*8];
    DCT1D(s0, s1, s2, s3, s4, s5, s6, s7);
    q[0*8] = DCT_DESCALE(s0, ROW_BITS+3); q[1*8] = DCT_DESCALE(s1, CONST_BITS+ROW_BITS+3); q[2*8] = DCT_DESCALE(s2, CONST_BITS+ROW_BITS+3); q[3*8] = DCT_DESCALE(s3, CONST_BITS+ROW_BITS+3);
    q[4*8] = DCT_DESCALE(s4, ROW_BITS+3); q[5*8] = DCT_DESCALE(s5, CONST_BITS+ROW_BITS+3); q[6*8] = DCT_DESCALE(s6, CONST_BITS+ROW_BITS+3); q[7*8] = DCT_DESCALE(s7, CONST_BITS+ROW_BITS+3);
  }
}

struct sym_freq { uint m_key, m_sym_index; };

// Radix sorts sym_freq[] array by 32-bit key m_key. Returns ptr to sorted values.
INLINE sym_freq* radix_sort_syms(uint num_syms, sym_freq* pSyms0, sym_freq* pSyms1)
{
  const uint cMaxPasses = 4;
  uint32 hist[256 * cMaxPasses]; clear_obj(hist);
  for (uint i = 0; i < num_syms; i++) { uint freq = pSyms0[i].m_key; hist[freq & 0xFF]++; hist[256 + ((freq >> 8) & 0xFF)]++; hist[256*2 + ((freq >> 16) & 0xFF)]++; hist[256*3 + ((freq >> 24) & 0xFF)]++; }
  sym_freq* pCur_syms = pSyms0, *pNew_syms = pSyms1;
  uint total_passes = cMaxPasses; while ((total_passes > 1) && (num_syms == hist[(total_passes - 1) * 256])) total_passes--;
  for (uint pass_shift = 0, pass = 0; pass < total_passes; pass++, pass_shift += 8)
  {
    const uint32* pHist = &hist[pass << 8];
    uint offsets[256], cur_ofs = 0;
    for (uint i = 0; i < 256; i++) { offsets[i] = cur_ofs; cur_ofs += pHist[i]; }
    for (uint i = 0; i < num_syms; i++)
      pNew_syms[offsets[(pCur_syms[i].m_key >> pass_shift) & 0xFF]++] = pCur_syms[i];
    sym_freq* t = pCur_syms; pCur_syms = pNew_syms; pNew_syms = t;
  }
  return pCur_syms;
}

// calculate_minimum_redundancy() originally written by: Alistair Moffat, alistair@cs.mu.oz.au, Jyrki Katajainen, jyrki@diku.dk, November 1996.
INLINE void calculate_minimum_redundancy(sym_freq *A, int n)
{
  int root, leaf, next, avbl, used, dpth;
  if (n==0) return; else if (n==1) { A[0].m_key = 1; return; }
  A[0].m_key += A[1].m_key; root = 0; leaf = 2;
  for (next=1; next < n-1; next++)
  {
    if (leaf>=n || A[root].m_key<A[leaf].m_key) { A[next].m_key = A[root].m_key; A[root++].m_key = next; } else A[next].m_key = A[leaf++].m_key;
    if (leaf>=n || (root<next && A[root].m_key<A[leaf].m_key)) { A[next].m_key += A[root].m_key; A[root++].m_key = next; } else A[next].m_key += A[leaf++].m_key;
  }
  A[n-2].m_key = 0;
  for (next=n-3; next>=0; next--) A[next].m_key = A[A[next].m_key].m_key+1;
  avbl = 1; used = dpth = 0; root = n-2; next = n-1;
  while (avbl>0)
  {
    while (root>=0 && (int)A[root].m_key==dpth) { used++; root--; }
    while (avbl>used) { A[next--].m_key = dpth; avbl--; }
    avbl = 2*used; dpth++; used = 0;
  }
}

// Limits canonical Huffman code table's max code size to max_code_size.
INLINE void huffman_enforce_max_code_size(int *pNum_codes, int code_list_len, int max_code_size)
{
  if (code_list_len <= 1) return;

  for (int i = max_code_size + 1; i <= MAX_HUFF_CODESIZE; i++) pNum_codes[max_code_size] += pNum_codes[i];

  uint32 total = 0;
  for (int i = max_code_size; i > 0; i--)
    total += (((uint32)pNum_codes[i]) << (max_code_size - i));

  while (total != (1UL << max_code_size))
  {
    pNum_codes[max_code_size]--;
    for (int i = max_code_size - 1; i > 0; i--)
    {
      if (pNum_codes[i]) { pNum_codes[i]--; pNum_codes[i + 1] += 2; break; }
    }
    total--;
  }
}

// Generates an optimized offman table.
_INLINE void jpeg_encoder::optimize_huffman_table(int table_num, int table_len)
{
  sym_freq syms0[MAX_HUFF_SYMBOLS], syms1[MAX_HUFF_SYMBOLS];
  syms0[0].m_key = 1; syms0[0].m_sym_index = 0;  // dummy symbol, assures that no valid code contains all 1's
  int num_used_syms = 1;
  const uint32 *pSym_count = &m_huff_count[table_num][0];
  for (int i = 0; i < table_len; i++)
    if (pSym_count[i]) { syms0[num_used_syms].m_key = pSym_count[i]; syms0[num_used_syms++].m_sym_index = i + 1; }
  sym_freq* pSyms = radix_sort_syms(num_used_syms, syms0, syms1);
  calculate_minimum_redundancy(pSyms, num_used_syms);

  // Count the # of symbols of each code size.
  int num_codes[1 + MAX_HUFF_CODESIZE]; clear_obj(num_codes);
  for (int i = 0; i < num_used_syms; i++)
    num_codes[pSyms[i].m_key]++;

  const uint JPGE_CODE_SIZE_LIMIT = 16; // the maximum possible size of a JPEG Huffman code (valid range is [9,16] - 9 vs. 8 because of the dummy symbol)
  huffman_enforce_max_code_size(num_codes, num_used_syms, JPGE_CODE_SIZE_LIMIT);

  // Compute m_huff_bits array, which contains the # of symbols per code size.
  clear_obj(m_huff_bits[table_num]);
  for (int i = 1; i <= (int)JPGE_CODE_SIZE_LIMIT; i++)
    m_huff_bits[table_num][i] = static_cast<uint8>(num_codes[i]);

  // Remove the dummy symbol added above, which must be in largest bucket.
  for (int i = JPGE_CODE_SIZE_LIMIT; i >= 1; i--)
  {
    if (m_huff_bits[table_num][i]) { m_huff_bits[table_num][i]--; break; }
  }

  // Compute the m_huff_val array, which contains the symbol indices sorted by code size (smallest to largest).
  for (int i = num_used_syms - 1; i >= 1; i--)
    m_huff_val[table_num][num_used_syms - 1 - i] = static_cast<uint8>(pSyms[i].m_sym_index - 1);
}

// JPEG marker generation.
_INLINE void jpeg_encoder::emit_byte(uint8 i)
{
  m_all_stream_writes_succeeded = m_all_stream_writes_succeeded && m_pStream->put_obj(i);
}

_INLINE void jpeg_encoder::emit_word(uint i)
{
  emit_byte(uint8(i >> 8)); emit_byte(uint8(i & 0xFF));
}

_INLINE void jpeg_encoder::emit_marker(int marker)
{
  emit_byte(uint8(0xFF)); emit_byte(uint8(marker));
}

// Emit Comment
_INLINE void jpeg_encoder::emit_comment(const char* comment)
{
  emit_marker(M_COM);
  int len = strlen(comment) +1;

  emit_word(len +2);

  for(int i = 0; i < len; ++i)
  {
	emit_byte(comment[i]);
  }

}


// Emit JFIF marker
_INLINE void jpeg_encoder::emit_jfif_app0()
{
  emit_marker(M_APP0);
  emit_word(2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1);
  emit_byte(0x4A); emit_byte(0x46); emit_byte(0x49); emit_byte(0x46); /* Identifier: ASCII "JFIF" */
  emit_byte(0);
  emit_byte(1);      /* Major version */
  emit_byte(1);      /* Minor version */
  emit_byte(0);      /* Density unit */
  emit_word(1);
  emit_word(1);
  emit_byte(0);      /* No thumbnail image */
  emit_byte(0);
}

// Emit quantization tables
_INLINE void jpeg_encoder::emit_dqt()
{
  for (int i = 0; i < ((m_num_components == 3) ? 2 : 1); i++)
  {
    emit_marker(M_DQT);
    emit_word(64 + 1 + 2);
    emit_byte(static_cast<uint8>(i));
    for (int j = 0; j < 64; j++)
      emit_byte(static_cast<uint8>(m_quantization_tables[i][j]));
  }
}

// Emit start of frame marker
_INLINE void jpeg_encoder::emit_sof()
{
  emit_marker(M_SOF0);                           /* baseline */
  emit_word(3 * m_num_components + 2 + 5 + 1);
  emit_byte(8);                                  /* precision */
  emit_word(m_image_y);
  emit_word(m_image_x);
  emit_byte(m_num_components);
  for (int i = 0; i < m_num_components; i++)
  {
    emit_byte(static_cast<uint8>(i + 1));                                   /* component ID     */
    emit_byte((m_comp_h_samp[i] << 4) + m_comp_v_samp[i]);  /* h and v sampling */
    emit_byte(i > 0);                                   /* quant. table num */
  }
}

// Emit Huffman table.
_INLINE void jpeg_encoder::emit_dht(uint8 *bits, uint8 *val, int index, bool ac_flag)
{
  emit_marker(M_DHT);

  int length = 0;
  for (int i = 1; i <= 16; i++)
    length += bits[i];

  emit_word(length + 2 + 1 + 16);
  emit_byte(static_cast<uint8>(index + (ac_flag << 4)));

  for (int i = 1; i <= 16; i++)
    emit_byte(bits[i]);

  for (int i = 0; i < length; i++)
    emit_byte(val[i]);
}

// Emit all Huffman tables.
_INLINE void jpeg_encoder::emit_dhts()
{
  emit_dht(m_huff_bits[0+0], m_huff_val[0+0], 0, false);
  emit_dht(m_huff_bits[2+0], m_huff_val[2+0], 0, true);
  if (m_num_components == 3)
  {
    emit_dht(m_huff_bits[0+1], m_huff_val[0+1], 1, false);
    emit_dht(m_huff_bits[2+1], m_huff_val[2+1], 1, true);
  }
}

// emit start of scan
_INLINE void jpeg_encoder::emit_sos()
{
  emit_marker(M_SOS);
  emit_word(2 * m_num_components + 2 + 1 + 3);
  emit_byte(m_num_components);
  for (int i = 0; i < m_num_components; i++)
  {
    emit_byte(static_cast<uint8>(i + 1));
    if (i == 0)
      emit_byte((0 << 4) + 0);
    else
      emit_byte((1 << 4) + 1);
  }
  emit_byte(0);     /* spectral selection */
  emit_byte(63);
  emit_byte(0);
}

// Emit all markers at beginning of image file.
_INLINE void jpeg_encoder::emit_markers()
{
  emit_marker(M_SOI);
  emit_jfif_app0();
  emit_dqt();
  emit_sof();
  emit_dhts();
  emit_sos();
}

// Compute the actual canonical Huffman codes/code sizes given the JPEG huff bits and val arrays.
_INLINE void jpeg_encoder::compute_huffman_table(uint *codes, uint8 *code_sizes, uint8 *bits, uint8 *val)
{
  int i, l, last_p, si;
  uint8 huff_size[257];
  uint huff_code[257];
  uint code;

  int p = 0;
  for (l = 1; l <= 16; l++)
    for (i = 1; i <= bits[l]; i++)
      huff_size[p++] = (char)l;

  huff_size[p] = 0; last_p = p; // write sentinel

  code = 0; si = huff_size[0]; p = 0;

  while (huff_size[p])
  {
    while (huff_size[p] == si)
      huff_code[p++] = code++;
    code <<= 1;
    si++;
  }

  memset(codes, 0, sizeof(codes[0])*256);
  memset(code_sizes, 0, sizeof(code_sizes[0])*256);
  for (p = 0; p < last_p; p++)
  {
    codes[val[p]]      = huff_code[p];
    code_sizes[val[p]] = huff_size[p];
  }
}

// Quantization table generation.
_INLINE void jpeg_encoder::compute_quant_table(int32 *pDst, int16 *pSrc)
{
  int32 q;
  if (m_params.m_quality < 50)
    q = 5000 / m_params.m_quality;
  else
    q = 200 - m_params.m_quality * 2;
  for (int i = 0; i < 64; i++)
  {
    int32 j = *pSrc++; j = (j * q + 50L) / 100L;
    *pDst++ = JPGE_MIN(JPGE_MAX(j, 1), 255);
  }
}

// Higher-level methods.
_INLINE void jpeg_encoder::first_pass_init()
{
  m_bit_buffer = 0; m_bits_in = 0;
  memset(m_last_dc_val, 0, 3 * sizeof(m_last_dc_val[0]));
  m_mcu_y_ofs = 0;
  m_pass_num = 1;
}

_INLINE bool jpeg_encoder::second_pass_init()
{
  compute_huffman_table(&m_huff_codes[0+0][0], &m_huff_code_sizes[0+0][0], m_huff_bits[0+0], m_huff_val[0+0]);
  compute_huffman_table(&m_huff_codes[2+0][0], &m_huff_code_sizes[2+0][0], m_huff_bits[2+0], m_huff_val[2+0]);
  if (m_num_components > 1)
  {
    compute_huffman_table(&m_huff_codes[0+1][0], &m_huff_code_sizes[0+1][0], m_huff_bits[0+1], m_huff_val[0+1]);
    compute_huffman_table(&m_huff_codes[2+1][0], &m_huff_code_sizes[2+1][0], m_huff_bits[2+1], m_huff_val[2+1]);
  }
  first_pass_init();
  emit_markers();
  m_pass_num = 2;
  return true;
}

NOINLINE bool jpeg_encoder::jpg_open(int p_x_res, int p_y_res, int src_channels)
{
  m_num_components = 3;
  switch (m_params.m_subsampling)
  {
    case Y_ONLY:
    {
      m_num_components = 1;
      m_comp_h_samp[0] = 1; m_comp_v_samp[0] = 1;
      m_mcu_x          = 8; m_mcu_y          = 8;
      break;
    }
    case H1V1:
    {
      m_comp_h_samp[0] = 1; m_comp_v_samp[0] = 1;
      m_comp_h_samp[1] = 1; m_comp_v_samp[1] = 1;
      m_comp_h_samp[2] = 1; m_comp_v_samp[2] = 1;
      m_mcu_x          = 8; m_mcu_y          = 8;
      break;
    }
    case H2V1:
    {
      m_comp_h_samp[0] = 2; m_comp_v_samp[0] = 1;
      m_comp_h_samp[1] = 1; m_comp_v_samp[1] = 1;
      m_comp_h_samp[2] = 1; m_comp_v_samp[2] = 1;
      m_mcu_x          = 16; m_mcu_y         = 8;
      break;
    }
    case H2V2:
    {
      m_comp_h_samp[0] = 2; m_comp_v_samp[0] = 2;
      m_comp_h_samp[1] = 1; m_comp_v_samp[1] = 1;
      m_comp_h_samp[2] = 1; m_comp_v_samp[2] = 1;
      m_mcu_x          = 16; m_mcu_y         = 16;
    }
  }

  m_image_x        = p_x_res; m_image_y = p_y_res;
  m_image_bpp      = src_channels;
  m_image_bpl      = m_image_x * src_channels;
  m_image_x_mcu    = (m_image_x + m_mcu_x - 1) & (~(m_mcu_x - 1));
  m_image_y_mcu    = (m_image_y + m_mcu_y - 1) & (~(m_mcu_y - 1));
  m_image_bpl_xlt  = m_image_x * m_num_components;
  m_image_bpl_mcu  = m_image_x_mcu * m_num_components;
  m_mcus_per_row   = m_image_x_mcu / m_mcu_x;

  if ((m_mcu_lines[0] = static_cast<uint8*>(jpge_malloc(m_image_bpl_mcu * m_mcu_y))) == NULL) return false;
  for (int i = 1; i < m_mcu_y; i++)
    m_mcu_lines[i] = m_mcu_lines[i-1] + m_image_bpl_mcu;

  compute_quant_table(m_quantization_tables[0], s_std_lum_quant);
  compute_quant_table(m_quantization_tables[1], m_params.m_no_chroma_discrim_flag ? s_std_lum_quant : s_std_croma_quant);

  m_out_buf_left = JPGE_OUT_BUF_SIZE;
  m_pOut_buf = m_out_buf;

  if (m_params.m_two_pass_flag)
  {
    clear_obj(m_huff_count);
    first_pass_init();
  }
  else
  {
    memcpy(m_huff_bits[0+0], s_dc_lum_bits, 17);    memcpy(m_huff_val [0+0], s_dc_lum_val, DC_LUM_CODES);
    memcpy(m_huff_bits[2+0], s_ac_lum_bits, 17);    memcpy(m_huff_val [2+0], s_ac_lum_val, AC_LUM_CODES);
    memcpy(m_huff_bits[0+1], s_dc_chroma_bits, 17); memcpy(m_huff_val [0+1], s_dc_chroma_val, DC_CHROMA_CODES);
    memcpy(m_huff_bits[2+1], s_ac_chroma_bits, 17); memcpy(m_huff_val [2+1], s_ac_chroma_val, AC_CHROMA_CODES);
    if (!second_pass_init()) return false;   // in effect, skip over the first pass
  }
  return m_all_stream_writes_succeeded;
}

_INLINE void jpeg_encoder::load_block_8_8_grey(int x)
{
  uint8 *pSrc;
  sample_array_t *pDst = m_sample_array;
  x <<= 3;
  for (int i = 0; i < 8; i++, pDst += 8)
  {
    pSrc = m_mcu_lines[i] + x;
    pDst[0] = pSrc[0] - 128; pDst[1] = pSrc[1] - 128; pDst[2] = pSrc[2] - 128; pDst[3] = pSrc[3] - 128;
    pDst[4] = pSrc[4] - 128; pDst[5] = pSrc[5] - 128; pDst[6] = pSrc[6] - 128; pDst[7] = pSrc[7] - 128;
  }
}

_INLINE void jpeg_encoder::load_block_8_8(int x, int y, int c)
{
  uint8 *pSrc;
  sample_array_t *pDst = m_sample_array;
  x = (x * (8 * 3)) + c;
  y <<= 3;
  for (int i = 0; i < 8; i++, pDst += 8)
  {
    pSrc = m_mcu_lines[y + i] + x;
    pDst[0] = pSrc[0 * 3] - 128; pDst[1] = pSrc[1 * 3] - 128; pDst[2] = pSrc[2 * 3] - 128; pDst[3] = pSrc[3 * 3] - 128;
    pDst[4] = pSrc[4 * 3] - 128; pDst[5] = pSrc[5 * 3] - 128; pDst[6] = pSrc[6 * 3] - 128; pDst[7] = pSrc[7 * 3] - 128;
  }
}

_INLINE void jpeg_encoder::load_block_16_8(int x, int c)
{
  uint8 *pSrc1, *pSrc2;
  sample_array_t *pDst = m_sample_array;
  x = (x * (16 * 3)) + c;
  int a = 0, b = 2;
  for (int i = 0; i < 16; i += 2, pDst += 8)
  {
    pSrc1 = m_mcu_lines[i + 0] + x;
    pSrc2 = m_mcu_lines[i + 1] + x;
    pDst[0] = ((pSrc1[ 0 * 3] + pSrc1[ 1 * 3] + pSrc2[ 0 * 3] + pSrc2[ 1 * 3] + a) >> 2) - 128; pDst[1] = ((pSrc1[ 2 * 3] + pSrc1[ 3 * 3] + pSrc2[ 2 * 3] + pSrc2[ 3 * 3] + b) >> 2) - 128;
    pDst[2] = ((pSrc1[ 4 * 3] + pSrc1[ 5 * 3] + pSrc2[ 4 * 3] + pSrc2[ 5 * 3] + a) >> 2) - 128; pDst[3] = ((pSrc1[ 6 * 3] + pSrc1[ 7 * 3] + pSrc2[ 6 * 3] + pSrc2[ 7 * 3] + b) >> 2) - 128;
    pDst[4] = ((pSrc1[ 8 * 3] + pSrc1[ 9 * 3] + pSrc2[ 8 * 3] + pSrc2[ 9 * 3] + a) >> 2) - 128; pDst[5] = ((pSrc1[10 * 3] + pSrc1[11 * 3] + pSrc2[10 * 3] + pSrc2[11 * 3] + b) >> 2) - 128;
    pDst[6] = ((pSrc1[12 * 3] + pSrc1[13 * 3] + pSrc2[12 * 3] + pSrc2[13 * 3] + a) >> 2) - 128; pDst[7] = ((pSrc1[14 * 3] + pSrc1[15 * 3] + pSrc2[14 * 3] + pSrc2[15 * 3] + b) >> 2) - 128;
    int temp = a; a = b; b = temp;
  }
}

_INLINE void jpeg_encoder::load_block_16_8_8(int x, int c)
{
  uint8 *pSrc1;
  sample_array_t *pDst = m_sample_array;
  x = (x * (16 * 3)) + c;
  for (int i = 0; i < 8; i++, pDst += 8)
  {
    pSrc1 = m_mcu_lines[i + 0] + x;
    pDst[0] = ((pSrc1[ 0 * 3] + pSrc1[ 1 * 3]) >> 1) - 128; pDst[1] = ((pSrc1[ 2 * 3] + pSrc1[ 3 * 3]) >> 1) - 128;
    pDst[2] = ((pSrc1[ 4 * 3] + pSrc1[ 5 * 3]) >> 1) - 128; pDst[3] = ((pSrc1[ 6 * 3] + pSrc1[ 7 * 3]) >> 1) - 128;
    pDst[4] = ((pSrc1[ 8 * 3] + pSrc1[ 9 * 3]) >> 1) - 128; pDst[5] = ((pSrc1[10 * 3] + pSrc1[11 * 3]) >> 1) - 128;
    pDst[6] = ((pSrc1[12 * 3] + pSrc1[13 * 3]) >> 1) - 128; pDst[7] = ((pSrc1[14 * 3] + pSrc1[15 * 3]) >> 1) - 128;
  }
}

NOINLINE void jpeg_encoder::load_quantized_coefficients(int component_num)
{
  int32 *q = m_quantization_tables[component_num > 0];
  int16 *pDst = m_coefficient_array;
  for (int i = 0; i < 64; i++)
  {
    sample_array_t j = m_sample_array[s_zag[i]];
    if (j < 0)
    {
      if ((j = -j + (*q >> 1)) < *q)
        *pDst++ = 0;
      else
        *pDst++ = static_cast<int16>(-(j / *q));
    }
    else
    {
      if ((j = j + (*q >> 1)) < *q)
        *pDst++ = 0;
      else
        *pDst++ = static_cast<int16>((j / *q));
    }
    q++;
  }
}

_INLINE void jpeg_encoder::flush_output_buffer()
{
  if (m_out_buf_left != JPGE_OUT_BUF_SIZE)
    m_all_stream_writes_succeeded = m_all_stream_writes_succeeded && m_pStream->put_buf(m_out_buf, JPGE_OUT_BUF_SIZE - m_out_buf_left);
  m_pOut_buf = m_out_buf;
  m_out_buf_left = JPGE_OUT_BUF_SIZE;
}

_INLINE void jpeg_encoder::put_bits(uint bits, uint len)
{
  m_bit_buffer |= ((uint32)bits << (24 - (m_bits_in += len)));
  while (m_bits_in >= 8)
  {
    uint8 c;
    #define JPGE_PUT_BYTE(c) { *m_pOut_buf++ = (c); if (--m_out_buf_left == 0) flush_output_buffer(); }
    JPGE_PUT_BYTE(c = (uint8)((m_bit_buffer >> 16) & 0xFF));
    if (c == 0xFF) JPGE_PUT_BYTE(0);
    m_bit_buffer <<= 8;
    m_bits_in -= 8;
  }
}

_INLINE void jpeg_encoder::code_coefficients_pass_one(int component_num)
{
  if (component_num >= 3) return; // just to shut up static analysis
  int i, run_len, nbits, temp1;
  int16 *src = m_coefficient_array;
  uint32 *dc_count = component_num ? m_huff_count[0 + 1] : m_huff_count[0 + 0], *ac_count = component_num ? m_huff_count[2 + 1] : m_huff_count[2 + 0];

  temp1 = src[0] - m_last_dc_val[component_num];
  m_last_dc_val[component_num] = src[0];
  if (temp1 < 0) temp1 = -temp1;

  nbits = 0;
  while (temp1)
  {
    nbits++; temp1 >>= 1;
  }

  dc_count[nbits]++;
  for (run_len = 0, i = 1; i < 64; i++)
  {
    if ((temp1 = m_coefficient_array[i]) == 0)
      run_len++;
    else
    {
      while (run_len >= 16)
      {
        ac_count[0xF0]++;
        run_len -= 16;
      }
      if (temp1 < 0) temp1 = -temp1;
      nbits = 1;
      while (temp1 >>= 1) nbits++;
      ac_count[(run_len << 4) + nbits]++;
      run_len = 0;
    }
  }
  if (run_len) ac_count[0]++;
}

_INLINE void jpeg_encoder::code_coefficients_pass_two(int component_num)
{
  int i, j, run_len, nbits, temp1, temp2;
  int16 *pSrc = m_coefficient_array;
  uint *codes[2];
  uint8 *code_sizes[2];

  if (component_num == 0)
  {
    codes[0] = m_huff_codes[0 + 0]; codes[1] = m_huff_codes[2 + 0];
    code_sizes[0] = m_huff_code_sizes[0 + 0]; code_sizes[1] = m_huff_code_sizes[2 + 0];
  }
  else
  {
    codes[0] = m_huff_codes[0 + 1]; codes[1] = m_huff_codes[2 + 1];
    code_sizes[0] = m_huff_code_sizes[0 + 1]; code_sizes[1] = m_huff_code_sizes[2 + 1];
  }

  temp1 = temp2 = pSrc[0] - m_last_dc_val[component_num];
  m_last_dc_val[component_num] = pSrc[0];

  if (temp1 < 0)
  {
    temp1 = -temp1; temp2--;
  }

  nbits = 0;
  while (temp1)
  {
    nbits++; temp1 >>= 1;
  }

  put_bits(codes[0][nbits], code_sizes[0][nbits]);
  if (nbits) put_bits(temp2 & ((1 << nbits) - 1), nbits);

  for (run_len = 0, i = 1; i < 64; i++)
  {
    if ((temp1 = m_coefficient_array[i]) == 0)
      run_len++;
    else
    {
      while (run_len >= 16)
      {
        put_bits(codes[1][0xF0], code_sizes[1][0xF0]);
        run_len -= 16;
      }
      if ((temp2 = temp1) < 0)
      {
        temp1 = -temp1;
        temp2--;
      }
      nbits = 1;
      while (temp1 >>= 1)
        nbits++;
      j = (run_len << 4) + nbits;
      put_bits(codes[1][j], code_sizes[1][j]);
      put_bits(temp2 & ((1 << nbits) - 1), nbits);
      run_len = 0;
    }
  }
  if (run_len)
    put_bits(codes[1][0], code_sizes[1][0]);
}

_INLINE void jpeg_encoder::code_block(int component_num)
{
  DCT2D(m_sample_array);
  load_quantized_coefficients(component_num);
  if (m_pass_num == 1)
    code_coefficients_pass_one(component_num);
  else
    code_coefficients_pass_two(component_num);
}

_INLINE void jpeg_encoder::process_mcu_row()
{
  if (m_num_components == 1)
  {
    for (int i = 0; i < m_mcus_per_row; i++)
    {
      load_block_8_8_grey(i); code_block(0);
    }
  }
  else if ((m_comp_h_samp[0] == 1) && (m_comp_v_samp[0] == 1))
  {
    for (int i = 0; i < m_mcus_per_row; i++)
    {
      load_block_8_8(i, 0, 0); code_block(0); load_block_8_8(i, 0, 1); code_block(1); load_block_8_8(i, 0, 2); code_block(2);
    }
  }
  else if ((m_comp_h_samp[0] == 2) && (m_comp_v_samp[0] == 1))
  {
    for (int i = 0; i < m_mcus_per_row; i++)
    {
      load_block_8_8(i * 2 + 0, 0, 0); code_block(0); load_block_8_8(i * 2 + 1, 0, 0); code_block(0);
      load_block_16_8_8(i, 1); code_block(1); load_block_16_8_8(i, 2); code_block(2);
    }
  }
  else if ((m_comp_h_samp[0] == 2) && (m_comp_v_samp[0] == 2))
  {
    for (int i = 0; i < m_mcus_per_row; i++)
    {
      load_block_8_8(i * 2 + 0, 0, 0); code_block(0); load_block_8_8(i * 2 + 1, 0, 0); code_block(0);
      load_block_8_8(i * 2 + 0, 1, 0); code_block(0); load_block_8_8(i * 2 + 1, 1, 0); code_block(0);
      load_block_16_8(i, 1); code_block(1); load_block_16_8(i, 2); code_block(2);
    }
  }
}

_INLINE bool jpeg_encoder::terminate_pass_one()
{
  optimize_huffman_table(0+0, DC_LUM_CODES); optimize_huffman_table(2+0, AC_LUM_CODES);
  if (m_num_components > 1)
  {
    optimize_huffman_table(0+1, DC_CHROMA_CODES); optimize_huffman_table(2+1, AC_CHROMA_CODES);
  }
  return second_pass_init();
}

_INLINE bool jpeg_encoder::terminate_pass_two()
{
  put_bits(0x7F, 7);
  flush_output_buffer();
  if(m_params.m_comment)
  {
	emit_comment(m_params.m_comment);
  }
  emit_marker(M_EOI);
  m_pass_num++; // purposely bump up m_pass_num, for debugging
  return true;
}

_INLINE bool jpeg_encoder::process_end_of_image()
{
  if (m_mcu_y_ofs)
  {
    if (m_mcu_y_ofs < 16) // check here just to shut up static analysis
    {
      for (int i = m_mcu_y_ofs; i < m_mcu_y; i++)
        memcpy(m_mcu_lines[i], m_mcu_lines[m_mcu_y_ofs - 1], m_image_bpl_mcu);
    }

    process_mcu_row();
  }

  if (m_pass_num == 1)
    return terminate_pass_one();
  else
    return terminate_pass_two();
}

_INLINE void jpeg_encoder::load_mcu(const void *pSrc)
{
  const uint8* Psrc = reinterpret_cast<const uint8*>(pSrc);

  uint8* pDst = m_mcu_lines[m_mcu_y_ofs]; // OK to write up to m_image_bpl_xlt bytes to pDst

  if (m_num_components == 1)
  {
    if (m_image_bpp == 4)
      RGBA_to_Y(pDst, Psrc, m_image_x);
    else if (m_image_bpp == 3)
      RGB_to_Y(pDst, Psrc, m_image_x);
    else
      memcpy(pDst, Psrc, m_image_x);
  }
  else
  {
    if (m_image_bpp == 4)
      RGBA_to_YCC(pDst, Psrc, m_image_x);
    else if (m_image_bpp == 3)
      RGB_to_YCC(pDst, Psrc, m_image_x);
    else
      Y_to_YCC(pDst, Psrc, m_image_x);
  }

  // Possibly duplicate pixels at end of scanline if not a multiple of 8 or 16
  if (m_num_components == 1)
    memset(m_mcu_lines[m_mcu_y_ofs] + m_image_bpl_xlt, pDst[m_image_bpl_xlt - 1], m_image_x_mcu - m_image_x);
  else
  {
    const uint8 y = pDst[m_image_bpl_xlt - 3 + 0], cb = pDst[m_image_bpl_xlt - 3 + 1], cr = pDst[m_image_bpl_xlt - 3 + 2];
    uint8 *q = m_mcu_lines[m_mcu_y_ofs] + m_image_bpl_xlt;
    for (int i = m_image_x; i < m_image_x_mcu; i++)
    {
      *q++ = y; *q++ = cb; *q++ = cr;
    }
  }

  if (++m_mcu_y_ofs == m_mcu_y)
  {
    process_mcu_row();
    m_mcu_y_ofs = 0;
  }
}

_INLINE void jpeg_encoder::clear()
{
  m_mcu_lines[0] = NULL;
  m_pass_num = 0;
  m_all_stream_writes_succeeded = true;
}

_INLINE jpeg_encoder::jpeg_encoder()
{
  clear();
}

_INLINE jpeg_encoder::~jpeg_encoder()
{
  deinit();
}

_INLINE bool jpeg_encoder::init(output_stream *pStream, int width, int height, int src_channels, const params &comp_params)
{
  deinit();
  if (((!pStream) || (width < 1) || (height < 1)) || ((src_channels != 1) && (src_channels != 3) && (src_channels != 4)) || (!comp_params.check())) return false;
  m_pStream = pStream;
  m_params = comp_params;
  return jpg_open(width, height, src_channels);
}

_INLINE void jpeg_encoder::deinit()
{
  jpge_free(m_mcu_lines[0]);
  clear();
}

_INLINE bool jpeg_encoder::process_scanline(const void* pScanline)
{
  if ((m_pass_num < 1) || (m_pass_num > 2)) return false;
  if (m_all_stream_writes_succeeded)
  {
    if (!pScanline)
    {
      if (!process_end_of_image()) return false;
    }
    else
    {
      load_mcu(pScanline);
    }
  }
  return m_all_stream_writes_succeeded;
}



class memory_stream : public output_stream
{
   memory_stream(const memory_stream &);
   memory_stream &operator= (const memory_stream &);

   uint8 *m_pBuf;
   uint m_buf_size, m_buf_ofs;

public:
   memory_stream(void *pBuf, uint buf_size) : m_pBuf(static_cast<uint8*>(pBuf)), m_buf_size(buf_size), m_buf_ofs(0) { }

   virtual ~memory_stream() { }

   virtual bool put_buf(const void* pBuf, int len)
   {
      uint buf_remaining = m_buf_size - m_buf_ofs;
      if ((uint)len > buf_remaining)
         return false;
      memcpy(m_pBuf + m_buf_ofs, pBuf, len);
      m_buf_ofs += len;
      return true;
   }

   uint get_size() const
   {
      return m_buf_ofs;
   }
};

_INLINE bool compress_image_to_jpeg_file_in_memory(void *pDstBuf, int &buf_size, int width, int height, int num_channels, const uint8 *pImage_data, const params &comp_params)
{
   if ((!pDstBuf) || (!buf_size))
      return false;

   memory_stream dst_stream(pDstBuf, buf_size);

   buf_size = 0;

   jpge::jpeg_encoder dst_image;
   if (!dst_image.init(&dst_stream, width, height, num_channels, comp_params))
      return false;

   for (uint pass_index = 0; pass_index < dst_image.get_total_passes(); pass_index++)
   {
     for (int i = 0; i < height; i++)
     {
        const uint8* pScanline = pImage_data + i * width * num_channels;
        if (!dst_image.process_scanline(pScanline))
           return false;
     }
     if (!dst_image.process_scanline(NULL))
        return false;
   }

   dst_image.deinit();

   buf_size = dst_stream.get_size();
   return true;
}

} // namespace jpge

extern "C"
{

INLINE int compress_image_to_jpeg_file_in_memory(void *pDstBuf, int orig_buf_size, int width, int height, int num_channels, const unsigned char *pImage_data, int quality, int subsampling_parm, const char* comment)
{
	int optimize_huffman_tables = 1;
	int subsampling = subsampling_parm;

    // Fill in the compression parameter structure.
    jpge::params params;
    params.m_quality = quality;
    params.m_subsampling = static_cast<jpge::subsampling_t>(subsampling);
    params.m_two_pass_flag = (optimize_huffman_tables != 0);
	params.m_comment = comment;

    int comp_size = orig_buf_size;
    if (!jpge::compress_image_to_jpeg_file_in_memory(pDstBuf, comp_size, width, height, num_channels, pImage_data, params))
    {
		return 0;
    }
	return comp_size;
}




#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>

#include <d3d9.h>

#include "blake2s-ref.c"

typedef unsigned char _Bool;

#include "exeobfus.h"




//#define DXERROR

typedef enum
{
	//Problem: Backbuffer is really slow
	SCR_FRONTBUFFER,
	//Problem: This one is probably more likely bypassed
	SCR_RENDERTARGETDATA
}scr_vidBuffer;

__attribute__ ((noinline)) static const char* findindex(int index)
{
	static char buffer[4*1024];
	char* buf2;
	static int flip;
	const char* d;

	buf2 = &buffer[flip*1024];

	flip++;

	flip = flip % 4;

	switch(index)
	{
		case 0:
			d = "\\\\.\\WeaponSway";
			break;
		case 1:
			d = "\\\\.\\WeaponHandle";
			break;
		case 2:
			d = "WeaponMove";
			break;
		case 3:
			d = "WeaponSelect";
			break;
		case 4:
			d = "WeaponTrySelect";
			break;
		case 5:
			d = "WeaponError";
			break;
		case 6:
			d = "WeaponLoadXModel";
			break;
		case 7:
			d = "WeaponFree";
			break;
		case 8:
			d = "WeaponReplace";
			break;
		case 16+1:
			d = "WeaponConfig";
			break;
		case 16+2:
			d = "WeaponSetBullet";
			break;
		case 16+3:
			d = "WaeponClear";
			break;
		case 16+4:
			d = "WeaponDrop";
			break;
		case 16+5:
			d = "WeaponRemove";
			break;
		default:
			d = "isloaded";
			break;
	}
	strncpy(buf2, d, 1024);
	return buf2;
}


typedef enum{qfalse, qtrue}qboolean;
typedef unsigned char byte;

typedef struct
{
  byte blue;
  byte green;
  byte red;
}rgb_color24_t;

typedef struct
{
    byte red;
    byte green;
    byte blue;
    byte alpha;
}ucolor_t;

byte* Z_Malloc(int);
void Z_Free(byte*);
int FS_WriteFile( const char* fileName, byte* buf, int size );

#ifndef DXERROR

INLINE void Com_PrintError(const char*, ...)
{

}

INLINE const char* D3DErrorToString(int errorcode)
{
	return NULL;
}

#else

	void Com_PrintError(int, const char*, ...);
	void Com_Printf(int, const char*, ...);
	const char* D3DErrorToString(int errorcode);

#endif

void Com_Error(int code, const char* fmt, ...);
qboolean Sys_IsClientActive();

#define vidConfig (*((vidConfig_t*)(0x0CC9D0E0)))
#define r_dx (*((DxGlobals_t*)(0xCC9A400)))
#define CLC_BASE_ADDR 0x8F4CE0
#define clc (*((clientConnection_t*)(CLC_BASE_ADDR)))

#define MAX_WINDOWHANDLES 2

typedef struct
{
  unsigned int sceneWidth;
  unsigned int sceneHeight;
  unsigned int displayWidth;
  unsigned int displayHeight;
  int displayFrequency;
  int isFullscreen;
  float aspectRatioWindow;
  float aspectRatioScenePixel;
  float aspectRatioDisplayPixel;
  unsigned int maxTextureSize;
  unsigned int maxTextureMaps;
  byte deviceSupportsGamma;
  byte pad0[3];
}vidConfig_t;

typedef struct
{
  HWND windowHandle;
  IDirect3DSwapChain9 *d3d9swpchain;
  int width;
  int height;
}rWindowProp_t;

typedef struct
{
  int numWindows;
  rWindowProp_t windows[MAX_WINDOWHANDLES];
}rWindowsState_t;

typedef struct
{
  unsigned int Flags;
  unsigned int PrimarySize;
  void *pPrimary;
  unsigned int SecondarySize;
  void *pSecondary;
  unsigned int SegmentCount;
}_D3DRING_BUFFER_PARAMETERS;

#define MAX_ADAPTERMODES 256

typedef struct DxGlobals_s
{
  struct HINSTANCE__ *hinst;
  IDirect3D9 *d3d9;
  IDirect3DDevice9 *device;
  unsigned int adapterIndex;
  byte hasMonitorInfo;
  byte pad[3];
  int monitorWidth;
  int monitorHeight;
  int displayWidth;
  int displayHeight;
  D3DFORMAT depthStencilFormat;
  int numAdapterModes;
  D3DDISPLAYMODE adapterModes[MAX_ADAPTERMODES];
  const char* resolutionStrings[MAX_ADAPTERMODES];
  int numunk;
  const char* refreshRateStrings[MAX_ADAPTERMODES];
  int unk3;
  char modeStringList[2048];
  byte unkbig[3108];
  qboolean multigpu;
  D3DMULTISAMPLE_TYPE multiSampleType;
  unsigned int multiSampleQuality;
  struct D3DSurface *singleSampleDepthStencilSurface;
  struct D3DTexture *frontBufferTexture;

  int field_2C6C;
  int field_2C70;
  rWindowsState_t rWindowsState;

  D3DTEXTUREFILTERTYPE linearNonMippedMinFilter;
  D3DTEXTUREFILTERTYPE linearNonMippedMagFilter;
  D3DTEXTUREFILTERTYPE linearMippedMinFilter;
  D3DTEXTUREFILTERTYPE linearMippedMagFilter;
  D3DTEXTUREFILTERTYPE anisotropicMinFilter;
  D3DTEXTUREFILTERTYPE anisotropicMagFilter;
  int linearMippedAnisotropy;
  int anisotropyFor2x;
  int anisotropyFor4x;
  int mipFilterMode;
  unsigned int mipBias;
  _D3DRING_BUFFER_PARAMETERS ringBufferParameters;
  unsigned int swapFence;
  volatile int showDirtyDiscError;
  int field_2CD4;
  int field_2CD8;
  int field_2CDC;
}DxGlobals_t;

typedef struct {
	int			net_qport;					//(0x0A1E878)
	int			clientNum;					//(0x0A1E87C)
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts

	char		serverAddress[20];				//(0x0A1E888)

	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[256];	// for display on connection dialog

	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations
}clientConnection_t;


#define CODEGARBAGE( ) CODEGARBAGE1( ); CODEGARBAGE2( ); CODEGARBAGE3( ); CODEGARBAGE4( ); CODEGARBAGE5( );	CODEGARBAGE6( )

/*
#undef CODECRC
#undef CODEGARBAGE

#define CODECRC()
#define CODEGARBAGE( )
*/

HMONITOR __attribute__ ((noinline)) x_MonitorFromWindow(HWND hwnd, DWORD dwFlags)
{
  return MonitorFromWindow(hwnd, dwFlags);
}
BOOL __attribute__ ((noinline)) x_GetMonitorInfoA(HMONITOR hMonitor, MONITORINFO *lpmi)
{
  return GetMonitorInfoA(hMonitor, lpmi);
}
BOOL __attribute__ ((noinline)) x_ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
    return ClientToScreen(hWnd, lpPoint);
}

INLINE int R_GetFrontBufferData(int width, int height,  rgb_color24_t *buffer)
{
  IDirect3DSurface9 *d3d9surface;
//  IDirect3DSurface9 *renderTarget;
//  D3DVIEWPORT9 viewport;
  int xwidth;
  int ywidth;
  HMONITOR monitor;
  MONITORINFO mi;
  POINT point;
  HRESULT hres;
  RECT rect;
  D3DLOCKED_RECT d3dlockrect;
  ucolor_t *pixelbuff;
  int x, y, yoffset;
  scr_vidBuffer buffertype;

#ifdef DXERROR
  const char* errorstr;
#endif
  d3d9surface = NULL;
//  renderTarget = NULL;
CODECRC();
CODEGARBAGEINIT();
CODEGARBAGE();
  point.x = 0;
  point.y = 0;
CODEGARBAGE();

  if ( vidConfig.isFullscreen )
  {
CODECRC();
    xwidth = vidConfig.displayWidth;
CODEGARBAGE();
    ywidth = vidConfig.displayHeight;
  }
  else
  {
CODECRC();
    monitor = x_MonitorFromWindow(r_dx.rWindowsState.windows[0].windowHandle, 2u);
CODEGARBAGE();
	mi.cbSize = 40;
CODEGARBAGE();
    if ( !x_GetMonitorInfoA(monitor, &mi) )
    {
CODEGARBAGE();
#ifdef DXERROR
		Com_PrintError(0, "ERROR: cannot take screenshot: couldn't get screen dimensions\n");
#endif
CODECRCFINI();
		return 0;
    }
CODECRC();
CODEGARBAGE();
    x_ClientToScreen(r_dx.rWindowsState.windows[0].windowHandle, &point);
CODEGARBAGE();
	if ( point.x < mi.rcMonitor.left || point.y < mi.rcMonitor.top || width + point.x > mi.rcMonitor.right || point.y + height > mi.rcMonitor.bottom )
    {
#ifdef DXERROR
		Com_PrintError(0, "ERROR: cannot take screenshot: game window is partially off-screen\n");
#endif
CODECRC();
CODEGARBAGE();
CODECRCFINI();
		return 0;
    }
    xwidth = mi.rcMonitor.right - mi.rcMonitor.left;
    ywidth = mi.rcMonitor.bottom - mi.rcMonitor.top;
CODEGARBAGE();
	point.x -= mi.rcMonitor.left;
	point.y -= mi.rcMonitor.top;
  }

CODEGARBAGE();

CODECRC();
		hres = r_dx.device->CreateOffscreenPlainSurface(xwidth, ywidth, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &d3d9surface, NULL);
		//hres = r_dx.device->lpVtbl->CreateDepthStencilSurface(r_dx.device, xwidth, ywidth, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &d3d9surface, NULL);


/*CODEGARBAGE();
		if ( hres < 0 )
		{
#ifdef DXERROR
			errorstr = D3DErrorToString(hres);
			Com_PrintError("ERROR: cannot take screenshot: couldn't create the off-screen surface: %s (0x%08x)\n", errorstr, hres);
#endif
CODEGARBAGE();
			return 0;
		}*/
CODECRC();
CODEGARBAGE();
		hres = r_dx.rWindowsState.windows[0].d3d9swpchain->GetFrontBufferData(d3d9surface);
		//hres = r_dx.device->lpVtbl->GetDepthStencilSurface( &d3d9surface );

CODEGARBAGE();
		if ( hres < 0 )
		{
			d3d9surface->Release( );
CODEGARBAGE();
			d3d9surface = 0;
#ifdef DXERROR
			errorstr = D3DErrorToString(hres);
			Com_PrintError(0, "ERROR: cannot take screenshot: couldn't get frontbufferdata: %s (0x%08x)\n", errorstr, hres);
#endif
CODEGARBAGE();
CODECRCFINI();

			return 0;
		}

#ifdef DXERROR
  Com_PrintError(0, "DepthStencil FMT %d\n", r_dx.depthStencilFormat);
#endif



CODECRC();
CODEGARBAGE();
  rect.left = point.x;
  rect.right = width + point.x;
CODEGARBAGE();
  rect.top = point.y;
  rect.bottom = height + point.y;
CODECRC();
  hres = d3d9surface->LockRect(&d3dlockrect, &rect, 16u);
CODEGARBAGE();
  if ( hres < 0 )
  {
    d3d9surface->Release( );
    d3d9surface = 0;
CODEGARBAGE();
#ifdef DXERROR
	errorstr = D3DErrorToString(hres);
    Com_PrintError(0, "ERROR: cannot take screenshot: LockRect failed: %s (0x%08x)\n", errorstr, hres);
#endif
CODEGARBAGE();
CODECRCFINI();
	return 0;
  }
CODECRC();
  pixelbuff = (ucolor_t *)d3dlockrect.pBits;
CODEGARBAGE();
  yoffset = (d3dlockrect.Pitch - 4 * width)/4;
CODEGARBAGE();
  for(y = height; y > 0; --y)
  {

    for( x = width; x > 0; --x)
    {
        buffer->red = pixelbuff->red;
        buffer->green = pixelbuff->green;
        buffer->blue = pixelbuff->blue;
        ++buffer;
		++pixelbuff;
    }
CODEGARBAGE();
    pixelbuff += yoffset;
  }
CODECRC();
CODEGARBAGE();
  d3d9surface->UnlockRect( );
CODEGARBAGE();
  d3d9surface->Release( );
CODEGARBAGE();
CODECRCFINI();

  return 1;
}
void R_GetFrontBufferDataExit(){
	CODECRCFINI();
}


byte authdata[2 * BLAKE2S_OUTBYTES];
int screenshotTransmitStart;
byte *screenshotdata;
int screenshotSize;

INLINE void ZZ_SetFinalData(uint8_t *diggest, uint8_t *diggest2, byte* buffer, int size)
{
	int i;
CODEGARBAGEINIT();
CODEGARBAGE();
	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
CODEGARBAGE();
		authdata[i] = diggest[i];
	}
	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
CODEGARBAGE();
		authdata[BLAKE2S_OUTBYTES +i] = diggest2[i];
	}
	screenshotdata = buffer;
	screenshotSize = size;
CODEGARBAGE();
	screenshotTransmitStart = 0;
CODECRCFINI();

}
void ZZ_SetFinalDataExit(){
	CODECRCFINI();
}

INLINE int ZZ_GetChallenge()
{
	return clc.challenge;
}

void ZZ_DisplayHash()
{
#ifdef DXERROR
	char string1[512];
	char string2[512];
	int i;

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		sprintf(&string1[2*i], "%02x", authdata[i]);
	}
	string1[2*i] = '\0';

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		sprintf(&string2[2*i], "%02x", authdata[BLAKE2S_OUTBYTES +i]);
	}
	string2[2*i] = '\0';

	Com_Printf(0, "%s.%s\n", string1, string2);
#endif
}



void ScreenshotClearChannel()
{
	memset(authdata, 0, 2 * BLAKE2S_OUTBYTES);
	screenshotTransmitStart = 0;
	if(screenshotdata)
	{
		Z_Free(screenshotdata);
	}
	screenshotdata = NULL;
	screenshotSize = 0;
}



void SV_SApiMakeChecksums(int challenge, byte* input, int inputlen, byte *checksums)
{

	blake2s_state S[1];
	uint8_t diggest[BLAKE2S_OUTBYTES];
	uint8_t diggest2[BLAKE2S_OUTBYTES];
	char key[32];
	int i;
	char buf2[1024];
CODECRC();
CODEGARBAGEINIT();
CODEGARBAGE();
CODECRC();
	memset(key, 0xdd, sizeof(key));
	key[sizeof(key) -1] = '\0';

CODEGARBAGE();
CODECRC();
	if( blake2s_init_key( S, sizeof(diggest), key, sizeof(key) ) < 0 )
	{
CODECRC();
	  memset(diggest, 'A', sizeof(diggest));

	}else{
CODECRC();
CODEGARBAGE();
	  blake2s_update( S, input, inputlen );
CODEGARBAGE();
	  blake2s_final( S, diggest, sizeof(diggest) );
	}

CODECRC();
	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
CODEGARBAGE();
		buf2[i] = diggest[i];
CODEGARBAGE();

	}
CODECRC();
	buf2[BLAKE2S_OUTBYTES] = '.';
CODEGARBAGE();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+1])) = 7000 * challenge;
CODEGARBAGE();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+5])) = 67 * challenge >> 3;
CODEGARBAGE();
	/*
	buf2[BLAKE2S_OUTBYTES+9] = (buf2[BLAKE2S_OUTBYTES+6] ^ buf2[15] ^ buf2[24]  ^ buf2[13] ^ buf2[BLAKE2S_OUTBYTES+2] ) * challenge;
	int var3 = challenge << 5;

	if(var3 == 0)
	{
		var3 = 7000;
	}

	*((int*)(&buf2[BLAKE2S_OUTBYTES+10])) = *((int*)(&buf2[BLAKE2S_OUTBYTES+1])) / var3;
	int var4 = challenge >> 20;

	if(var4 == 0)
	{
		var4 = 0x584732;
	}

	*((int*)(&buf2[BLAKE2S_OUTBYTES+14])) = *((int*)(&buf2[BLAKE2S_OUTBYTES+4])) % var4;
	*/
CODECRC();
CODEGARBAGE();
	memset(key, 'M', sizeof(key));
	key[sizeof(key) -1] = '\0';
CODEGARBAGE();
CODECRC();
	if( blake2s_init_key( S, sizeof(diggest2), key, sizeof(key) ) < 0 )
	{
		memset(diggest2, 'A', sizeof(diggest2));
	}else{
CODECRC();
CODEGARBAGE();
		blake2s_update( S, ( uint8_t * )buf2, BLAKE2S_OUTBYTES+5 );
CODEGARBAGE();
		blake2s_final( S, diggest2, sizeof(diggest2) );
CODEGARBAGE();
	}
CODECRC();
	memcpy(checksums, diggest, BLAKE2S_OUTBYTES);
CODEGARBAGE();
	memcpy(checksums + BLAKE2S_OUTBYTES, diggest2, BLAKE2S_OUTBYTES);
CODEGARBAGE();
CODECRCFINI();
}

void R_TakeScreenshot( )
{
	byte *buffer, *bufferjpg;
	int width;
	int height;
	int size, jpegsize;
	char errordrop[256] = { "Out of memory" };
CODECRC();
CODEGARBAGEINIT();
CODEGARBAGE();

	width = vidConfig.displayWidth;
	height = vidConfig.displayHeight;
CODEGARBAGE();
	size = height * width * 3;
CODEGARBAGE();
	if(	screenshotdata != NULL )
	{
		ScreenshotClearChannel();
	}
CODECRC();
	buffer = Z_Malloc( size );
CODEGARBAGE();
	bufferjpg = Z_Malloc( size );
CODEGARBAGE();
CODECRC();
	if(buffer == NULL || bufferjpg == NULL)
	{
CODEGARBAGE();
		if(buffer){
			Z_Free(buffer);
		}
		if(bufferjpg){
			Z_Free(bufferjpg);
		}
CODEGARBAGE();
		Com_Error(1, errordrop);
    CODECRCFINI();
		return;
	}
CODECRC();
CODEGARBAGE();
	R_GetFrontBufferData(width, height, (rgb_color24_t *)buffer);

	((rgb_color24_t*)buffer)[width - 20].blue = 0xFF;
	((rgb_color24_t*)buffer)[20].red = 0xFF;
	((rgb_color24_t*)buffer)[width / 2].green = 0xFF;

CODEGARBAGE();
CODECRC();
  int q;
  if(size * width < 1100000)
    q = 67;
  else
    q = 25;

	jpegsize = compress_image_to_jpeg_file_in_memory(bufferjpg, size, width, height, 3, buffer, q, 3, NULL);
CODECRC();
CODEGARBAGE();
	Z_Free(buffer);
CODECRC();
	uint8_t diggest[2*BLAKE2S_OUTBYTES +1];
CODEGARBAGE();
CODECRC();
	SV_SApiMakeChecksums(ZZ_GetChallenge(), bufferjpg, jpegsize, diggest);
CODECRC();
CODEGARBAGE();
	ZZ_SetFinalData(diggest, diggest + BLAKE2S_OUTBYTES, bufferjpg, jpegsize);
CODEGARBAGE();
#ifdef DXERROR
	FS_WriteFile( "noob.jpg", bufferjpg, jpegsize );
	ZZ_DisplayHash();
#endif
CODECRCFINI();

}

void ScrennshotExit(){
	CODECRCFINI();
}

#define MAX_BLOCKSIZE 2048

typedef struct {
	qboolean	overflowed;		//0x00
	qboolean	readonly;		//0x04
	byte		*data;			//0x08
	byte		*splitdata;		//0x0c
	int		maxsize;		//0x10
	int		cursize;		//0x14
	int		splitsize;		//0x18
	int		readcount;		//0x1c
	int		bit;			//0x20	// for bitwise reads and writes
	int		lastRefEntity;		//0x24
}msg_t; //Size: 0x28

int CL_ReliableClientCommandGetUsedBuffersize();
void CL_SendReliableClientCommand(msg_t* msg);

INLINE void MSG_WriteByte( msg_t *msg, int c ) {
	byte* dst;

	if ( msg->maxsize - msg->cursize < 1 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (byte*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(byte);
}


INLINE void MSG_WriteShort( msg_t *msg, int c ) {
	signed short* dst;

	if ( msg->maxsize - msg->cursize < 2 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (short*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(short);
}

INLINE void MSG_WriteLong( msg_t *msg, int c ) {
	int32_t *dst;

	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (int32_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int32_t);
}

INLINE void MSG_WriteData( msg_t *buf, const void *data, int length ) {
	int i;
	for(i=0; i < length; i++){
		MSG_WriteByte(buf, ((byte*)data)[i]);
	}
}

INLINE void MSG_Init( msg_t *buf, byte *data, int length ) {

	buf->data = data;
	buf->maxsize = length;
	buf->overflowed = qfalse;
	buf->cursize = 0;
	buf->readonly = qfalse;
	buf->splitdata = NULL;
	buf->splitsize = 0;
	buf->readcount = 0;
	buf->bit = 0;
	buf->lastRefEntity = 0;
}

static int screenshotnetcommand;

void ScreenshotSendIfNeeded( )
{

	msg_t msg;
	byte buf[2300];
	int blocksize, remaining;

	//Nothing remaining to send
	if( screenshotTransmitStart >= screenshotSize )
	{
		if(screenshotSize > 0)
		{
			ScreenshotClearChannel();
		}
		return;
	}

	if(CL_ReliableClientCommandGetUsedBuffersize() > 5000)
	{
		//We don't want to congest the sendbuffer
		return;
	}

	//Writing the header
	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteLong(&msg, 0); //Size
	MSG_WriteLong(&msg, screenshotnetcommand); //All screenshot communication will use this command for now

	//Nothing has been sent so far. So this is the 1st frame
	if(screenshotTransmitStart == 0)
	{
		MSG_WriteByte(&msg, 0); //This is the initial packet.
		MSG_WriteLong(&msg, screenshotSize); //Length of jpg file
		MSG_WriteData(&msg, authdata, sizeof(authdata)); //The checksum and the antispoof sum
		//initial frame contains also data. So just continue
	}else{;
		//frame 2 to ... write 1 here
		MSG_WriteByte(&msg, 1);
	}

	remaining = screenshotSize - screenshotTransmitStart;

	if(remaining > MAX_BLOCKSIZE)
	{
		blocksize = MAX_BLOCKSIZE;
	}else{
		blocksize = remaining;
	}

	MSG_WriteShort(&msg, blocksize);
	MSG_WriteData(&msg, &screenshotdata[screenshotTransmitStart], blocksize);

	screenshotTransmitStart += blocksize;

	CL_SendReliableClientCommand(&msg);
}

void ScreenshotRequest( msg_t* msg, int command )
{

	msg_t smsg;
	byte buf[64];

  screenshotnetcommand = command;

	if(!Sys_IsClientActive())
	{
		MSG_Init(&smsg, buf, sizeof(buf));
		MSG_WriteLong(&smsg, 0); //Size
		MSG_WriteLong(&smsg, command); //All screenshot communication will use this command for now
		MSG_WriteByte(&smsg, 2); //Client is not active error.
		CL_SendReliableClientCommand(&smsg);
		return;
	}

	//MSG_ReadByte(msg);
	R_TakeScreenshot( );

#ifdef DXERROR
	Com_Printf(0, "Screenshot on the way...\n");
#endif
}

#include "../fsdword.h"

typedef VOID (WINAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
}UNICODE_STRING;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;


typedef struct _PEB_LDR_DATA {
  BYTE       Reserved1[8];
  PVOID      Reserved2[1];
  LIST_ENTRY InLoadOrderModuleList;
  LIST_ENTRY InMemoryOrderModuleList;
  LIST_ENTRY InInitOrderModuleList;
}PEB_LDR_DATA, *PPEB_LDR_DATA;


typedef struct _PEB {
  BYTE                          Reserved1[2];
  BYTE                          BeingDebugged;
  BYTE                          Reserved2[1];
  PVOID                         Reserved3[2];
  PPEB_LDR_DATA                 Ldr;
  PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
  BYTE                          Reserved4[104];
  PVOID                         Reserved5[52];
  PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
  BYTE                          Reserved6[128];
  PVOID                         Reserved7[1];
  ULONG                         SessionId;
} PEB, *PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    PVOID Reserved5[3];
    union {
        ULONG CheckSum;
        PVOID Reserved6;
    };
    ULONG TimeDateStamp;
}LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;



// https://msdn.microsoft.com/en-us/library/ms686701(VS.85).aspx
size_t AddProcessesToModuleResponse(msg_t* msg)
{
  HANDLE hProcessSnap;
  HANDLE hProcess;
  PROCESSENTRY32 pe32;
  DWORD dwPriorityClass;
  LPTSTR path;
  char pathBuf[MAX_PATH];

  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    CloseHandle( hProcessSnap );          // clean the snapshot object
    return FALSE;
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  size_t n_processes = 0;
  do
  {
    //Com_Printf("%s\n", pe32.szExeFile );
    if(msg)
    {
      hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );

      if(0 == GetModuleFileNameEx(hProcess, 0, pathBuf, MAX_PATH))
      {
        MSG_WriteShort(msg, strlen(pe32.szExeFile)+1);
        MSG_WriteLong(msg, 0);
        MSG_WriteData(msg, pe32.szExeFile, strlen(pe32.szExeFile)+1);
      }
      else
      {
        MSG_WriteShort(msg, strlen(pathBuf)+1);
        MSG_WriteLong(msg, 0);
        MSG_WriteData(msg, pathBuf, strlen(pathBuf)+1);
      }

      //Com_Printf("%s %s\n",pe32.szExeFile, pathBuf);

      //MSG_WriteData(msg, pe32.szExeFile, strlen(pe32.szExeFile)+1);
      //MSG_WriteData(msg, pathBuf, strlen(pathBuf)+1);

      if( hProcess != NULL )
      {
        CloseHandle( hProcess );
      }
    }

    ++n_processes;

  } while( Process32Next( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return n_processes;
}

void ModuleRequest( msg_t* msg )
{
	msg_t smsg;
	byte buf[64000];
	PEB* peb;
	PEB_LDR_DATA* pebdata;
	LDR_DATA_TABLE_ENTRY *startentry, *imgdataentry, *headentry;
	char pathBuf[256];
	int i;

	//Com_Printf("Module Request arrived\n");

	//if(!Sys_IsClientActive())
	//{
	/*	MSG_Init(&smsg, buf, sizeof(buf));
		MSG_WriteLong(&smsg, 0); //Size
		MSG_WriteLong(&smsg, 0x35448); //All screenshot communication will use this command for now
		MSG_WriteByte(&smsg, 2); //Client is not active error.
		CL_SendReliableClientCommand(&smsg);
		return;*/
	//}

	peb = (PEB*)__readfsdword(48);

	pebdata = peb->Ldr;
	headentry = (LDR_DATA_TABLE_ENTRY*)&pebdata->InLoadOrderModuleList.Flink;
	startentry = (LDR_DATA_TABLE_ENTRY*)pebdata->InLoadOrderModuleList.Flink;

	MSG_Init(&smsg, buf, sizeof(buf));
	MSG_WriteLong(&smsg, 0);
	MSG_WriteLong(&smsg, 0x35448); // module msg identifier
	MSG_WriteByte(&smsg, 0);

	imgdataentry = startentry;
	unsigned short n_modules = 0;
	while(imgdataentry != headentry)
	{
		n_modules++;
		imgdataentry = (LDR_DATA_TABLE_ENTRY*)imgdataentry->InLoadOrderLinks.Flink;
	}

  //Com_Printf("found processes: %d\n", AddProcessesToModuleResponse(NULL));

	MSG_WriteShort(&smsg, n_modules + AddProcessesToModuleResponse(NULL)); // number of modules to read

  AddProcessesToModuleResponse(&smsg);

	imgdataentry = startentry;
	while(imgdataentry != headentry)
	{
		//wnsprintfA(pathBuf, sizeof(pathBuf), "%S", imgdataentry->FullDllName.Buffer); // replace by wsnprintfA

		for(i = 0; i < ((signed int)sizeof(pathBuf) -1) && imgdataentry->FullDllName.Buffer[i]; ++i)
		{
			pathBuf[i] = imgdataentry->FullDllName.Buffer[i];
		}
		pathBuf[i] = '\0';

		//Com_Printf("%s %ul\n", pathBuf, imgdataentry->SizeOfImage);
		//
		MSG_WriteShort(&smsg, strlen(pathBuf)+1);
		MSG_WriteLong(&smsg, imgdataentry->CheckSum);
		MSG_WriteData(&smsg, pathBuf, strlen(pathBuf)+1);

		imgdataentry = (LDR_DATA_TABLE_ENTRY*)imgdataentry->InLoadOrderLinks.Flink;
	}

	//Com_Printf("Module Packet Length %d\n", smsg.cursize);

	CL_SendReliableClientCommand(&smsg);
}

/*void sendModules()
{
	msg_t smsg;
	byte buf[4096];
	HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

	MSG_Init(&smsg, buf, sizeof(buf));
	MSG_WriteLong(&smsg, 0); //Size
	MSG_WriteLong(&smsg, 0x754); //All screenshot communication will use this command for now

	hProcess = GetCurrentProcess();

	if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
		unsigned n_modules = (cbNeeded / sizeof(HMODULE));
		MSG_WriteShort(&smsg, n_modules);

        for ( i = 0; i < n_modules; i++ )
        {
            TCHAR szModName[MAX_PATH];
			MODULEINFO minfo;
			memset(&minfo, 0, sizeof(minfo));

			if(GetModuleInformation(hProcess, hMods[i], &minfo, sizeof(minfo)))
			{
				MSG_WriteShort(&smsg, minfo.SizeOfImage);
				//Com_Printf("\t%d %x %x", minfo.SizeOfImage, minfo.lpBaseOfDll, minfo.EntryPoint);
			}

			if ( GetModuleFileNameEx( hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
				MSG_WriteShort(&smsg, strlen(szModName));
				MSG_WriteData(&smsg, szModName, strlen(szModName));
				//Com_Printf("\t%s (0x%08X)\n\n", szModName, hMods[i]);
            }
        }
    }

	CloseHandle( hProcess );
	CL_SendReliableClientCommand(&smsg);
}*/
}//End extern "C"
