#ifndef __KERNEL_FONT_H__
#define __KERNEL_FONT_H__

#include "_types.h"

#pragma region fonts

extern const uint8_t font_EXCL[8];
extern const uint8_t font_DBLQ[8];
extern const uint8_t font_HASH[8];
extern const uint8_t font_DLRS[8];
extern const uint8_t font_PERC[8];
extern const uint8_t font_AMPR[8];
extern const uint8_t font_APOS[8];
extern const uint8_t font_LPAR[8];
extern const uint8_t font_RPAR[8];
extern const uint8_t font_ASTR[8];
extern const uint8_t font_PLUS[8];
extern const uint8_t font_COMM[8];
extern const uint8_t font_MINS[8];
extern const uint8_t font_DOTS[8];
extern const uint8_t font_SLSH[8];
extern const uint8_t font_0[8];
extern const uint8_t font_1[8];
extern const uint8_t font_2[8];
extern const uint8_t font_3[8];
extern const uint8_t font_4[8];
extern const uint8_t font_5[8];
extern const uint8_t font_6[8];
extern const uint8_t font_7[8];
extern const uint8_t font_8[8];
extern const uint8_t font_9[8];
extern const uint8_t font_COLN[8];
extern const uint8_t font_SCLN[8];
extern const uint8_t font_LESS[8];
extern const uint8_t font_EQUL[8];
extern const uint8_t font_GRTR[8];
extern const uint8_t font_QUES[8];
extern const uint8_t font_ATSN[8];
extern const uint8_t font_A[8];
extern const uint8_t font_B[8];
extern const uint8_t font_C[8];
extern const uint8_t font_D[8];
extern const uint8_t font_E[8];
extern const uint8_t font_F[8];
extern const uint8_t font_G[8];
extern const uint8_t font_H[8];
extern const uint8_t font_I[8];
extern const uint8_t font_J[8];
extern const uint8_t font_K[8];
extern const uint8_t font_L[8];
extern const uint8_t font_M[8];
extern const uint8_t font_N[8];
extern const uint8_t font_O[8];
extern const uint8_t font_P[8];
extern const uint8_t font_Q[8];
extern const uint8_t font_R[8];
extern const uint8_t font_S[8];
extern const uint8_t font_T[8];
extern const uint8_t font_U[8];
extern const uint8_t font_V[8];
extern const uint8_t font_W[8];
extern const uint8_t font_X[8];
extern const uint8_t font_Y[8];
extern const uint8_t font_Z[8];
extern const uint8_t font_LSBR[8];
extern const uint8_t font_BSLH[8];
extern const uint8_t font_RSBR[8];
extern const uint8_t font_CART[8];
extern const uint8_t font_UNDS[8];
extern const uint8_t font_GRVE[8];
extern const uint8_t font_a[8];
extern const uint8_t font_b[8];
extern const uint8_t font_c[8];
extern const uint8_t font_d[8];
extern const uint8_t font_e[8];
extern const uint8_t font_f[8];
extern const uint8_t font_g[8];
extern const uint8_t font_h[8];
extern const uint8_t font_i[8];
extern const uint8_t font_j[8];
extern const uint8_t font_k[8];
extern const uint8_t font_l[8];
extern const uint8_t font_m[8];
extern const uint8_t font_n[8];
extern const uint8_t font_o[8];
extern const uint8_t font_p[8];
extern const uint8_t font_q[8];
extern const uint8_t font_r[8];
extern const uint8_t font_s[8];
extern const uint8_t font_t[8];
extern const uint8_t font_u[8];
extern const uint8_t font_v[8];
extern const uint8_t font_w[8];
extern const uint8_t font_x[8];
extern const uint8_t font_y[8];
extern const uint8_t font_z[8];
extern const uint8_t font_LCBR[8];
extern const uint8_t font_VBAR[8];
extern const uint8_t font_RCBR[8];
extern const uint8_t font_TILD[8];

#pragma endregion

void ltr(uint32_t x,
         uint32_t y,
         char c);

void ltrs(uint32_t x,
          uint32_t y,
          char *string);
#endif