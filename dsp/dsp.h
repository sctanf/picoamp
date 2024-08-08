#ifndef SCTANFDSP_H
#define SCTANFDSP_H

#define USE_FIX16_MUL

#include <stdio.h>

#include "pico/stdlib.h"

typedef int32_t dspfx;

#define fpformat 15
#define mulfx(a,b) ((dspfx)(((int64_t)(a)*(int64_t)(b))>>fpformat))
#define intfx(a) ((a)<<fpformat)
#define fxint(a) ((a)>>fpformat)
#define floatfx(a) ((dspfx)((a)*(1<<fpformat)))
#define fxfloat(a) ((float)(a)/(1<<fpformat))
#define fxabs(a) ((a)<0?-(a):(a))

#define fpformat2 30
#define mulfx2(a,b) ((dspfx)(((int64_t)(a)*(int64_t)(b))>>fpformat2))
#define intfx2(a) ((a)<<fpformat2)
#define fxint2(a) ((a)>>fpformat2)
#define floatfx2(a) ((dspfx)((a)*(1<<fpformat2)))
#define fxfloat2(a) ((float)(a)/(1<<fpformat2))
#define fxabs2(a) ((a)<0?-(a):(a))

#define fpformat3 28
//#define mulfx0(a,b) ((int64_t)(a)*(int64_t)(b))
#define mulfx3(a,b) ((dspfx)(((int64_t)(a)*(int64_t)(b))>>fpformat3))
#define fxint3(a) ((a)>>fpformat3)
//#define mulshift(a) ((dspfx)fxint3(a))
#define floatfx3(a) ((int64_t)((a)*(1<<fpformat3)))

#ifdef USE_FIX16_MUL
typedef int32_t fix3_28_t;

// https://github.com/ploopyco/headphones/blob/master/firmware/code/fix16.inl
static inline fix3_28_t fix16_mul(fix3_28_t inArg0, fix3_28_t inArg1) {
    int32_t A = (inArg0 >> 14), C = (inArg1 >> 14);
    uint32_t B = (inArg0 & 0x3FFF), D = (inArg1 & 0x3FFF);
    int32_t AC = A*C;
    int32_t AD_CB = A*D + C*B;
    int32_t product_hi = AC + (AD_CB >> 14);

#if HANDLE_CARRY
	// Handle carry from lower bits to upper part of result.
    uint32_t BD = B*D;
	uint32_t ad_cb_temp = AD_CB << 14;
	uint32_t product_lo = BD + ad_cb_temp;

	if (product_lo < BD)
		product_hi++;
#endif

    return product_hi;
}

#define mulfx0(a,b) fix16_mul(a,b)
#define mulshift(a) (a)
#else
#define mulfx0(a,b) ((int64_t)(a)*(int64_t)(b))
#define mulshift(a) ((dspfx)fxint3(a))
#endif

typedef struct {
	const dspfx k;
	dspfx y1, z1, u1;
	dspfx y1r, z1r, u1r;
} fwi;

typedef struct {
	dspfx a1z, a2z, b1z, b2z;
	dspfx a1zr, a2zr, b1zr, b2zr;
} biquad;

#define fwi(a,_k) fwi a = {.k=floatfx(_k),.y1=0,.z1=0,.u1=0,.y1r=0,.z1r=0,.u1r=0,};
#define biquad(a) biquad a = {.a1z=0,.a2z=0,.b1z=0,.b2z=0,.a1zr=0,.a2zr=0,.b1zr=0,.b2zr=0};
#define biquadconstfx(a,b,c,d,e) floatfx3(a),floatfx3(b),floatfx3(c),floatfx3(d),floatfx3(e)
#define biquadconstsfx(a) biquadconstfx(a)
#define hgconstfx(a,b,c,d,e,f,g,h) floatfx3(a),floatfx3(b),floatfx3(c),floatfx3(d),floatfx3(e),floatfx3(f),floatfx3(g),floatfx3(h)
#define hgconstsfx(a) hgconstfx(a)

static inline void process_fwi(fwi *const filter, int16_t iters, int32_t *in, int32_t *out) {
	int16_t iters2 = iters * 2;
	out[0] = filter->z1 + mulfx(filter->k, filter->u1);
	out[1] = filter->z1r + mulfx(filter->k, filter->u1r);
	for (int i = 2; i < iters2; i++) {
		out[i] = ((in[i] > 0) && (in[i - 2] <= 0)) ? 0 : out[i - 2] + mulfx(filter->k, fxabs(in[i - 2]));
	}
	filter->z1 = out[iters2 - 2];
	filter->u1 = fxabs(in[iters2 - 2]);
	filter->z1r = out[iters2 - 1];
	filter->u1r = fxabs(in[iters2 - 1]);
}

static inline void process_biquad(biquad *const filter, int64_t a0, int64_t a1, int64_t a2, int64_t b1, int64_t b2, int16_t iters, int32_t *in, int32_t *out) {
//	if (a0 == 1 && a1 == 0 && a2 == 0 && b1 == 0 && b2 == 0) {
//		memcpy(out, in, sizeof(int32_t) * iters * 2);
//		return; // no processing to do
//	}
	int16_t iters2 = iters * 2;
	out[0] = mulshift(mulfx0(a0, in[0]) + mulfx0(a1, filter->a1z) - mulfx0(b1, filter->b1z) + mulfx0(a2, filter->a2z) - mulfx0(b2, filter->b2z));
	out[2] = mulshift(mulfx0(a0, in[2]) + mulfx0(a1, in[0]) - mulfx0(b1, out[0]) + mulfx0(a2, filter->a1z) - mulfx0(b2, filter->b1z));
	out[1] = mulshift(mulfx0(a0, in[1]) + mulfx0(a1, filter->a1zr) - mulfx0(b1, filter->b1zr) + mulfx0(a2, filter->a2zr) - mulfx0(b2, filter->b2zr));
	out[3] = mulshift(mulfx0(a0, in[3]) + mulfx0(a1, in[1]) - mulfx0(b1, out[1]) + mulfx0(a2, filter->a1zr) - mulfx0(b2, filter->b1zr));
	for (int i = 4; i < iters2; i++) {
		out[i] = mulshift(mulfx0(a0, in[i]) + mulfx0(a1, in[i - 2]) - mulfx0(b1, out[i - 2]) + mulfx0(a2, in[i - 4]) - mulfx0(b2, out[i - 4])); // takes up the most time by far..
	}
	filter->a2z = in[iters2 - 4];
	filter->b2z = out[iters2 - 4];
	filter->a1z = in[iters2 - 2];
	filter->b1z = out[iters2 - 2];
	filter->a2zr = in[iters2 - 3];
	filter->b2zr = out[iters2 - 3];
	filter->a1zr = in[iters2 - 1];
	filter->b1zr = out[iters2 - 1];
}

static inline void process_harmonic_generator(int64_t d0, int64_t d1, int64_t d2, int64_t d3, int64_t d4, int64_t d5, int64_t d6, int64_t d7, int16_t iters, int32_t *in, int32_t *out) {
	int16_t iters2 = iters * 2;
	for (int i = 0; i < iters2; i++) {
		out[i] =
			d0 + mulfx3(
				d1 + mulfx3(
					d2 + mulfx3(
						d3 + mulfx3(
							d4 + mulfx3(
								d5 + mulfx3(
									d6 + mulfx3(
										d7,
										in[i]),
									in[i]),
								in[i]),
							in[i]),
						in[i]),
					in[i]),
				in[i]);
	}
}

#endif