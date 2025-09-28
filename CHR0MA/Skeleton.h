/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007-2023 Adobe Inc.                                  */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Inc. and its suppliers, if                    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Inc. and its                    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Inc.            */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*
	Skeleton.h
*/

#pragma once

#ifndef SKELETON_H
#define SKELETON_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "Skeleton_Strings.h"

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	1
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter defaults */

#define	CHR0MA_CHROMA_MIN				-100
#define	CHR0MA_CHROMA_MAX				100
#define	CHR0MA_CHROMA_DFLT				0
#define	CHR0MA_HUE_DFLT					0
#define CHR0MA_CLIP_DFLT				0
#define CHR0MA_ADAPTIVE_CLIP_ALPHA_MIN	0.05
#define CHR0MA_ADAPTIVE_CLIP_ALPHA_MAX	5
#define CHR0MA_ADAPTIVE_CLIP_ALPHA_DFLT	0.5

enum {
	SKELETON_INPUT = 0,
	CHR0MA_HUE,
	CHR0MA_CHROMA,
	CHR0MA_CLIP,
	CHROMA_ADAPTIVE_CLIP_ALPHA,
	SKELETON_NUM_PARAMS
};

enum {
	HUE_DISK_ID = 1,
	CHROMA_DISK_ID,
	CLIP_DISK_ID,
	ADAPTIVE_CLIP_ALPHA_ID,
};

// order must match order for StrID_Clip_Param_Options in Skeleton_Strings.cpp
enum ClipType {
	CLIP_TYPE_ADAPTIVE_HUE_DEPENDENT = 1,
	CLIP_TYPE_CHROMA_CLIP,
	CLIP_TYPE_TRUNCATE,
	CLIP_TYPE_DEBUG
};

typedef struct UserInput {
	float		hueRad;  // GainInfo is a struct that contains one double (double float accuracy) gainF
	float		chromaScale;
	ClipType	clipType;
	float		adaptiveClipAlpha;
} UserInput, *UserInputP, **UserInputH;


struct RGB {
	float r, g, b;
};

struct Lab {
	float L, a, b;
};


extern "C" {

	DllExport
	PF_Err
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

#endif // SKELETON_H