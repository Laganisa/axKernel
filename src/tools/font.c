#include "tools/_font.h"
#include "tools/_virtio.h"

#pragma region ascii

const uint8_t font_NULL[8];

const uint8_t font_EXCL[8];

const uint8_t font_DBLQ[8];

const uint8_t font_HASH[8];

const uint8_t font_DLRS[8];

const uint8_t font_PERC[8];

const uint8_t font_AMPR[8];

const uint8_t font_APOS[8];

const uint8_t font_LPAR[8];

const uint8_t font_RPAR[8];

const uint8_t font_ASTR[8];

const uint8_t font_PLUS[8];

const uint8_t font_COMM[8];

const uint8_t font_MINS[8];

const uint8_t font_DOTS[8];

const uint8_t font_SLSH[8];

const uint8_t font_0[8];

const uint8_t font_1[8];

const uint8_t font_2[8];

const uint8_t font_3[8];

const uint8_t font_4[8];

const uint8_t font_5[8];

const uint8_t font_6[8];

const uint8_t font_7[8];

const uint8_t font_8[8];

const uint8_t font_9[8];

const uint8_t font_COLN[8];

const uint8_t font_SCLN[8];

const uint8_t font_LESS[8];

const uint8_t font_EQUL[8];

const uint8_t font_GRTR[8];

const uint8_t font_QUES[8];

const uint8_t font_ATSN[8];

const uint8_t font_A[8] = {
    0b00011000,
    0b00100100,
    0b00100100,
    0b00100100,
    0b01000010,
    0b01111110,
    0b01000010,
    0b01000010};

const uint8_t font_B[8];

const uint8_t font_C[8];

const uint8_t font_D[8];

const uint8_t font_E[8];

const uint8_t font_F[8];

const uint8_t font_G[8];

const uint8_t font_H[8];

const uint8_t font_I[8];

const uint8_t font_J[8];

const uint8_t font_K[8];

const uint8_t font_L[8];

const uint8_t font_M[8];

const uint8_t font_N[8];

const uint8_t font_O[8];

const uint8_t font_P[8];

const uint8_t font_Q[8];

const uint8_t font_R[8];

const uint8_t font_S[8];

const uint8_t font_T[8];

const uint8_t font_U[8];

const uint8_t font_V[8];

const uint8_t font_W[8];

const uint8_t font_X[8];

const uint8_t font_Y[8];

const uint8_t font_Z[8];

const uint8_t font_LSBR[8];

const uint8_t font_BSLH[8];

const uint8_t font_RSBR[8];

const uint8_t font_CART[8];

const uint8_t font_UNDS[8];

const uint8_t font_GRVE[8];

const uint8_t font_a[8];

const uint8_t font_b[8];

const uint8_t font_c[8];

const uint8_t font_d[8];

const uint8_t font_e[8];

const uint8_t font_f[8];

const uint8_t font_g[8];

const uint8_t font_h[8];

const uint8_t font_i[8];

const uint8_t font_j[8];

const uint8_t font_k[8];

const uint8_t font_l[8];

const uint8_t font_m[8];

const uint8_t font_n[8];

const uint8_t font_o[8];

const uint8_t font_p[8];

const uint8_t font_q[8];

const uint8_t font_r[8];

const uint8_t font_s[8];

const uint8_t font_t[8];

const uint8_t font_u[8];

const uint8_t font_v[8];

const uint8_t font_w[8];

const uint8_t font_x[8];

const uint8_t font_y[8];

const uint8_t font_z[8];

const uint8_t font_LCBR[8];

const uint8_t font_VBAR[8];

const uint8_t font_RCBR[8];

const uint8_t font_TILD[8];

#pragma endregion

uint8_t *font_table[128] =
    {
        [0] = font_NULL,
        [33] = font_EXCL,
        [34] = font_DBLQ,
        [35] = font_HASH,
        [36] = font_DLRS,
        [37] = font_PERC,
        [38] = font_AMPR,
        [39] = font_APOS,
        [40] = font_LPAR,
        [41] = font_RPAR,
        [42] = font_ASTR,
        [43] = font_PLUS,
        [44] = font_COMM,
        [45] = font_MINS,
        [46] = font_DOTS,
        [47] = font_SLSH,
        [48] = font_0,
        [49] = font_1,
        [50] = font_2,
        [51] = font_3,
        [52] = font_4,
        [53] = font_5,
        [54] = font_6,
        [55] = font_7,
        [56] = font_8,
        [57] = font_9,
        [58] = font_COLN,
        [59] = font_SCLN,
        [60] = font_LESS,
        [61] = font_EQUL,
        [62] = font_GRTR,
        [63] = font_QUES,
        [64] = font_ATSN,
        [65] = font_A,
        [66] = font_B,
        [67] = font_C,
        [68] = font_D,
        [69] = font_E,
        [70] = font_F,
        [71] = font_G,
        [72] = font_H,
        [73] = font_I,
        [74] = font_J,
        [75] = font_K,
        [76] = font_L,
        [77] = font_M,
        [78] = font_N,
        [79] = font_O,
        [80] = font_P,
        [81] = font_Q,
        [82] = font_R,
        [83] = font_S,
        [84] = font_T,
        [85] = font_U,
        [86] = font_V,
        [87] = font_W,
        [88] = font_X,
        [89] = font_Y,
        [90] = font_Z,
        [91] = font_LSBR,
        [92] = font_BSLH,
        [93] = font_RSBR,
        [94] = font_CART,
        [95] = font_UNDS,
        [96] = font_GRVE,
        [97] = font_a,
        [98] = font_b,
        [99] = font_c,
        [100] = font_d,
        [101] = font_e,
        [102] = font_f,
        [103] = font_g,
        [104] = font_h,
        [105] = font_i,
        [106] = font_j,
        [107] = font_k,
        [108] = font_l,
        [109] = font_m,
        [110] = font_n,
        [111] = font_o,
        [112] = font_p,
        [113] = font_q,
        [114] = font_r,
        [115] = font_s,
        [116] = font_t,
        [117] = font_u,
        [118] = font_v,
        [119] = font_w,
        [120] = font_x,
        [121] = font_y,
        [122] = font_z,
        [123] = font_LCBR,
        [124] = font_VBAR,
        [125] = font_RCBR,
        [126] = font_TILD};

void ltr(uint32_t x,
         uint32_t y,
         char c)
{
    if (x > gpu_display_width || y > gpu_display_height)
        return;

    // 글자 처리
    uint8_t *glyph = font_table[(uint8_t)c];

    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            if ((glyph[row] >> (7 - col)) & 1)
            {
                draw_pixel(x + col, y + row, 0xFFFFFFFF);
            }
        }
    }

    if (gpu_transfer_to_host_2d(
            x,
            y,
            8,
            8) < 0)
    {
        puts("GPU partial transfer failed\n");
        return;
    }

    if (gpu_resource_flush(
            x,
            y,
            8,
            8) < 0)
    {
        puts("GPU partial flush failed\n");
        return;
    }
}