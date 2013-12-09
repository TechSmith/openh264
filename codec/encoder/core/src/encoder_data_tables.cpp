/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

// exp_data.c
// export date cross various modules (.c)
#include "typedefs.h"
#include "wels_common_basis.h"
#include "mb_cache.h"
#include "utils.h"
#include "md.h"
#include "sample.h"
#include "svc_enc_golomb.h"
#include "vlc_encoder.h"
namespace WelsSVCEnc {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at mb_cache.h
const uint8_t g_kuiSmb4AddrIn256[16] = 
{
	0,		4,		16*4,		16*4+4,
	8,		12,		16*4+8,		16*4+12,
	16*8,	16*8+4,	16*12,		16*12+4,
	16*8+8,  16*8+12,  16*12+8, 16*12+12
};                       

//////pNonZeroCount[16+8] mapping scan index
const uint8_t g_kuiMbCountScan4Idx[24] =
{                     //  0   1 | 4  5      luma 8*8 block           pNonZeroCount[16+8] 
	0,  1,  4,  5,   //  2   3 | 6  7        0  |  1                  0   1   2   3 
	2,  3,  6,  7,   //---------------      ---------                 4   5   6   7 
	8,  9, 12, 13,   //  8   9 | 12 13       2  |  3                  8   9  10  11 
	10, 11, 14, 15,   // 10  11 | 14 15-----------------------------> 12  13  14  15 
	16, 17, 20, 21,   //----------------    chroma 8*8 block          16  17  18  19  
	18, 19, 22, 23   // 16  17 | 20 21        0    1                 20  21  22  23 
};

const uint8_t g_kuiCache48CountScan4Idx[24] =
{	// [16 + 2*4]
	9, 10, 17, 18,	
	11, 12, 19, 20,	
	25, 26, 33, 34,	
	27, 28, 35, 36,	
	14, 15,			
	22, 23,			
	38, 39,			
	46, 47			
};	


//cache element equal to 30
const uint8_t g_kuiCache30ScanIdx[16] = //mv or uiRefIndex cache scan index, 4*4 block as basic unit
{
	7,  8, 13, 14,
	9, 10, 15, 16,
	19, 20, 25, 26,
	21, 22, 27, 28
};

const uint8_t g_kuiCache12_8x8RefIdx[4] = //mv or uiRefIndex cache scan index, 4*4 block as basic unit
{
	5,6,
	9, 10
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at mb_cache.h

const str_t *g_sWelsLogTags[] = {
	"ERR",
	"WARN",
	"INFO",
	"DBUG",
	"RESV"
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at wels_common_basis.h
const uint8_t g_kuiChromaQpTable[52]={
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
	12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
	28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
	37,38,38,38,39,39,39,39
};

/*
 *	vcl type map for given NAL unit type and corresponding H264 type (0: AVC; 1: SVC).
 */
const EVclType g_keTypeMap[32][2] =
{
	{ NON_VCL,	NON_VCL },	// 0: NAL_UNIT_UNSPEC_0
	{ VCL,		VCL,	},	// 1: NAL_UNIT_CODED_SLICE
	{ VCL,		NOT_APP },	// 2: NAL_UNIT_CODED_SLICE_DPA
	{ VCL,		NOT_APP },	// 3: NAL_UNIT_CODED_SLICE_DPB
	{ VCL,		NOT_APP },	// 4: NAL_UNIT_CODED_SLICE_DPC
	{ VCL,		VCL		},	// 5: NAL_UNIT_CODED_SLICE_IDR
	{ NON_VCL,	NON_VCL },	// 6: NAL_UNIT_SEI
	{ NON_VCL,	NON_VCL },	// 7: NAL_UNIT_SPS
	{ NON_VCL,	NON_VCL },	// 8: NAL_UNIT_PPS
	{ NON_VCL,	NON_VCL },	// 9: NAL_UNIT_AU_DELIMITER
	{ NON_VCL,	NON_VCL },	// 10: NAL_UNIT_END_OF_SEQ
	{ NON_VCL,	NON_VCL },	// 11: NAL_UNIT_END_OF_STR
	{ NON_VCL,	NON_VCL	},	// 12: NAL_UNIT_FILLER_DATA
	{ NON_VCL,	NON_VCL },	// 13: NAL_UNIT_SPS_EXT
	{ NON_VCL,	NON_VCL },	// 14: NAL_UNIT_PREFIX, NEED associate succeeded NAL to make a VCL
	{ NON_VCL,	NON_VCL },	// 15: NAL_UNIT_SUBSET_SPS
	{ NON_VCL,	NON_VCL },	// 16: NAL_UNIT_RESV_16
	{ NON_VCL,	NON_VCL },	// 17: NAL_UNIT_RESV_17
	{ NON_VCL,	NON_VCL },	// 18: NAL_UNIT_RESV_18
	{ NON_VCL,	NON_VCL },	// 19: NAL_UNIT_AUX_CODED_SLICE
	{ NON_VCL,	VCL		},	// 20: NAL_UNIT_CODED_SLICE_EXT
	{ NON_VCL,	NON_VCL },	// 21: NAL_UNIT_RESV_21
	{ NON_VCL,	NON_VCL },	// 22: NAL_UNIT_RESV_22
	{ NON_VCL,	NON_VCL },	// 23: NAL_UNIT_RESV_23
	{ NON_VCL,	NON_VCL },	// 24: NAL_UNIT_UNSPEC_24
	{ NON_VCL,	NON_VCL },	// 25: NAL_UNIT_UNSPEC_25
	{ NON_VCL,	NON_VCL },	// 26: NAL_UNIT_UNSPEC_26
	{ NON_VCL,	NON_VCL	},	// 27: NAL_UNIT_UNSPEC_27
	{ NON_VCL,	NON_VCL },	// 28: NAL_UNIT_UNSPEC_28
	{ NON_VCL,	NON_VCL },	// 29: NAL_UNIT_UNSPEC_29
	{ NON_VCL,	NON_VCL },	// 30: NAL_UNIT_UNSPEC_30
	{ NON_VCL,	NON_VCL }	// 31: NAL_UNIT_UNSPEC_31
};

__align16( const uint16_t, g_kuiDequantCoeff[52][8]) = {
/* 0*/{   10,   13,   10,   13,   13,   16,   13,   16 },	/* 1*/{   11,   14,   11,   14,   14,   18,   14,   18 },
/* 2*/{   13,   16,   13,   16,   16,   20,   16,   20 },	/* 3*/{   14,   18,   14,   18,   18,   23,   18,   23 },
/* 4*/{   16,   20,   16,   20,   20,   25,   20,   25 },	/* 5*/{   18,   23,   18,   23,   23,   29,   23,   29 },
/* 6*/{   20,   26,   20,   26,   26,   32,   26,   32 },	/* 7*/{   22,   28,   22,   28,   28,   36,   28,   36 },
/* 8*/{   26,   32,   26,   32,   32,   40,   32,   40 },	/* 9*/{   28,   36,   28,   36,   36,   46,   36,   46 },
/*10*/{   32,   40,   32,   40,   40,   50,   40,   50 },	/*11*/{   36,   46,   36,   46,   46,   58,   46,   58 },
/*12*/{   40,   52,   40,   52,   52,   64,   52,   64 },	/*13*/{   44,   56,   44,   56,   56,   72,   56,   72 },
/*14*/{   52,   64,   52,   64,   64,   80,   64,   80 },	/*15*/{   56,   72,   56,   72,   72,   92,   72,   92 },
/*16*/{   64,   80,   64,   80,   80,  100,   80,  100 },	/*17*/{   72,   92,   72,   92,   92,  116,   92,  116 },
/*18*/{   80,  104,   80,  104,  104,  128,  104,  128 },	/*19*/{   88,  112,   88,  112,  112,  144,  112,  144 },
/*20*/{  104,  128,  104,  128,  128,  160,  128,  160 },	/*21*/{  112,  144,  112,  144,  144,  184,  144,  184 },
/*22*/{  128,  160,  128,  160,  160,  200,  160,  200 },	/*23*/{  144,  184,  144,  184,  184,  232,  184,  232 },
/*24*/{  160,  208,  160,  208,  208,  256,  208,  256 },	/*25*/{  176,  224,  176,  224,  224,  288,  224,  288 },
/*26*/{  208,  256,  208,  256,  256,  320,  256,  320 },	/*27*/{  224,  288,  224,  288,  288,  368,  288,  368 },
/*28*/{  256,  320,  256,  320,  320,  400,  320,  400 },	/*29*/{  288,  368,  288,  368,  368,  464,  368,  464 },
/*30*/{  320,  416,  320,  416,  416,  512,  416,  512 },	/*31*/{  352,  448,  352,  448,  448,  576,  448,  576 },
/*32*/{  416,  512,  416,  512,  512,  640,  512,  640 },	/*33*/{  448,  576,  448,  576,  576,  736,  576,  736 },
/*34*/{  512,  640,  512,  640,  640,  800,  640,  800 },	/*35*/{  576,  736,  576,  736,  736,  928,  736,  928 },
/*36*/{  640,  832,  640,  832,  832, 1024,  832, 1024 },	/*37*/{  704,  896,  704,  896,  896, 1152,  896, 1152 },
/*38*/{  832, 1024,  832, 1024, 1024, 1280, 1024, 1280 },	/*39*/{  896, 1152,  896, 1152, 1152, 1472, 1152, 1472 },
/*40*/{ 1024, 1280, 1024, 1280, 1280, 1600, 1280, 1600 },	/*41*/{ 1152, 1472, 1152, 1472, 1472, 1856, 1472, 1856 },
/*42*/{ 1280, 1664, 1280, 1664, 1664, 2048, 1664, 2048 },	/*43*/{ 1408, 1792, 1408, 1792, 1792, 2304, 1792, 2304 },
/*44*/{ 1664, 2048, 1664, 2048, 2048, 2560, 2048, 2560 },	/*45*/{ 1792, 2304, 1792, 2304, 2304, 2944, 2304, 2944 },
/*46*/{ 2048, 2560, 2048, 2560, 2560, 3200, 2560, 3200 },	/*47*/{ 2304, 2944, 2304, 2944, 2944, 3712, 2944, 3712 },
/*48*/{ 2560, 3328, 2560, 3328, 3328, 4096, 3328, 4096 },	/*49*/{ 2816, 3584, 2816, 3584, 3584, 4608, 3584, 4608 },
/*50*/{ 3328, 4096, 3328, 4096, 4096, 5120, 4096, 5120 },	/*51*/{ 3584, 4608, 3584, 4608, 4608, 5888, 4608, 5888 },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at md.h
const int32_t g_kiQpCostTable[52] = 
{
	1, 1, 1, 1, 1, 1, 1, 1,  /*  0-7 */
	1, 1, 1, 1,              /*  8-11 */
	1, 1, 1, 1, 2, 2, 2, 2,  /* 12-19 */
	3, 3, 3, 4, 4, 4, 5, 6,  /* 20-27 */
	6, 7, 8, 9,10,11,13,14,  /* 28-35 */
	16,18,20,23,25,29,32,36,  /* 36-43 */
	40,45,51,57,64,72,81,91   /* 44-51 */
};
const int8_t g_kiMapModeI16x16[7] = 
{
	0, 1, 2, 3, 2, 2, 2
};//{I16_PRED_V, I16_PRED_H, I16_PRED_DC, I16_PRED_P, I16_PRED_DC, I16_PRED_DC, I16_PRED_DC};

const int8_t g_kiMapModeIntraChroma[7] = 
{
	0, 1, 2, 3, 0, 0, 0
};//{C_PRED_DC, C_PRED_H, C_PRED_V, C_PRED_P, C_PRED_DC_L, C_PRED_DC_T, C_PRED_DC_128};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at svc_enc_golomb.h

const uint32_t g_uiGolombUELength[256] =
{
	1,  3,  3,  5,  5,  5,  5,  7,  7,  7,  7,  7,  7,  7,  7,    //14
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9, //30
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, //46
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, //62
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, //
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	17
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at vlc_encoder.h

//g_kuiVlcCoeffToken[nc][total-coeff][trailing-ones][0--value, 1--bit count]
const uint8_t g_kuiVlcCoeffToken[5][17][4][2] = 
{
	{//0<=nc<2
		{	{ 1,  1}, { 0,  0}, { 0,  0}, { 0,  0} }, //0
		{	{ 5,  6}, { 1,  2}, { 0,  0}, { 0,  0} },//1
		{	{ 7,  8}, { 4,  6}, { 1,  3}, { 0,  0} },//2
		{	{ 7,  9}, { 6,  8}, { 5,  7}, { 3,  5} },//3
		{	{ 7, 10}, { 6,  9}, { 5,  8}, { 3,  6} },//4
		{	{ 7, 11}, { 6, 10}, { 5,  9}, { 4,  7} },//5
		{	{15, 13}, { 6, 11}, { 5, 10}, { 4,  8} },//6
		{	{11, 13}, {14, 13}, { 5, 11}, { 4,  9} },//7
		{	{ 8, 13}, {10, 13}, {13, 13}, { 4, 10} },//8
		{	{15, 14}, {14, 14}, { 9, 13}, { 4, 11} },//9
		{	{11, 14}, {10, 14}, {13, 14}, {12, 13} },//10
		{	{15, 15}, {14, 15}, { 9, 14}, {12, 14} },//11
		{	{11, 15}, {10, 15}, {13, 15}, { 8, 14} },//12
		{	{15, 16}, { 1, 15}, { 9, 15}, {12, 15} },//13
		{	{11, 16}, {14, 16}, {13, 16}, { 8, 15} },//14
		{	{ 7, 16}, {10, 16}, { 9, 16}, {12, 16} },//15
		{	{ 4, 16}, { 6, 16}, { 5, 16}, { 8, 16} }//16
	},

	{//2<=nc<4
		{	{ 3,  2}, { 0,  0}, { 0,  0}, { 0,  0} },//0
		{	{11,  6}, { 2,  2}, { 0,  0}, { 0,  0} },//1
		{	{ 7,  6}, { 7,  5}, { 3,  3}, { 0,  0} },//2
		{	{ 7,  7}, {10,  6}, { 9,  6}, { 5,  4} },//3
		{	{ 7,  8}, { 6,  6}, { 5,  6}, { 4,  4} },//4
		{	{ 4,  8}, { 6,  7}, { 5,  7}, { 6,  5} },//5
		{	{ 7,  9}, { 6,  8}, { 5,  8}, { 8,  6} },//6
		{	{15, 11}, { 6,  9}, { 5,  9}, { 4,  6} },//7
		{	{11, 11}, {14, 11}, {13, 11}, { 4,  7} },//8
		{	{15, 12}, {10, 11}, { 9, 11}, { 4,  9} },//9
		{	{11, 12}, {14, 12}, {13, 12}, {12, 11} },//10
		{	{ 8, 12}, {10, 12}, { 9, 12}, { 8, 11} },//11
		{	{15, 13}, {14, 13}, {13, 13}, {12, 12} },//12
		{	{11, 13}, {10, 13}, { 9, 13}, {12, 13} },//13
		{	{ 7, 13}, {11, 14}, { 6, 13}, { 8, 13} },//14
		{	{ 9, 14}, { 8, 14}, {10, 14}, { 1, 13} },//15
		{	{ 7, 14}, { 6, 14}, { 5, 14}, { 4, 14} }//16
	},

	{//4<=nc<8
		{	{15,  4}, { 0,  0}, { 0,  0}, { 0,  0} },//0
		{	{15,  6}, {14,  4}, { 0,  0}, { 0,  0} },//1
		{	{11,  6}, {15,  5}, {13,  4}, { 0,  0} },//2
		{	{ 8,  6}, {12,  5}, {14,  5}, {12,  4} },//3
		{	{15,  7}, {10,  5}, {11,  5}, {11,  4} },//4
		{	{11,  7}, { 8,  5}, { 9,  5}, {10,  4} },//5
		{	{ 9,  7}, {14,  6}, {13,  6}, { 9,  4} },//6
		{	{ 8,  7}, {10,  6}, { 9,  6}, { 8,  4} },//7 
		{	{15,  8}, {14,  7}, {13,  7}, {13,  5} },//8
		{	{11,  8}, {14,  8}, {10,  7}, {12,  6} },//9
		{	{15,  9}, {10,  8}, {13,  8}, {12,  7} },//10
		{	{11,  9}, {14,  9}, { 9,  8}, {12,  8} },//11
		{	{ 8,  9}, {10,  9}, {13,  9}, { 8,  8} },//12
		{	{13, 10}, { 7,  9}, { 9,  9}, {12,  9} },//13
		{	{ 9, 10}, {12, 10}, {11, 10}, {10, 10} },//14
		{	{ 5, 10}, { 8, 10}, { 7, 10}, { 6, 10} },//15
		{	{ 1, 10}, { 4, 10}, { 3, 10}, { 2, 10} }//16
	},

	{//8<=nc
		{	{ 3,  6}, { 0,  0}, { 0,  0}, { 0,  0} },//0
		{	{ 0,  6}, { 1,  6}, { 0,  0}, { 0,  0} },//1
		{	{ 4,  6}, { 5,  6}, { 6,  6}, { 0,  0} },//2
		{	{ 8,  6}, { 9,  6}, {10,  6}, {11,  6} },//3
		{	{12,  6}, {13,  6}, {14,  6}, {15,  6} },//4
		{	{16,  6}, {17,  6}, {18,  6}, {19,  6} },//5
		{	{20,  6}, {21,  6}, {22,  6}, {23,  6} },//6
		{	{24,  6}, {25,  6}, {26,  6}, {27,  6} },//7
		{	{28,  6}, {29,  6}, {30,  6}, {31,  6} },//8
		{	{32,  6}, {33,  6}, {34,  6}, {35,  6} },//9
		{	{36,  6}, {37,  6}, {38,  6}, {39,  6} },//10
		{	{40,  6}, {41,  6}, {42,  6}, {43,  6} },//11
		{	{44,  6}, {45,  6}, {46,  6}, {47,  6} },//12
		{	{48,  6}, {49,  6}, {50,  6}, {51,  6} },//13
		{	{52,  6}, {53,  6}, {54,  6}, {55,  6} },//14
		{	{56,  6}, {57,  6}, {58,  6}, {59,  6} },//15
		{	{60,  6}, {61,  6}, {62,  6}, {63,  6} }//16
	},

	{//nc == -1
		{	{ 1,  2}, { 0,  0}, { 0,  0}, { 0,  0} },//0
		{	{ 7,  6}, { 1,  1}, { 0,  0}, { 0,  0} },//1
		{	{ 4,  6}, { 6,  6}, { 1,  3}, { 0,  0} },//2
		{	{ 3,  6}, { 3,  7}, { 2,  7}, { 5,  6} },//3
		{	{ 2,  6}, { 3,  8}, { 2,  8}, { 0,  7} },//4
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//5
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//6
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//7
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//8
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//9
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//10
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//11
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//12
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//13
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//14
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} },//15
		{	{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0} }//16
	}
};

//const uint8_t g_kuiVlcLevelPrefix[15][2] =
//{
//	{1, 1}, {1, 2}
//}; 

//g_kuiVlcTotalZeros[tzVlcIndex][total_zeros][0--value, 1--bit count]
const uint8_t g_kuiVlcTotalZeros[16][16][2] = 
{
	{//0 not available
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} 	
	},
	{//1
		{1, 1}, {3, 3}, {2, 3}, {3, 4}, {2, 4}, {3, 5}, {2, 5}, {3, 6}, {2, 6}, {3, 7}, {2, 7}, {3, 8}, {2, 8}, {3, 9}, {2, 9}, {1, 9}
	},
	{//2
		{7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {5, 4}, {4, 4}, {3, 4}, {2, 4}, {3, 5}, {2, 5}, {3, 6}, {2, 6}, {1, 6}, {0, 6}, {0, 0}
	},
	{//3
		{5, 4}, {7, 3}, {6, 3}, {5, 3}, {4, 4}, {3, 4}, {4, 3}, {3, 3}, {2, 4}, {3, 5}, {2, 5}, {1, 6}, {1, 5}, {0, 6}, {0, 0}, {0, 0}
	},
	{//4
		{3, 5}, {7, 3}, {5, 4}, {4, 4}, {6, 3}, {5, 3}, {4, 3}, {3, 4}, {3, 3}, {2, 4}, {2, 5}, {1, 5}, {0, 5}, {0, 0}, {0, 0}, {0, 0}
	},
	{//5
		{5, 4}, {4, 4}, {3, 4}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 4}, {1, 5}, {1, 4}, {0, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//6
		{1, 6}, {1, 5}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 3}, {1, 4}, {1, 3}, {0, 6}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//7
		{1, 6}, {1, 5}, {5, 3}, {4, 3}, {3, 3}, {3, 2}, {2, 3}, {1, 4}, {1, 3}, {0, 6}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//8
		{1, 6}, {1, 4}, {1, 5}, {3, 3}, {3, 2}, {2, 2}, {2, 3}, {1, 3}, {0, 6}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//9
		{1, 6}, {0, 6}, {1, 4}, {3, 2}, {2, 2}, {1, 3}, {1, 2}, {1, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//10
		{1, 5}, {0, 5}, {1, 3}, {3, 2}, {2, 2}, {1, 2}, {1, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//11
		{0, 4}, {1, 4}, {1, 3}, {2, 3}, {1, 1}, {3, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//12
		{0, 4}, {1, 4}, {1, 2}, {1, 1}, {1, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//13
		{0, 3}, {1, 3}, {1, 1}, {1, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//14
		{0, 2}, {1, 2}, {1, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//15
		{0, 1}, {1, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	}
};

const uint8_t g_kuiVlcTotalZerosChromaDc[4][4][2] =
{
	{
		{0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{
		{1, 1}, {1, 2}, {1, 3}, {0, 3}
	},
	{
		{1, 1}, {1, 2}, {0, 2}, {0, 0} 
	},
	{
		{1, 1}, {0, 1}, {0, 0}, {0, 0}
	}
};
//

//g_kuiVlcRunBefore[zeros-left][run-before][0--value, 1--bit count]
const uint8_t g_kuiVlcRunBefore[8][15][2] = 
{
	{//0 not available
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} 	
	},
	{//1
		{1, 1}, {0, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//2
		{1, 1}, {1, 2}, {0, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//3
		{3, 2}, {2, 2}, {1, 2}, {0, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//4
		{3, 2}, {2, 2}, {1, 2}, {1, 3}, {0, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//5
		{3, 2}, {2, 2}, {3, 3}, {2, 3}, {1, 3}, {0, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//6
		{3, 2}, {0, 3}, {1, 3}, {3, 3}, {2, 3}, {5, 3}, {4, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},
	{//>6
		{7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 3}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1, 10}, {1, 11}
	}
};

const ALIGNED_DECLARE(uint8_t, g_kuiEncNcMapTable[18], 16) =
{
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
};



const uint8_t   g_kuiTemporalIdListTable[MAX_TEMPORAL_LEVEL][MAX_GOP_SIZE + 1] = 
{
	{  0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0  },  // gop size = 1
	{  0, 1, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0,
       0  },  // uiGopSize = 2
	{  0, 2, 1, 2, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0,
       0  },  // uiGopSize = 4
	{  0, 3, 2, 3, 1, 3, 2, 3,
       0, 0, 0, 0, 0, 0, 0, 0,
       0  },  // uiGopSize = 8
	{  0, 4, 3, 4, 2, 4, 3, 4,
       1, 4, 3, 4, 2, 4, 3, 4,
       0  }  //  uiGopSize = 16
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at svc_encode_slice.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
