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

#include "Skeleton.h"

typedef struct {
	A_u_long	index;
	A_char		str[256];
} TableString;



TableString		g_strs[StrID_NUMTYPES] = {
	StrID_NONE,								"",
	StrID_Name,								"CHR0MA",
	StrID_Description,						"Change hue & saturation using oklab color science\r by temberlin <3",
	StrID_Hue_Param_Name,					"Hue",
	StrID_Chroma_Param_Name,				"Chroma",
	StrID_Clip_Param_Name,					"Clip Method",
	StrID_Adaptive_Clip_Alpha_Param_Name,	"Adaptive Clip Alpha",
	StrID_Clip_Param_Options,				"Adaptive Hue-dependent|Chroma Clipped|Truncate|Debug", // order must match order for ClipType in Skeleton.h
};


char	*GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
	