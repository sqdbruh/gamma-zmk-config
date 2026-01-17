struct segment_info {
    uint32_t high;
    uint32_t low;
};

static const struct segment_info b1 = {.high = 3, .low = 4};
static const struct segment_info c1 = {.high = 2, .low = 4};
static const struct segment_info a2 = {.high = 2, .low = 3};
static const struct segment_info b2 = {.high = 3, .low = 2};

static const struct segment_info c2 = {.high = 4, .low = 3};
static const struct segment_info d2 = {.high = 4, .low = 2};
static const struct segment_info e2 = {.high = 5, .low = 2};
static const struct segment_info f2 = {.high = 5, .low = 3};

static const struct segment_info g2 = {.high = 5, .low = 4};
static const struct segment_info a3 = {.high = 1, .low = 2};
static const struct segment_info b3 = {.high = 2, .low = 1};
static const struct segment_info c3 = {.high = 1, .low = 3};

static const struct segment_info d3 = {.high = 3, .low = 1};
static const struct segment_info e3 = {.high = 1, .low = 4};
static const struct segment_info f3 = {.high = 4, .low = 1};
static const struct segment_info g3 = {.high = 5, .low = 1};

#define B1 0
#define C1 1
#define A2 2
#define B2 3
#define C2 4
#define D2 5
#define E2 6
#define _F2 7
#define G2 8
#define A3 9
#define B3 10
#define C3 11
#define D3 12
#define E3 13
#define _F3 14
#define G3 15

static struct segment_info segments[] = {
    b1, // 0
    c1, // 1
    a2, // 2
    b2, // 3
    c2, // 4
    d2, // 5
    e2, // 6
    f2, // 7
    g2, // 8
    a3, // 9
    b3, // 10
    c3, // 11
    d3, // 12
    e3, // 13
    f3, // 14
    g3  // 15
};

static const uint16_t digit_xx0 =
    (1 << A3) | (1 << _F3) | (1 << C3) | (1 << D3) | (1 << E3) | (1 << B3);
static const uint16_t digit_xx1 = (1 << B3) | (1 << C3);
static const uint16_t digit_xx2 = (1 << A3) | (1 << B3) | (1 << G3) | (1 << E3) | (1 << D3);
static const uint16_t digit_xx3 = (1 << A3) | (1 << B3) | (1 << G3) | (1 << C3) | (1 << D3);
static const uint16_t digit_xx4 = (1 << _F3) | (1 << B3) | (1 << G3) | (1 << C3);
static const uint16_t digit_xx5 = (1 << A3) | (1 << _F3) | (1 << G3) | (1 << C3) | (1 << D3);
static const uint16_t digit_xx6 =
    (1 << A3) | (1 << _F3) | (1 << G3) | (1 << C3) | (1 << D3) | (1 << E3);
static const uint16_t digit_xx7 = (1 << A3) | (1 << B3) | (1 << C3);
static const uint16_t digit_xx8 =
    (1 << A3) | (1 << _F3) | (1 << G3) | (1 << C3) | (1 << D3) | (1 << E3) | (1 << B3);
static const uint16_t digit_xx9 =
    (1 << A3) | (1 << _F3) | (1 << G3) | (1 << C3) | (1 << D3) | (1 << B3);

static const uint16_t digit_x0x =
    (1 << A2) | (1 << _F2) | (1 << C2) | (1 << D2) | (1 << E2) | (1 << B2);
static const uint16_t digit_x1x = (1 << B2) | (1 << C2);
static const uint16_t digit_x2x = (1 << A2) | (1 << B2) | (1 << G2) | (1 << E2) | (1 << D2);
static const uint16_t digit_x3x = (1 << A2) | (1 << B2) | (1 << G2) | (1 << C2) | (1 << D2);
static const uint16_t digit_x4x = (1 << _F2) | (1 << B2) | (1 << G2) | (1 << C2);
static const uint16_t digit_x5x = (1 << A2) | (1 << _F2) | (1 << G2) | (1 << C2) | (1 << D2);
static const uint16_t digit_x6x =
    (1 << A2) | (1 << _F2) | (1 << G2) | (1 << C2) | (1 << D2) | (1 << E2);
static const uint16_t digit_x7x = (1 << A2) | (1 << B2) | (1 << C2);
static const uint16_t digit_x8x =
    (1 << A2) | (1 << _F2) | (1 << G2) | (1 << C2) | (1 << D2) | (1 << E2) | (1 << B2);
static const uint16_t digit_x9x =
    (1 << A2) | (1 << _F2) | (1 << G2) | (1 << C2) | (1 << D2) | (1 << B2);

static const uint16_t digit_1xx = (1 << B1) | (1 << C1);
static const uint16_t digit_xCh =
    (1 << G2) | (1 << E2) | (1 << D2) | (1 << _F3) | (1 << E3) | (1 << G3) | (1 << C3);

