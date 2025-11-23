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

/*	Skeleton.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017
	1.1			Added 'Support URL' to PiPL and entry point				cjr			3/31/2023

*/

#include "Skeleton.h"

#include <cmath>
#include <cfloat>

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc
	out_data->out_flags2 = PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	def.flags = PF_ParamFlag_START_COLLAPSED;
	PF_ADD_ANGLE(STR(StrID_Hue_Param_Name), 
							CHR0MA_HUE_DFLT,
							HUE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDER(STR(StrID_Chroma_Param_Name),
						-10000000,
						10000000,
						CHR0MA_CHROMA_MIN,
						CHR0MA_CHROMA_MAX,
						0,
						CHR0MA_CHROMA_DFLT,
						0,
						0,
						0,
						CHROMA_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_POPUP(STR(StrID_Clip_Param_Name),
		2,
		CHR0MA_CLIP_DFLT,
		STR(StrID_Clip_Param_Options),
		CLIP_DISK_ID);

	AEFX_CLR_STRUCT(def);
		
	PF_ADD_FLOAT_SLIDER(STR(StrID_Adaptive_Clip_Alpha_Param_Name),
		-10000000,
		10000000,
		CHR0MA_ADAPTIVE_CLIP_ALPHA_MIN,
		CHR0MA_ADAPTIVE_CLIP_ALPHA_MAX,
		0,
		CHR0MA_ADAPTIVE_CLIP_ALPHA_DFLT,
		3,
		0,
		0,
		ADAPTIVE_CLIP_ALPHA_ID);

	out_data->num_params = SKELETON_NUM_PARAMS;

	return err;
}

static Lab
linear_srgb_to_oklab(
	const RGB	*inP
)
{
	float l = 0.4122214708f * inP->r + 0.5363325363f * inP->g + 0.0514459929f * inP->b;
	float m = 0.2119034982f * inP->r + 0.6806995451f * inP->g + 0.1073969566f * inP->b;
	float s = 0.0883024619f * inP->r + 0.2817188376f * inP->g + 0.6299787005f * inP->b;  //TO oklab transform

	float l_ = cbrtf(l);
	float m_ = cbrtf(m);
	float s_ = cbrtf(s);
	return Lab { 
		0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
		1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,    //sets up struct with Lab values
		0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_, 
	}; //except a, b are shifted up .5 to be 0-1
};

static float clamp(float x, float min, float max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;

	return x;
}

// Finds the maximum saturation possible for a given hue that fits in sRGB
// Saturation here is defined as S = C/L
// a and b must be normalized so a^2 + b^2 == 1
float compute_max_saturation(float a, float b)
{
	// Max saturation will be when one of r, g or b goes below zero.

	// Select different coefficients depending on which component goes below zero first
	float k0, k1, k2, k3, k4, wl, wm, ws;

	if (-1.88170328f * a - 0.80936493f * b > 1)
	{
		// Red component
		k0 = +1.19086277f; k1 = +1.76576728f; k2 = +0.59662641f; k3 = +0.75515197f; k4 = +0.56771245f;
		wl = +4.0767416621f; wm = -3.3077115913f; ws = +0.2309699292f;
	}
	else if (1.81444104f * a - 1.19445276f * b > 1)
	{
		// Green component
		k0 = +0.73956515f; k1 = -0.45954404f; k2 = +0.08285427f; k3 = +0.12541070f; k4 = +0.14503204f;
		wl = -1.2684380046f; wm = +2.6097574011f; ws = -0.3413193965f;
	}
	else
	{
		// Blue component
		k0 = +1.35733652f; k1 = -0.00915799f; k2 = -1.15130210f; k3 = -0.50559606f; k4 = +0.00692167f;
		wl = -0.0041960863f; wm = -0.7034186147f; ws = +1.7076147010f;
	}

	// Approximate max saturation using a polynomial:
	float S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

	// Do one step Halley's method to get closer
	// this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
	// this should be sufficient for most applications, otherwise do two/three steps 

	float k_l = +0.3963377774f * a + 0.2158037573f * b;
	float k_m = -0.1055613458f * a - 0.0638541728f * b;
	float k_s = -0.0894841775f * a - 1.2914855480f * b;

	{
		float l_ = 1.f + S * k_l;
		float m_ = 1.f + S * k_m;
		float s_ = 1.f + S * k_s;

		float l = l_ * l_ * l_;
		float m = m_ * m_ * m_;
		float s = s_ * s_ * s_;

		float l_dS = 3.f * k_l * l_ * l_;
		float m_dS = 3.f * k_m * m_ * m_;
		float s_dS = 3.f * k_s * s_ * s_;

		float l_dS2 = 6.f * k_l * k_l * l_;
		float m_dS2 = 6.f * k_m * k_m * m_;
		float s_dS2 = 6.f * k_s * k_s * s_;

		float f = wl * l + wm * m + ws * s;
		float f1 = wl * l_dS + wm * m_dS + ws * s_dS;
		float f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

		S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
	}

	return S;
}

static RGB oklab_to_linear_srgb(Lab lab) {

	float l_ = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
	float m_ = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
	float s_ = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

	float l = l_ * l_ * l_;
	float m = m_ * m_ * m_;
	float s = s_ * s_ * s_;

	return RGB{
		4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
		-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
		-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
	};

}


// finds L_cusp and C_cusp for a given hue
// a and b must be normalized so a^2 + b^2 == 1
struct LC { float L; float C; };
LC find_cusp(float a, float b)
{
	// First, find the maximum saturation (saturation S = C/L)
	float S_cusp = compute_max_saturation(a, b);

	// Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
	RGB rgb_at_max = oklab_to_linear_srgb({ 1, S_cusp * a, S_cusp * b });
	float L_cusp = cbrtf(1.f / std::fmax(std::fmax(rgb_at_max.r, rgb_at_max.g), rgb_at_max.b));
	float C_cusp = L_cusp * S_cusp;

	return { L_cusp , C_cusp };
}

// Finds intersection of the line defined by 
// L = L0 * (1 - t) + t * L1;
// C = t * C1;
// a and b must be normalized so a^2 + b^2 == 1
float find_gamut_intersection(float a, float b, float L1, float C1, float L0)
{
	// Find the cusp of the gamut triangle
	LC cusp = find_cusp(a, b);

	// Find the intersection for upper and lower half seprately
	float t;
	if (((L1 - L0) * cusp.C - (cusp.L - L0) * C1) <= 0.f)
	{
		// Lower half

		t = cusp.C * L0 / (C1 * cusp.L + cusp.C * (L0 - L1));
	}
	else
	{
		// Upper half

		// First intersect with triangle
		t = cusp.C * (L0 - 1.f) / (C1 * (cusp.L - 1.f) + cusp.C * (L0 - L1));

		// Then one step Halley's method
		{
			float dL = L1 - L0;
			float dC = C1;

			float k_l = +0.3963377774f * a + 0.2158037573f * b;
			float k_m = -0.1055613458f * a - 0.0638541728f * b;
			float k_s = -0.0894841775f * a - 1.2914855480f * b;

			float l_dt = dL + dC * k_l;
			float m_dt = dL + dC * k_m;
			float s_dt = dL + dC * k_s;


			// If higher accuracy is required, 2 or 3 iterations of the following block can be used:
			{
				float L = L0 * (1.f - t) + t * L1;
				float C = t * C1;

				float l_ = L + C * k_l;
				float m_ = L + C * k_m;
				float s_ = L + C * k_s;

				float l = l_ * l_ * l_;
				float m = m_ * m_ * m_;
				float s = s_ * s_ * s_;

				float ldt = 3 * l_dt * l_ * l_;
				float mdt = 3 * m_dt * m_ * m_;
				float sdt = 3 * s_dt * s_ * s_;

				float ldt2 = 6 * l_dt * l_dt * l_;
				float mdt2 = 6 * m_dt * m_dt * m_;
				float sdt2 = 6 * s_dt * s_dt * s_;

				float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1;
				float r1 = 4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
				float r2 = 4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

				float u_r = r1 / (r1 * r1 - 0.5f * r * r2);
				float t_r = -r * u_r;

				float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1;
				float g1 = -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
				float g2 = -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

				float u_g = g1 / (g1 * g1 - 0.5f * g * g2);
				float t_g = -g * u_g;

				float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1;
				float b1 = -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
				float b2 = -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

				float u_b = b1 / (b1 * b1 - 0.5f * b * b2);
				float t_b = -b * u_b;

				t_r = u_r >= 0.f ? t_r : FLT_MAX;
				t_g = u_g >= 0.f ? t_g : FLT_MAX;
				t_b = u_b >= 0.f ? t_b : FLT_MAX;

				t += std::fmin(t_r, std::fmin(t_g, t_b));
			}
		}
	}

	return t;
}


static RGB gamut_clip_preserve_chroma(RGB rgb)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
		return rgb;

	Lab lab = linear_srgb_to_oklab(&rgb);

	float L = lab.L;
	float eps = 0.00001f;
	float C = std::fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
	float a_ = lab.a / C;
	float b_ = lab.b / C;

	float L0 = clamp(L, 0, 1);

	float t = find_gamut_intersection(a_, b_, L, C, L0);
	float L_clipped = L0 * (1 - t) + t * L;
	float C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

float sgn(float x)
{
	return (float)(0.f < x) - (float)(x < 0.f);
}

RGB gamut_clip_adaptive_L0_L_cusp(RGB rgb, float alpha = 0.05f)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
		return rgb;

	Lab lab = linear_srgb_to_oklab(&rgb);

	float L = lab.L;
	//float eps = 0.00001f;
	float eps = 0.0000f;
	float C = std::fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
	float a_ = lab.a / C;
	float b_ = lab.b / C;

	// The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
	LC cusp = find_cusp(a_, b_);

	float Ld = L - cusp.L;
	float k = 2.f * (Ld > 0 ? 1.f - cusp.L : cusp.L);

	float e1 = 0.5f * k + fabs(Ld) + alpha * C / k;
	float tosqrt = e1 * e1 - 2.f * k * fabs(Ld);
	if (tosqrt < 0) tosqrt = 0;
	float L0 = cusp.L + 0.5f * (sgn(Ld) * (e1 - sqrtf(tosqrt)));

	float t = find_gamut_intersection(a_, b_, L, C, L0);
	t = clamp(t, 0.f, 1.f); // XXX: why is this needed?????
	float L_clipped = L0 * (1.f - t) + t * L;
	float C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

static void
OKtoRGB(
	ClipType	clip,
	float		adaptiveClipAlpha,
	const Lab	*inP,
	RGB			*outP
)
{
	*outP = oklab_to_linear_srgb(*inP);

	bool clipped =
		outP->r > 1.f || outP->r < 0.f ||
		outP->g > 1.f || outP->g < 0.f ||
		outP->b > 1.f || outP->b < 0.f;


	if (clipped) {
		switch (clip) {
		case CLIP_TYPE_ADAPTIVE_HUE_DEPENDENT:
			*outP = gamut_clip_adaptive_L0_L_cusp(*outP, adaptiveClipAlpha);
			outP->r = MIN(MAX(outP->r, 0.f), 1.f);
			outP->g = MIN(MAX(outP->g, 0.f), 1.f);
			outP->b = MIN(MAX(outP->b, 0.f), 1.f);
			break;
		case CLIP_TYPE_CHROMA_CLIP:
			*outP = gamut_clip_preserve_chroma(*outP);
			break;
		case CLIP_TYPE_TRUNCATE:
			outP->r = MIN(MAX(outP->r, 0.f), 1.f);
			outP->g = MIN(MAX(outP->g, 0.f), 1.f);
			outP->b = MIN(MAX(outP->b, 0.f), 1.f);
			break;
		case CLIP_TYPE_DEBUG:
			outP->r = 1.f;
			outP->g = 0.f;
			outP->b = 0.f;
			break;
		default:
			// bug!!
			break;
		}
	}
};

static void
HueChroma(
	float hueRadians,
	float chromaScale,
	const Lab* inP,
	Lab* outP
)
{
	float hue = (atan2f(inP->b, inP->a) + hueRadians); // does hue rotate??
	//float C = sqrtf(powf(inP->a, 2) + powf(inP->b, 2)) * chromaScale; MULT
	float C = sqrtf(powf(inP->a, 2) + powf(inP->b, 2)) + chromaScale;
	C = MAX(C, 0);
	*outP = Lab{ inP->L, C * cosf(hue), C * sinf(hue) }; // outputs struct Lab d
};

static PF_Err
HueFunc16 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
	PF_Err		err = PF_Err_NONE;

	UserInput* giP = reinterpret_cast<UserInput*>(refcon);
	//new function here
	if (giP->hueRad != CHR0MA_HUE_DFLT || giP->chromaScale != CHR0MA_CHROMA_DFLT) {
		//test fx
		/*outP->alpha = inP->alpha;
		outP->red = 2;
		outP->green = 12567;
		outP->blue = 17883;*/
		
		//"real" fx		
		RGB inRgb = { float(inP->red) / PF_MAX_CHAN16 , float(inP->green) / PF_MAX_CHAN16 , float(inP->blue) / PF_MAX_CHAN16 }; // sets up struct with inP RGB values
		Lab outLab;
		RGB outRgb;
		Lab inLab = linear_srgb_to_oklab(&inRgb);
		HueChroma(giP->hueRad, giP->chromaScale, &inLab, &outLab);
		OKtoRGB(giP->clipType, giP->adaptiveClipAlpha, &outLab, &outRgb);

		outP->red	= (A_u_short)(outRgb.r * PF_MAX_CHAN16);
		outP->green = (A_u_short)(outRgb.g * PF_MAX_CHAN16);
		outP->blue	= (A_u_short)(outRgb.b * PF_MAX_CHAN16);
		outP->alpha = inP->alpha;
	}
	else {
		*outP = *inP;
	}
	return err;

	//OLD gain function below

	//GainInfo	*giP	= reinterpret_cast<GainInfo*>(refcon);
	//PF_FpLong	tempF	= 0;
	//
	//				
	//if (giP){
	//	tempF = giP->gainF * PF_MAX_CHAN16 / 100.0; 
	//	if (tempF > PF_MAX_CHAN16){
	//		tempF = PF_MAX_CHAN16;				//puts input in 16 bit range
	//	};

	//	outP->alpha		=	inP->alpha;
	//	outP->red		=	MIN((inP->red	+ (A_u_char) tempF), PF_MAX_CHAN16); // 'red' reads the red channel of inP as an unsigned short (aka 16bit)
	//	outP->green		=	MIN((inP->green	+ (A_u_char) tempF), PF_MAX_CHAN16); // A_u_char casts gain to 255 range 
	//	outP->blue		=	MIN((inP->blue	+ (A_u_char) tempF), PF_MAX_CHAN16);
	//}

	//

	//return err;
}

