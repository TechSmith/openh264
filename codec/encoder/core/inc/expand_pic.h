/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
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
 *
 * \file		expand_pic.h
 *
 * \brief		Interface for expanding reconstructed picture to be used for reference
 *
 * \date		06/08/2009
 *************************************************************************************
 */

#ifndef EXPAND_PIC_H
#define EXPAND_PIC_H

#include "typedefs.h"
#include "picture.h"

namespace WelsSVCEnc {
typedef void (*PExpandPictureFunc)( uint8_t *pDst, const int32_t kiStride, const int32_t kiPicW, const int32_t kiPicH );

void ExpandReferencingPicture( SPicture *pPic, PExpandPictureFunc pExpLuma, PExpandPictureFunc pExpChrom[2] );

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#if defined(X86_ASM)
void ExpandPictureLuma_sse2(	uint8_t *pDst,
								const int32_t kiStride,
								const int32_t kiPicW,
								const int32_t kiPicH	);
void ExpandPictureChromaAlign_sse2(	uint8_t *pDst,
									const int32_t kiStride,
									const int32_t kiPicW,
									const int32_t kiPicH	);
void ExpandPictureChromaUnalign_sse2(	uint8_t *pDst,
									const int32_t kiStride,
									const int32_t kiPicW,
									const int32_t kiPicH	);
#endif//X86_ASM
	
#if defined(__cplusplus)
}
#endif//__cplusplus

void InitExpandPictureFunc( void *pL, const uint32_t kuiCPUFlags );
}
#endif