static const uint16_t digit_xbt =
    (1 << _F2) | (1 << E2) | (1 << D2) | (1 << C2) | (1 << G2) | (1 << _F3) | (1 << E3) | (1 << D3) | (1 << G3);

static const uint16_t digits188[] = {
    digit_xx0,
    digit_xx1,
    digit_xx2,
    digit_xx3,
    digit_xx4,
    digit_xx5,
    digit_xx6,
    digit_xx7,
    digit_xx8,
    digit_xx9,

    digit_x1x | digit_xx0,
    digit_x1x | digit_xx1,
    digit_x1x | digit_xx2,
    digit_x1x | digit_xx3,
    digit_x1x | digit_xx4,
    digit_x1x | digit_xx5,
    digit_x1x | digit_xx6,
    digit_x1x | digit_xx7,
    digit_x1x | digit_xx8,
    digit_x1x | digit_xx9,

    digit_x2x | digit_xx0,
    digit_x2x | digit_xx1,
    digit_x2x | digit_xx2,
    digit_x2x | digit_xx3,
    digit_x2x | digit_xx4,
    digit_x2x | digit_xx5,
    digit_x2x | digit_xx6,
    digit_x2x | digit_xx7,
    digit_x2x | digit_xx8,
    digit_x2x | digit_xx9,

    digit_x3x | digit_xx0,
    digit_x3x | digit_xx1,
    digit_x3x | digit_xx2,
    digit_x3x | digit_xx3,
    digit_x3x | digit_xx4,
    digit_x3x | digit_xx5,
    digit_x3x | digit_xx6,
    digit_x3x | digit_xx7,
    digit_x3x | digit_xx8,
    digit_x3x | digit_xx9,

    digit_x4x | digit_xx0,
    digit_x4x | digit_xx1,
    digit_x4x | digit_xx2,
    digit_x4x | digit_xx3,
    digit_x4x | digit_xx4,
    digit_x4x | digit_xx5,
    digit_x4x | digit_xx6,
    digit_x4x | digit_xx7,
    digit_x4x | digit_xx8,
    digit_x4x | digit_xx9,

    digit_x5x | digit_xx0,
    digit_x5x | digit_xx1,
    digit_x5x | digit_xx2,
    digit_x5x | digit_xx3,
    digit_x5x | digit_xx4,
    digit_x5x | digit_xx5,
    digit_x5x | digit_xx6,
    digit_x5x | digit_xx7,
    digit_x5x | digit_xx8,
    digit_x5x | digit_xx9,

    digit_x6x | digit_xx0,
    digit_x6x | digit_xx1,
    digit_x6x | digit_xx2,
    digit_x6x | digit_xx3,
    digit_x6x | digit_xx4,
    digit_x6x | digit_xx5,
    digit_x6x | digit_xx6,
    digit_x6x | digit_xx7,
    digit_x6x | digit_xx8,
    digit_x6x | digit_xx9,

    digit_x7x | digit_xx0,
    digit_x7x | digit_xx1,
    digit_x7x | digit_xx2,
    digit_x7x | digit_xx3,
    digit_x7x | digit_xx4,
    digit_x7x | digit_xx5,
    digit_x7x | digit_xx6,
    digit_x7x | digit_xx7,
    digit_x7x | digit_xx8,
    digit_x7x | digit_xx9,

    digit_x8x | digit_xx0,
    digit_x8x | digit_xx1,
    digit_x8x | digit_xx2,
    digit_x8x | digit_xx3,
    digit_x8x | digit_xx4,
    digit_x8x | digit_xx5,
    digit_x8x | digit_xx6,
    digit_x8x | digit_xx7,
    digit_x8x | digit_xx8,
    digit_x8x | digit_xx9,

    digit_x9x | digit_xx0,
    digit_x9x | digit_xx1,
    digit_x9x | digit_xx2,
    digit_x9x | digit_xx3,
    digit_x9x | digit_xx4,
    digit_x9x | digit_xx5,
    digit_x9x | digit_xx6,
    digit_x9x | digit_xx7,
    digit_x9x | digit_xx8,
    digit_x9x | digit_xx9,

    digit_1xx | digit_x0x | digit_xx0,
    digit_1xx | digit_x0x | digit_xx1,
    digit_1xx | digit_x0x | digit_xx2,
    digit_1xx | digit_x0x | digit_xx3,
    digit_1xx | digit_x0x | digit_xx4,
    digit_1xx | digit_x0x | digit_xx5,
    digit_1xx | digit_x0x | digit_xx6,
    digit_1xx | digit_x0x | digit_xx7,
    digit_1xx | digit_x0x | digit_xx8,
    digit_1xx | digit_x0x | digit_xx9,

    digit_1xx | digit_x1x | digit_xx0,
    digit_1xx | digit_x1x | digit_xx1,
    digit_1xx | digit_x1x | digit_xx2,
    digit_1xx | digit_x1x | digit_xx3,
    digit_1xx | digit_x1x | digit_xx4,
    digit_1xx | digit_x1x | digit_xx5,
    digit_1xx | digit_x1x | digit_xx6,
    digit_1xx | digit_x1x | digit_xx7,
    digit_1xx | digit_x1x | digit_xx8,
    digit_1xx | digit_x1x | digit_xx9,

    digit_1xx | digit_x2x | digit_xx0,
    digit_1xx | digit_x2x | digit_xx1,
    digit_1xx | digit_x2x | digit_xx2,
    digit_1xx | digit_x2x | digit_xx3,
    digit_1xx | digit_x2x | digit_xx4,
    digit_1xx | digit_x2x | digit_xx5,
    digit_1xx | digit_x2x | digit_xx6,
    digit_1xx | digit_x2x | digit_xx7,
    digit_1xx | digit_x2x | digit_xx8,
    digit_1xx | digit_x2x | digit_xx9,
};