static PF_Err
HueFunc8 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel8	*inP, 
	PF_Pixel8	*outP)
{
	PF_Err		err = PF_Err_NONE;

	UserInput* giP = reinterpret_cast<UserInput*>(refcon);
	//new function here
	if (giP->hueRad != CHR0MA_HUE_DFLT || giP->chromaScale != CHR0MA_CHROMA_DFLT) {
		//"real" fx		
		RGB inRgb = { float(inP->red) / PF_MAX_CHAN8 , float(inP->green) / PF_MAX_CHAN8 , float(inP->blue) / PF_MAX_CHAN8 }; // sets up struct with inP RGB values
		Lab inLab, outLab;
		RGB outRgb;
		inLab = linear_srgb_to_oklab(&inRgb);
		HueChroma(giP->hueRad, giP->chromaScale, &inLab, &outLab);
		OKtoRGB(giP->clipType, giP->adaptiveClipAlpha, &outLab, &outRgb);

		outP->red	= (A_u_char)(outRgb.r * PF_MAX_CHAN8);
		outP->green = (A_u_char)(outRgb.g * PF_MAX_CHAN8);
		outP->blue	= (A_u_char)(outRgb.b * PF_MAX_CHAN8);
		outP->alpha = inP->alpha;
	}
	else {
		*outP = *inP;
	}
	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	UserInput			giP;
	AEFX_CLR_STRUCT(giP); // clears parameters before adding them
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top; // defines the area that needs to be rendered?

	PF_Fixed hueFixed = params[CHR0MA_HUE]->u.ad.value;
	giP.hueRad 	= 
		(float(hueFixed >> 16) + (float((hueFixed & 0xffff) / 0x10000))) / 180.f * M_PI; // sets gainF in giP to be the user input

	giP.chromaScale = params[CHR0MA_CHROMA]->u.fs_d.value / 1000.f; // making slider 1000x "real" value so numbers are in a reasonable range
	giP.clipType = (ClipType)params[CHR0MA_CLIP]->u.pd.value;
	giP.adaptiveClipAlpha = params[CHROMA_ADAPTIVE_CLIP_ALPHA]->u.fs_d.value;

	if (PF_WORLD_IS_DEEP(output)){
		ERR(suites.Iterate16Suite2()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												HueFunc16,						// pixel function pointer
												output));
	} else {
		ERR(suites.Iterate8Suite2()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												HueFunc8,						// pixel function pointer
												output));	
	}

	return err;
}

static PF_Err
UserChangedParam(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	if ((ClipType)params[CHR0MA_CLIP]->u.pd.value != CLIP_TYPE_ADAPTIVE_HUE_DEPENDENT) {
		params[CHROMA_ADAPTIVE_CLIP_ALPHA]->ui_flags = PF_PUI_INVISIBLE;
	}

	return PF_Err_NONE;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction2(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB2 inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(
		inPtr,
		inPluginDataCallBackPtr,
		"CHR0MA", // Name
		"temberlin CHR0MA", // Match Name
		"Color Coreection", // Category
		AE_RESERVED_INFO, // Reserved Info
		"EffectMain",	// Entry point
		"https://temberl.in/");	// support URL

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;

			case PF_Cmd_USER_CHANGED_PARAM:

				err = UserChangedParam(	in_data, 
										out_data, 
										params, 
										output);
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

