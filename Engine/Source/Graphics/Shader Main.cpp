/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
/******************************************************************************/
MainShaderClass    Sh;
AmbientOcclusion   AO;
   LayeredCloudsFx LC;
VolumetricCloudsFx VolCloud;
VolumetricLights   VL;
HDR                Hdr;
MotionBlur         Mtn;
DepthOfField       Dof;
WaterShader        WS;

ShaderImage::Sampler SamplerPoint, SamplerLinearWrap, SamplerLinearWCC, SamplerLinearCWC, SamplerLinearCWW, SamplerLinearClamp, SamplerFont, SamplerAnisotropic, SamplerShadowMap;
/******************************************************************************/
// MAIN SHADER
/******************************************************************************/
#if DX11
void CreateAnisotropicSampler()
{
   D3D11_SAMPLER_DESC  sd; Zero(sd);
   if(D.texFilter()> 1)sd.Filter=D3D11_FILTER_ANISOTROPIC;else
   if(D.texFilter()==1)sd.Filter=(D.texMipFilter() ? D3D11_FILTER_MIN_MAG_MIP_LINEAR       : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT       );else
                       sd.Filter=(D.texMipFilter() ? D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT : D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
   sd.AddressU=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.MipLODBias    =0;
   sd.MaxAnisotropy =Mid(D.texFilter(), 1, 16);
   sd.MinLOD        =Max(D.texLod   (), 0    );
   sd.MaxLOD        =FLT_MAX;
   sd.ComparisonFunc=D3D11_COMPARISON_NEVER;
   SamplerAnisotropic.create(sd);
}
void CreateFontSampler()
{
   D3D11_SAMPLER_DESC sd; Zero(sd);
   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.MipLODBias    =-D.fontSharpness();
   sd.MaxAnisotropy =1;
   sd.MinLOD        =0;
   sd.MaxLOD        =FLT_MAX;
   sd.ComparisonFunc=D3D11_COMPARISON_NEVER;
   SamplerFont.create(sd);
}
#endif
/******************************************************************************/
static Vec4 DummyData[2]; ASSERT(SIZE(DummyData)==MIN_SHADER_PARAM_DATA_SIZE);
MainShaderClass::MainShaderClass()
{
   // 'Dummy' is used so we can already pre-assign some shader handles to this 'Dummy' at engine startup, so we don't have to check if they're not null "if(Sh.param)Sh.param->set(..)" but just operate on them straight away "Sh.param->set(..)"
   Dummy._data   =(Byte*)DummyData;
   Dummy._changed=(Bool*)DummyData;
}
/******************************************************************************/
void MainShaderClass::del()
{
   // delete all to detect GPU memory leaks when using D3D_DEBUG
   shader        =null;
   ShaderFiles  .del();
   ShaderParams .del(); // params before buffers, because they point to them
   ShaderBuffers.del();
   ShaderImages .del();

   SamplerPoint      .del();
   SamplerLinearWrap .del();
   SamplerLinearWCC  .del();
   SamplerLinearCWC  .del();
   SamplerLinearCWW  .del();
   SamplerLinearClamp.del();
   SamplerFont       .del();
   SamplerAnisotropic.del();
   SamplerShadowMap  .del();
}
void MainShaderClass::createSamplers()
{
#if DX11
   D3D11_SAMPLER_DESC sd; Zero(sd);
   sd.MipLODBias    =0;
   sd.MaxAnisotropy =1;
   sd.MinLOD        =0;
   sd.MaxLOD        =FLT_MAX;
   sd.ComparisonFunc=D3D11_COMPARISON_NEVER;

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_POINT;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   SamplerPoint.create(sd);

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
   SamplerLinearWrap.create(sd);

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   SamplerLinearWCC.create(sd);

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   SamplerLinearCWC.create(sd);

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
   SamplerLinearCWW.create(sd);

   sd.Filter  =D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   SamplerLinearClamp.create(sd);

   sd.Filter  =D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
   sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   sd.ComparisonFunc=(REVERSE_DEPTH ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL);
   SamplerShadowMap.create(sd);
   
   CreateAnisotropicSampler();
   CreateFontSampler       ();
#elif GL
   REPAO(SamplerPoint.filter )=GL_NEAREST;
   REPAO(SamplerPoint.address)=GL_CLAMP_TO_EDGE;

   REPAO(SamplerLinearWrap.filter )=GL_LINEAR;
   REPAO(SamplerLinearWrap.address)=GL_REPEAT;

   REPAO(SamplerLinearWCC.filter )=GL_LINEAR;
   REPAO(SamplerLinearWCC.address)=GL_CLAMP_TO_EDGE; SamplerLinearWCC.address[0]=GL_REPEAT;

   REPAO(SamplerLinearCWC.filter )=GL_LINEAR;
   REPAO(SamplerLinearCWC.address)=GL_CLAMP_TO_EDGE; SamplerLinearCWC.address[1]=GL_REPEAT;

   REPAO(SamplerLinearCWW.filter )=GL_LINEAR;
   REPAO(SamplerLinearCWW.address)=GL_REPEAT; SamplerLinearCWW.address[0]=GL_CLAMP_TO_EDGE;

   REPAO(SamplerLinearClamp.filter )=GL_LINEAR;
   REPAO(SamplerLinearClamp.address)=GL_CLAMP_TO_EDGE;

   REPAO(SamplerFont.filter )=GL_LINEAR;
   REPAO(SamplerFont.address)=GL_CLAMP_TO_EDGE;
#endif
}
void MainShaderClass::create()
{
   if(LogInit)LogN("MainShaderClass.create");
   compile();

   if(D.shaderModel()>=SM_4)path="Shader\\4\\" ;else
                            path="Shader\\GL\\";

   shader=ShaderFiles("Main"); if(!shader)Exit("Can't load the Main Shader");
#define MEASURE_SHADER_LOAD_TIME 0
#if     MEASURE_SHADER_LOAD_TIME
 //#define SLOW_SHADER_LOAD 0
   #pragma message("!! Warning: Use this only for debugging !!")
   Flt t=Time.curTime();
#endif
   getTechniques();
#if MEASURE_SHADER_LOAD_TIME
   Exit(S+"Shaders Loaded in: "+(Time.curTime()-t)+'s');
#endif
   connectRT    ();
}
/******************************************************************************/
void MainShaderClass::clear(                C   Vec4  &color,                       C Rect *rect) {Sh.Color[0]->set(color);                              Sh.SetCol->draw(       rect);}
void MainShaderClass::draw (C Image &image                  ,                       C Rect *rect) {                                                      Sh.Draw  ->draw(image, rect);}
void MainShaderClass::draw (C Image &image, C   Vec4  &color, C   Vec4  &color_add, C Rect *rect) {Sh.Color[0]->set(color); Sh.Color[1]->set(color_add); Sh.DrawC ->draw(image, rect);}
void MainShaderClass::draw (C Image &image, C ::Color &color, C ::Color &color_add, C Rect *rect) {Sh.Color[0]->set(color); Sh.Color[1]->set(color_add); Sh.DrawC ->draw(image, rect);}
/******************************************************************************/
Shader* MainShaderClass::getBloomDS(Bool glow, Bool uv_clamp, Bool half_res, Bool saturate, Bool gamma) {return get(S8+"BloomDS"+glow+uv_clamp+half_res+saturate+gamma);}
Shader* MainShaderClass::getBloom  (Bool dither, Bool gamma                                           ) {return get(S8+"Bloom"  +dither+gamma);}

Shader* MainShaderClass::getShdDir  (Int map_num, Bool clouds, Bool multi_sample) {return get(S8+"ShdDir"  +multi_sample+map_num+clouds);}
Shader* MainShaderClass::getShdPoint(                          Bool multi_sample) {return get(S8+"ShdPoint"+multi_sample);}
Shader* MainShaderClass::getShdCone (                          Bool multi_sample) {return get(S8+"ShdCone" +multi_sample);}

Shader* MainShaderClass::getDrawLightDir   (Bool shadow, Bool multi_sample, Bool quality            ) {return get(S8+"DrawLightDir"   +shadow+multi_sample+(quality && !multi_sample)            );} // MSAA doesn't have quality version (to make it faster)
Shader* MainShaderClass::getDrawLightPoint (Bool shadow, Bool multi_sample, Bool quality            ) {return get(S8+"DrawLightPoint" +shadow+multi_sample+(quality && !multi_sample)      +GL_ES);} // MSAA doesn't have quality version (to make it faster)
Shader* MainShaderClass::getDrawLightLinear(Bool shadow, Bool multi_sample, Bool quality            ) {return get(S8+"DrawLightLinear"+shadow+multi_sample+(quality && !multi_sample)      +GL_ES);} // MSAA doesn't have quality version (to make it faster)
Shader* MainShaderClass::getDrawLightCone  (Bool shadow, Bool multi_sample, Bool quality, Bool image) {return get(S8+"DrawLightCone"  +shadow+multi_sample+(quality && !multi_sample)+image+GL_ES);} // MSAA doesn't have quality version (to make it faster)
#if !DEPTH_CLIP_SUPPORTED
Shader* MainShaderClass::getDrawLightConeFlat(Bool shadow, Bool multi_sample, Bool quality, Bool image) {return get(S8+"DrawLightConeFlat"+shadow+multi_sample+(quality && !multi_sample)+image);} // MSAA doesn't have quality version (to make it faster)
#endif

Shader* MainShaderClass::getApplyLight(Int multi_sample, Bool ao, Bool cel_shade, Bool night_shade, Bool glow) {return get(S8+"ApplyLight"+multi_sample+ao+cel_shade+night_shade+glow);}

Shader* MainShaderClass::getSunRaysMask(Bool mask                                      ) {return get(S8+"SunRaysMask"+mask);}
Shader* MainShaderClass::getSunRays    (Bool mask, Bool dither, Bool jitter, Bool gamma) {return get(S8+"SunRays"    +mask+dither+jitter+gamma);}

Shader* MainShaderClass::getSky(Int multi_sample, Bool flat, Bool density, Int textures, Bool stars, Bool dither, Bool per_vertex, Bool cloud) {return get(S8+"Sky"+multi_sample+flat+density+textures+stars+dither+per_vertex+cloud);}

Shader* MainShaderClass::getSkyTF(                  Int  textures  ,                           Bool dither, Bool cloud) {Int multi_sample=0;                 Bool flat=true , density=false, stars=false, per_vertex=false; return getSky(multi_sample, flat, density, textures, stars, dither, per_vertex, cloud);}
Shader* MainShaderClass::getSkyT (Int multi_sample, Int  textures  ,                           Bool dither, Bool cloud) {                                    Bool flat=false, density=false, stars=false, per_vertex=false; return getSky(multi_sample, flat, density, textures, stars, dither, per_vertex, cloud);}
Shader* MainShaderClass::getSkyAF(                  Bool per_vertex,               Bool stars, Bool dither, Bool cloud) {Int multi_sample=0; Int textures=0; Bool flat=true , density=false                               ; return getSky(multi_sample, flat, density, textures, stars, dither, per_vertex, cloud);}
Shader* MainShaderClass::getSkyA (Int multi_sample, Bool per_vertex, Bool density, Bool stars, Bool dither, Bool cloud) {                    Int textures=0; Bool flat=false                                              ; return getSky(multi_sample, flat, density, textures, stars, dither, per_vertex, cloud);}

void MainShaderClass::initCubicShaders()
{
   if(!DrawTexCubic[0]) // check if not yet initialized because this is called multiple times for SLOW_SHADER_LOAD
   {
      REPD(color , 2)DrawTexCubic[color]=get(S8+"DrawTexCubic"+color);
      REPD(dither, 2)
      {
         DrawTexCubicF   [dither]=get(S8+"DrawTexCubicF"   +dither);
         DrawTexCubicFRGB[dither]=get(S8+"DrawTexCubicFRGB"+dither);
      }
   }
}
void MainShaderClass::loadFogBoxShaders()
{
   if(!FogBox) // check if not yet initialized because this is called multiple times
   {
      ShaderFile &sf=*ShaderFiles("Fog Local");
      FogBox =sf.get("FogBox"  );
      FogBox0=sf.get("FogBoxI0");
      FogBox1=sf.get("FogBoxI1");
   }
}
void MainShaderClass::loadFogHeightShaders()
{
   if(!FogHeight) // check if not yet initialized because this is called multiple times
   {
      ShaderFile &sf=*ShaderFiles("Fog Local");
      FogHeight =sf.get("FogHeight"  );
      FogHeight0=sf.get("FogHeightI0");
      FogHeight1=sf.get("FogHeightI1");
   }
}
void MainShaderClass::loadFogBallShaders()
{
   if(!FogBall) // check if not yet initialized because this is called multiple times
   {
      ShaderFile &sf=*ShaderFiles("Fog Local");
      FogBall =sf.get("FogBall"  );
      FogBall0=sf.get("FogBallI0");
      FogBall1=sf.get("FogBallI1");
   }
}

void MainShaderClass::getTechniques()
{
   // images
   Img[0]=ShaderImages("Img" );
   Img[1]=ShaderImages("Img1");
   Img[2]=ShaderImages("Img2");
   Img[3]=ShaderImages("Img3");

   ImgMS[0]=ShaderImages("ImgMS" );
   ImgMS[1]=ShaderImages("ImgMS1");

   ImgX[0]  =ShaderImages("ImgX"  );
   ImgX[1]  =ShaderImages("ImgX1" );
   ImgX[2]  =ShaderImages("ImgX2" );
   ImgX[3]  =ShaderImages("ImgX3" );
   ImgXMS   =ShaderImages("ImgXMS");
   ImgXF[0] =ShaderImages("ImgXF" );
   ImgXF[1] =ShaderImages("ImgXF1");
   ImgXY    =ShaderImages("ImgXY" );

   Cub[0]=ShaderImages("Cub");
   Cub[1]=ShaderImages("Cub1");

   Vol     =ShaderImages("Vol"    );
   VolXY[0]=ShaderImages("VolXY"  );
   VolXY[1]=ShaderImages("VolXY1" );

   Depth    =ShaderImages("Depth"  );
   DepthMS  =ShaderImages("DepthMS");
   ShdMap[0]=ShaderImages("ShdMap" ); ShdMap[0]->_sampler=&SamplerLinearClamp;
   ShdMap[1]=ShaderImages("ShdMap1"); ShdMap[1]->_sampler=&SamplerLinearClamp;

   // material textures
   Col[0]=ShaderImages("Col" );
   Col[1]=ShaderImages("Col1");
   Col[2]=ShaderImages("Col2");
   Col[3]=ShaderImages("Col3");
   Nrm[0]=ShaderImages("Nrm" );
   Nrm[1]=ShaderImages("Nrm1");
   Nrm[2]=ShaderImages("Nrm2");
   Nrm[3]=ShaderImages("Nrm3");
   Det[0]=ShaderImages("Det" );
   Det[1]=ShaderImages("Det1");
   Det[2]=ShaderImages("Det2");
   Det[3]=ShaderImages("Det3");
   Mac[0]=ShaderImages("Mac" );
   Mac[1]=ShaderImages("Mac1");
   Mac[2]=ShaderImages("Mac2");
   Mac[3]=ShaderImages("Mac3");
   Rfl[0]=ShaderImages("Rfl" );
   Rfl[1]=ShaderImages("Rfl1");
   Rfl[2]=ShaderImages("Rfl2");
   Rfl[3]=ShaderImages("Rfl3");
   Lum   =ShaderImages("Lum" );

   ImgSize         =GetShaderParam("ImgSize" );
   ImgClamp        =GetShaderParam("ImgClamp");
   RTSize          =GetShaderParam("RTSize"  );
   Coords          =GetShaderParam("Coords"  );
   Viewport        =GetShaderParam("Viewport");
   DepthWeightScale=GetShaderParam("DepthWeightScale");

   CamAngVel =GetShaderParam("CamAngVel" );
   ObjVel    =GetShaderParam("ObjVel"    );
   ViewMatrix=GetShaderParam("ViewMatrix");
   CamMatrix =GetShaderParam("CamMatrix" );
   ProjMatrix=GetShaderParam("ProjMatrix");
   FurVel    =GetShaderParam("FurVel"    );
   ClipPlane =GetShaderParam("ClipPlane" );

            ConstCast(Renderer.highlight       )=GetShaderParam("Highlight");
   Material=ConstCast(Renderer.material_color_l)=GetShaderParam("Material");
   MultiMaterial[0]=GetShaderParam("MultiMaterial0");
   MultiMaterial[1]=GetShaderParam("MultiMaterial1");
   MultiMaterial[2]=GetShaderParam("MultiMaterial2");
   MultiMaterial[3]=GetShaderParam("MultiMaterial3");

   LightDir   =GetShaderParam("LightDir"   );
   LightPoint =GetShaderParam("LightPoint" );
   LightLinear=GetShaderParam("LightLinear");
   LightCone  =GetShaderParam("LightCone"  );

   Step      =GetShaderParam("Step"      );
   Color[0]  =GetShaderParam("Color[0]"  );
   Color[1]  =GetShaderParam("Color[1]"  );
   BehindBias=GetShaderParam("BehindBias");

   VtxSkinning =GetShaderParamBool("VtxSkinning" );
   VtxHeightmap=GetShaderParam    ("VtxHeightmap");

   LightMapScale=GetShaderParam("LightMapScale");

   GrassRangeMulAdd=GetShaderParam("GrassRangeMulAdd");
   BendFactor      =GetShaderParam("BendFactor");

   NightShadeColor   =GetShaderParam    ("NightShadeColor"   ); // set in 'D.ambientSet()'
   AmbientColor_l    =GetShaderParam    ("AmbColor"          ); // set in 'D.ambientSet()'
   AmbientColorNS_l  =GetShaderParam    ("AmbNSColor"        ); // set in 'D.ambientSet()'
   AmbientMaterial   =GetShaderParamBool("AmbMaterial"       );
   AmbientContrast   =GetShaderParam    ("AmbientContrast"   ); AmbientContrast->set(D.ambientContrast());
   AmbientMin        =GetShaderParam    ("AmbientMin"        ); AmbientMin     ->set(D.ambientMin     ());
   AmbientRange_2    =GetShaderParam    ("AmbientRange_2"    );
   AmbientRangeInvSqr=GetShaderParam    ("AmbientRangeInvSqr");
   D.ambientSet(); D.ambientSetRange();

   HdrBrightness=GetShaderParam("HdrBrightness"); HdrBrightness->set(D.eyeAdaptationBrightness());
   HdrExp       =GetShaderParam("HdrExp"       ); HdrExp       ->set(D.eyeAdaptationExp       ());
   HdrMaxDark   =GetShaderParam("HdrMaxDark"   ); HdrMaxDark   ->set(D.eyeAdaptationMaxDark   ());
   HdrMaxBright =GetShaderParam("HdrMaxBright" ); HdrMaxBright ->set(D.eyeAdaptationMaxBright ());
   HdrWeight    =GetShaderParam("HdrWeight"    ); HdrWeight    ->set(D.eyeAdaptationWeight()/4  );

   TesselationDensity=GetShaderParam("TesselationDensity"); TesselationDensity->set(D.tesselationDensity());

   // TECHNIQUES
   Draw2DFlat  =get("Draw2DFlat");
   Draw3DFlat  =get("Draw3DFlat");
   Draw2DCol   =get("Draw2DCol");
   Draw3DCol   =get("Draw3DCol");
   Draw2DTex   =get("Draw2DTex");
   Draw2DTexC  =get("Draw2DTexC");
   Draw2DTexCol=get("Draw2DTexCol");
   REPD(alpha_test, 2)
   REPD(color     , 2)
   {
                  Draw2DDepthTex[alpha_test][color]     =get(S8+"Draw2DDepthTex"+alpha_test+color);
      REPD(fog, 2)Draw3DTex     [alpha_test][color][fog]=get(S8+"Draw3DTex"     +alpha_test+color+fog);
   }
   PaletteDraw=get("PaletteDraw");
   Simple     =get("Simple");

   DrawX =get("DrawX");
   DrawXG=get("DrawXG");
   REPD(dither, 2)
   REPD(gamma , 2)DrawXC[dither][gamma]=get(S8+"DrawXC"+dither+gamma);

 //DrawTexX  =get("DrawTexX"); used by Editor
 //DrawTexY  =get("DrawTexY"); used by Editor
 //DrawTexZ  =get("DrawTexZ"); used by Editor
 //DrawTexW  =get("DrawTexW"); used by Editor
 //DrawTexXG =get("DrawTexXG"); used by Editor
 //DrawTexYG =get("DrawTexYG"); used by Editor
 //DrawTexZG =get("DrawTexZG"); used by Editor
 //DrawTexWG =get("DrawTexWG"); used by Editor
 //DrawTexNrm=get("DrawTexNrm"); used by Editor

   DrawMask=get("DrawMask");

   // POINT (these can be null if failed to load)
   DrawTexPoint =find("DrawTexPoint");
   DrawTexPointC=find("DrawTexPointC");

   // CUBIC (these can be null if failed to load)
   REPD(color , 2)DrawTexCubicFast[color]=get(S8+"DrawTexCubicFast"+color);
   REPD(dither, 2)
   {
      DrawTexCubicFastF   [dither]=find(S8+"DrawTexCubicFastF"   +dither);
      DrawTexCubicFastFRGB[dither]=find(S8+"DrawTexCubicFastFRGB"+dither);
   }
#if !SLOW_SHADER_LOAD
   initCubicShaders();
#endif

   // FONT
   FontShadow  =GetShaderParam("FontShadow"  );
   FontLum     =GetShaderParam("FontLum"     );
   FontContrast=GetShaderParam("FontContrast");
   FontShade   =GetShaderParam("FontShade"   );
   FontDepth   =GetShaderParam("FontDepth"   );

   REPD(depth, 2)
   REPD(gamma, 2)
   {
      Font  [depth][gamma]=get(S8+"Font"  +depth+gamma);
      FontSP[depth][gamma]=get(S8+"FontSP"+depth+gamma);
   }

   // BASIC 2D
   Dither=get("Dither");
   SetCol=get("SetCol");
   Draw  =get("Draw"  );
   DrawC =get("DrawC" );
   DrawG =get("DrawG" );
   DrawCG=get("DrawCG");
   DrawA =get("DrawA" );
   if(D.shaderModel()>=SM_4)
   {
                                 DrawMs1=get("DrawMs1");
                                 DrawMsN=get("DrawMsN");
      if(D.shaderModel()>=SM_4_1)DrawMsM=get("DrawMsM");
   }

   // BLOOM
   BloomParams=GetShaderParam("BloomParams");
#if !SLOW_SHADER_LOAD
   REPD(glow, 2)
   REPD(c, 2)
   REPD(h, 2)
   REPD(s, 2)
   REPD(gamma, 2)
      BloomDS[glow][c][h][s][gamma]=getBloomDS(glow, c, h, s, gamma);

   REPD(dither, 2)
   REPD(gamma , 2)
      Bloom[dither][gamma]=getBloom(dither, gamma);
#endif

   // BLUR
   REPD(samples, 2)
   {
      BlurX[samples]=get(S8+"BlurX"+(samples?6:4));
      BlurY[samples]=get(S8+"BlurY"+(samples?6:4));
   }

   // DEPTH
   REPD(m, (D.shaderModel()>=SM_4_1) ? 3 : (D.shaderModel()>=SM_4) ? 2 : 1)
   REPD(p, 2)LinearizeDepth[m][p]=get(S8+"LinearizeDepth"+m+p);

   SetDepth=get("SetDepth");

   if(D.shaderModel()>=SM_4)
   {
      ResolveDepth=get("ResolveDepth");
      DetectMSCol =get("DetectMSCol");
    //DetectMSNrm =get("DetectMSNrm");
   }

#if !SLOW_SHADER_LOAD
                            Combine       =get("Combine0");
                            CombineSS     =get("Combine1");
   if(D.shaderModel()>=SM_4)CombineMS     =get("Combine2");
                            CombineSSAlpha=get("CombineSSAlpha");
#endif

   // SKY
   Sun            =GetShaderParam("Sun"            );
   SkyDnsExp      =GetShaderParam("SkyDnsExp"      );
   SkyHorExp      =GetShaderParam("SkyHorExp"      );
   SkyBoxBlend    =GetShaderParam("SkyBoxBlend"    );
   SkyHorCol      =GetShaderParam("SkyHorCol"      );
   SkySkyCol      =GetShaderParam("SkySkyCol"      );
   SkyStarOrn     =GetShaderParam("SkyStarOrn"     );
   SkyFracMulAdd  =GetShaderParam("SkyFracMulAdd"  );
   SkyDnsMulAdd   =GetShaderParam("SkyDnsMulAdd"   );
   SkySunHighlight=GetShaderParam("SkySunHighlight");
   SkySunPos      =GetShaderParam("SkySunPos"      );
#if !SLOW_SHADER_LOAD
   REPD(dither, 2)
   REPD(cloud , 2)
   {
      // Textures Flat
      REPD(textures, 2)SkyTF[textures][dither][cloud]=getSkyTF(textures+1, dither, cloud);

      // Atmospheric Flat
      REPD(per_vertex, 2)
      REPD(stars     , 2)SkyAF[per_vertex][stars][dither][cloud]=getSkyAF(per_vertex, stars, dither, cloud);

      REPD(multi_sample, (D.shaderModel()>=SM_4_1) ? 3 : (D.shaderModel()>=SM_4) ? 2 : 1)
      {
         // Textures
         REPD(textures, 2)SkyT[multi_sample][textures][dither][cloud]=getSkyT(multi_sample, textures+1, dither, cloud);

         // Atmospheric
         REPD(per_vertex, 2)
         REPD(density   , 2)
         REPD(stars     , 2)SkyA[multi_sample][per_vertex][density][stars][dither][cloud]=getSkyA(multi_sample, per_vertex, density, stars, dither, cloud);
      }
   }

   REPD(m, 2)SunRaysMask[m]=getSunRaysMask(m);

   REPD(m, 2)
   REPD(d, 2)
   REPD(j, 2)
   REPD(g, 2)
      SunRays[m][d][j][g]=getSunRays(m, d, j, g);
 //SunRaysSoft=get("SunRaysSoft");
#endif

   // SHADOWS
   REPAO(ShdStep      )=GetShaderParam(S8+"ShdStep["+i+']');
         ShdJitter     =GetShaderParam("ShdJitter");
         ShdRange      =GetShaderParam("ShdRange");
         ShdRangeMulAdd=GetShaderParam("ShdRangeMulAdd");
         ShdOpacity    =GetShaderParam("ShdOpacity");
         ShdMatrix     =GetShaderParam("ShdMatrix");
   REPAO(ShdMatrix4   )=GetShaderParam(S8+"ShdMatrix4["+i+']');

   // can be used for shadows in deferred and AO
   {
      Byte gl_es=(GL_ES ? D.gatherAvailable() ? 2 : 1 : 0); // GL_ES can't filter depth textures, so have to use special shader based on gather, all other platforms just use depth filtering, so no need, also GL_ES doesn't support NOPERSP
      REPD(geom, 2)
      {
         ShdBlur [geom][0]=get(S8+"ShdBlur" +geom+gl_es+4);
         ShdBlur [geom][1]=get(S8+"ShdBlur" +geom+gl_es+6);
         ShdBlur [geom][2]=get(S8+"ShdBlur" +geom+gl_es+8);
         ShdBlur [geom][3]=get(S8+"ShdBlur" +geom+gl_es+12);
         ShdBlurX[geom]   =get(S8+"ShdBlurX"+geom+gl_es+2);
         ShdBlurY[geom]   =get(S8+"ShdBlurY"+geom+gl_es+2);
      }
   }

   if(!D.deferredUnavailable())
   {
      // CLEAR
      ClearDeferred=get("ClearDeferred");

   #if !SLOW_SHADER_LOAD
      // SHADOW
      REPD(multi_sample, (D.shaderModel()>=SM_4_1) ? 2 : 1)
      {
         REPD(n, 6)
         REPD(c, 2)ShdDir[n][c][multi_sample]=getShdDir  (n+1, c, multi_sample);
                   ShdPoint    [multi_sample]=getShdPoint(        multi_sample);
                   ShdCone     [multi_sample]=getShdCone (        multi_sample);
      }

      // LIGHT
      REPD(multi_sample, (D.shaderModel()>=SM_4_1) ? 2 : 1)
      REPD(shadow      , 2)
      REPD(quality     , 2)
      {
                        DrawLightDir   [shadow][multi_sample][quality]       =getDrawLightDir   (shadow, multi_sample, quality);
                        DrawLightPoint [shadow][multi_sample][quality]       =getDrawLightPoint (shadow, multi_sample, quality);
                        DrawLightLinear[shadow][multi_sample][quality]       =getDrawLightLinear(shadow, multi_sample, quality);
         REPD(image, 2){DrawLightCone  [shadow][multi_sample][quality][image]=getDrawLightCone  (shadow, multi_sample, quality, image);
                     #if !DEPTH_CLIP_SUPPORTED
                        DrawLightConeFlat[shadow][multi_sample][quality][image]=getDrawLightConeFlat(shadow, multi_sample, quality, image);
                     #endif
                       }
      }

      // COL LIGHT
      REPD(multi_sample, (D.shaderModel()>=SM_4_1) ? 3 : 1)
      REPD(ao          , 2)
      REPD(  cel_shade , 2)
      REPD(night_shade , 2)
      REPD(glow        , 2)
         ApplyLight[multi_sample][ao][cel_shade][night_shade][glow]=getApplyLight(multi_sample, ao, cel_shade, night_shade, glow);
   #endif
   }

   // PARTICLE
   ParticleFrames=GetShaderParam("ParticleFrames");
   REPD(palette             , 2)
   REPD(soft                , 2)
   REPD(anim                , 3)
   REPD(motion_affects_alpha, 2)
      Particle[palette][soft][anim][motion_affects_alpha]=get(S8+"Particle"+palette+soft+anim+ 1 +motion_affects_alpha);
      Bilb                                               =get(S8+"Particle"+      0+   0+   0+ 0 +                   0);

   // FOG
   FogColor       =GetShaderParam(     "FogColor"  );
   FogDensity     =GetShaderParam(     "FogDensity");
   LocalFogColor  =GetShaderParam("LocalFogColor"  );
   LocalFogDensity=GetShaderParam("LocalFogDensity");
   LocalFogInside =GetShaderParam("LocalFogInside" );
                                 Fog[0]=get("Fog0");
   if(D.shaderModel()>=SM_4)
   {
                                 Fog[1]=get("Fog1");
      if(D.shaderModel()>=SM_4_1)Fog[2]=get("Fog2");
   }

   // OVERLAY
   OverlayParams=GetShaderParam("OverlayParams");
}
void MainShaderClass::connectRT()
{
   if(ShdMap[0])ShdMap[0]->set(Renderer._shd_map);
   if(ShdMap[1])ShdMap[1]->set(Renderer._cld_map);
}
/******************************************************************************/
// EFFECTS
/******************************************************************************/
void VolumetricCloudsFx::load()
{
   if(!shader)if(shader=ShaderFiles("Volumetric Clouds"))
   {
      Cloud     =GetShaderParam("Cloud");
      CloudMap  =GetShaderParam("CloudMap");
      Clouds    =shader->get("Clouds");
      CloudsMap =shader->get("CloudsMap");
      REPD(g, 2)CloudsDraw[g]=shader->get(S8+"CloudsDraw"+g);
   }
}
/******************************************************************************/
void VolumetricLights::load()
{
   if(!shader)if(shader=ShaderFiles("Volumetric Lights"))
   {
      REPD(n, 6)
      REPD(c, 2)VolDir[n][c]=shader->get(S8+"VolDir"+(n+1)+c);
                VolPoint    =shader->get(   "VolPoint" );
                VolLinear   =shader->get(   "VolLinear");
                VolCone     =shader->get(   "VolCone"  );
      Volumetric =shader->get("Volumetric0");
      VolumetricA=shader->get("Volumetric1");
   }
}
/******************************************************************************/
void HDR::load()
{
   if(!shader)if(shader=ShaderFiles("Hdr"))
   {
      HdrDS[0] =shader->get("HdrDS0"   );
      HdrDS[1] =shader->get("HdrDS1"   );
      HdrUpdate=shader->get("HdrUpdate");
      Hdr[0]   =shader->get("Hdr0"     );
      Hdr[1]   =shader->get("Hdr1"     );
   }   
}
/******************************************************************************/
void MotionBlur::load()
{
   if(!shader)if(shader=ShaderFiles("Motion Blur"))
   {
      MotionUVMulAdd     =GetShaderParam("MotionUVMulAdd");
      MotionVelScaleLimit=GetShaderParam("MotionVelScaleLimit");
      MotionPixelSize    =GetShaderParam("MotionPixelSize");

      Explosion=shader->get("Explosion");

      ClearSkyVel=shader->get("ClearSkyVel");

      REPD(h, 2)
      REPD(c, 2)Convert[h][c]=shader->get(S8+"Convert"+h+c);

      Dilate=shader->get("Dilate");

      REPD(c, 2)SetDirs[c]=shader->get(S8+"SetDirs"+c);

      REPD(d, 2)Blur[d]=shader->get(S8+"Blur"+d);

      pixels[0].pixels=1;
      pixels[1].pixels=2;
      pixels[2].pixels=4;
      pixels[3].pixels=6;
      pixels[4].pixels=8;
      pixels[5].pixels=12;
      pixels[6].pixels=16;
      pixels[7].pixels=24;
      pixels[8].pixels=32;
      ASSERT(ELMS(pixels)==9);
   }
}
C MotionBlur::Pixel* MotionBlur::pixel(Int pixel, Bool diagonal)
{
   if(pixel<=0)return null;
   Pixel *p;
   FREPA(pixels) // start from the smallest to find exact match or bigger, order is important
   {
      p=&pixels[i]; if(p->pixels>=pixel)break; // if this covers desired range of pixels to blur
   }
   if(!p->DilateX[diagonal])
   {
      p->DilateX[diagonal]=shader->get(S8+"DilateX"+diagonal+p->pixels);
      p->DilateY[diagonal]=shader->get(S8+"DilateY"+diagonal+p->pixels);
   }
   return p;
}
/******************************************************************************/
Shader* DepthOfField::getDS(Bool clamp , Bool realistic, Bool half_res) {return shader->get(S8+"DofDS"+clamp+realistic+half_res+D.gatherAvailable());}
Shader* DepthOfField::get  (Bool dither, Bool realistic               ) {return shader->get(S8+"Dof"+dither+realistic);}

void DepthOfField::load()
{
   if(!shader)if(shader=ShaderFiles("Depth of Field"))
   {
      DofParams=GetShaderParam("DofParams");

   #if !SLOW_SHADER_LOAD
      REPD(c, 2)
      REPD(r, 2)
      REPD(h, 2)DofDS[c][r][h]=getDS(c, r, h);

      REPD(d, 2)
      REPD(r, 2)Dof[d][r]=get(d, r);
   #endif

      pixels[ 0].pixels=2;
      pixels[ 1].pixels=3;
      pixels[ 2].pixels=4;
      pixels[ 3].pixels=5;
      pixels[ 4].pixels=6;
      pixels[ 5].pixels=7;
      pixels[ 6].pixels=8;
      pixels[ 7].pixels=9;
      pixels[ 8].pixels=10;
      pixels[ 9].pixels=11;
      pixels[10].pixels=12;
      ASSERT(ELMS(pixels)==11);
   }
}
C DepthOfField::Pixel& DepthOfField::pixel(Int pixel)
{
   Pixel *p;
   FREPA(pixels) // start from the smallest to find exact match or bigger, order is important
   {
      p=&pixels[i]; if(p->pixels>=pixel)break; // if this covers desired range of pixels to blur
   }
   if(!p->BlurX)
   {
      p->BlurX=shader->get(S8+"DofBlurX"+p->pixels);
      p->BlurY=shader->get(S8+"DofBlurY"+p->pixels);
   }
   return *p;
}
/******************************************************************************/
void WaterShader::load()
{
   if(!shader)if(shader=ShaderFiles("Water"))
   {
      REPD(fake_reflect, 2) // fake reflection
      {
         Lake [fake_reflect]=shader->get(S8+"Lake" +0+0+0+fake_reflect);
         River[fake_reflect]=shader->get(S8+"River"+0+0+0+fake_reflect);
         Ocean[fake_reflect]=shader->get(S8+"Ocean"+0+0+0+fake_reflect);

         REPD(shadow, 7)
         REPD(soft  , 2)
         {
            LakeL [fake_reflect][shadow][soft]=shader->get(S8+"Lake" +1+shadow+soft+fake_reflect);
            RiverL[fake_reflect][shadow][soft]=shader->get(S8+"River"+1+shadow+soft+fake_reflect);
            OceanL[fake_reflect][shadow][soft]=shader->get(S8+"Ocean"+1+shadow+soft+fake_reflect);
         }
      }
      REPD(r, 2)
      REPD(d, 2)Apply[r][d]=shader->get(S8+"Apply"+r+d);

      REPD(r, 2)Under[r]=shader->get(S8+"Under"+r);
   }
}
/******************************************************************************/
}
/******************************************************************************/