#define DRV2605_ADDR 0x5A ///< Device I2C address

#define DRV2605_REG_STATUS 0x00       ///< Status register
#define DRV2605_REG_MODE 0x01         ///< Mode register
#define DRV2605_MODE_INTTRIG 0x00     ///< Internal trigger mode
#define DRV2605_MODE_EXTTRIGEDGE 0x01 ///< External edge trigger mode
#define DRV2605_MODE_EXTTRIGLVL 0x02  ///< External level trigger mode
#define DRV2605_MODE_PWMANALOG 0x03   ///< PWM/Analog input mode
#define DRV2605_MODE_AUDIOVIBE 0x04   ///< Audio-to-vibe mode
#define DRV2605_MODE_REALTIME 0x05    ///< Real-time playback (RTP) mode
#define DRV2605_MODE_DIAGNOS 0x06     ///< Diagnostics mode
#define DRV2605_MODE_AUTOCAL 0x07     ///< Auto calibration mode

#define DRV2605_REG_RTPIN 0x02    ///< Real-time playback input register
#define DRV2605_REG_LIBRARY 0x03  ///< Waveform library selection register
#define DRV2605_REG_WAVESEQ1 0x04 ///< Waveform sequence register 1
#define DRV2605_REG_WAVESEQ2 0x05 ///< Waveform sequence register 2
#define DRV2605_REG_WAVESEQ3 0x06 ///< Waveform sequence register 3
#define DRV2605_REG_WAVESEQ4 0x07 ///< Waveform sequence register 4
#define DRV2605_REG_WAVESEQ5 0x08 ///< Waveform sequence register 5
#define DRV2605_REG_WAVESEQ6 0x09 ///< Waveform sequence register 6
#define DRV2605_REG_WAVESEQ7 0x0A ///< Waveform sequence register 7
#define DRV2605_REG_WAVESEQ8 0x0B ///< Waveform sequence register 8

#define DRV2605_REG_GO 0x0C          ///< Go register
#define DRV2605_REG_OVERDRIVE 0x0D   ///< Overdrive time offset register
#define DRV2605_REG_SUSTAINPOS 0x0E  ///< Sustain time offset, positive register
#define DRV2605_REG_SUSTAINNEG 0x0F  ///< Sustain time offset, negative register
#define DRV2605_REG_BREAK 0x10       ///< Brake time offset register
#define DRV2605_REG_AUDIOCTRL 0x11   ///< Audio-to-vibe control register
#define DRV2605_REG_AUDIOLVL 0x12    ///< Audio-to-vibe minimum input level register
#define DRV2605_REG_AUDIOMAX 0x13    ///< Audio-to-vibe maximum input level register
#define DRV2605_REG_AUDIOOUTMIN 0x14 ///< Audio-to-vibe minimum output drive register
#define DRV2605_REG_AUDIOOUTMAX 0x15 ///< Audio-to-vibe maximum output drive register
#define DRV2605_REG_RATEDV 0x16      ///< Rated voltage register
#define DRV2605_REG_CLAMPV 0x17      ///< Overdrive clamp voltage register
#define DRV2605_REG_AUTOCALCOMP 0x18 ///< Auto-calibration compensation result register
#define DRV2605_REG_AUTOCALEMP 0x19  ///< Auto-calibration back-EMF result register
#define DRV2605_REG_FEEDBACK 0x1A    ///< Feedback control register
#define DRV2605_REG_CONTROL1 0x1B    ///< Control1 Register
#define DRV2605_REG_CONTROL2 0x1C    ///< Control2 Register
#define DRV2605_REG_CONTROL3 0x1D    ///< Control3 Register
#define DRV2605_REG_CONTROL4 0x1E    ///< Control4 Register
#define DRV2605_REG_VBAT 0x21        ///< Vbat voltage-monitor register
#define DRV2605_REG_LRARESON 0x22    ///< LRA resonance-period register
