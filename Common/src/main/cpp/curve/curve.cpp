/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:20:04 CET 2023                                                 */

#define USE_RUSSIAN 1 

#define MENUARROWS 1
#define PERCENTILES 1

//#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
//#include <filesystem>
#include <math.h>
#include <cstdint>
#include <cinttypes>
#include <charconv>

using namespace std::literals;
//#include "glucose.h"
//ScanData   *glucosenow=nullptr;



#include "curve.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

//#include "gles3jni.h"
#include "config.h"
//#define FILEDIR "/sdcard/libre2/"
//#include "Glucograph.h"
#include "logs.h"
//#define LOGGER(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#include "settings/settings.h"

#include "SensorGlucoseData.h"
#include "sensoren.h"
#include "nums/numdata.h"
#include "nfcdata.h"

#include "error_codes.h"
#include "jugglucotext.h"
int startincolors=0;
int lasttouchedcolor=-1;
static void printGlString(const char* name, GLenum s) {
#ifndef NDEBUG
    const char* v = (const char*)glGetString(s);
    if(v)
	    LOGGER("GL %s: %s\n", name, v);
#endif
}
/*
NVGcolor *allcolors[]
{ &pink,&brown,& blue,& green,& lightblue,&greenblue,&red,
&transred,
&nowcolor,
&dooryellow,
&white,
&black,
&gray,
&yellow};*/

//extern NVGcolor invertcolor(const NVGcolor *colin) ;

	
void cpcolors(NVGcolor *foreground) {
	int wholes=nrcolors/oldnrcolors;
	for(int i=1;i<wholes;i++) 
		memcpy(foreground+i*oldnrcolors,foreground,oldnrcolors*sizeof(foreground[0]));
	if(int left=nrcolors%oldnrcolors) {
		memcpy(foreground+wholes*oldnrcolors,foreground,left*sizeof(foreground[0]));
		}
	}


void createcolors() {
	NVGcolor *foreground=settings->data()->colors;
	NVGcolor *background=settings->data()->colors+startbackground;
	if(!settings->data()->colorscreated) {
		memcpy(foreground,allcolors,sizeof(allcolors));
	//	foreground[darkgrayoffset]=  nvgRGBAf(0,0,0,0.4);
		foreground[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3); 
		foreground[lightredoffset]=  nvgRGBAf2(1, 0.95, 0.95, 1); 
		foreground[grayoffset]=  nvgRGBAf2(0,0,0,0.1);

//		for(int i=0;i<std::size(allcolors);i++)  
		for(int i=0;i<oldnrcolors;i++)  {
			background[i]=invertcolor(foreground+i);
			}
		background[darkgrayoffset]=   nvgRGBAf2(.8,.8,.8,.8);
		background[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3);
//		background[lightredoffset]=   nvgRGBA(65, 65, 65, 255); 
		background[grayoffset]= {{{1.0f,1.0f,1.0f,.4f}}}; 
//		background[redoffset]= nvgRGBAf2(1.0,0,0,1.0);
		}
	if(settings->data()->colorscreated<3) {
		foreground[threehouroffset]=  nvgRGBAf2(1.0,0,1,0.5);
		background[threehouroffset]=  nvgRGBAf2(1.0,0,1,1);
		}
	if(settings->data()->colorscreated<5) {
		cpcolors(foreground);
		cpcolors(background);
		}
	if(settings->data()->colorscreated<15) {
		background[lightredoffset]=   blackbean; 
		settings->data()->colorscreated=15;
		}
	}


#ifdef MENUARROWS
// pyftsubset <font-file> --unicodes=  --output-file=<path>
#include "fonts.h"
#endif
NVGcontext* genVG=nullptr;
	int font=0,menufont=0,monofont=0,whitefont=-1,blackfont=0;
float headheight;
//Numdata *numdata=nullptr;
jint width=-1,height=-1;

float dleft=0,dtop=0,dbottom{0},dright=0,dheight,dwidth;
float smallsize=300,menusize=smallsize,headsize=900,midsize, mediumfont;
float density;
float textheight,menutextheight;
float smallfontlineheight;

	struct {
	float left,top;
	union {
		float right;
		float width;
	};
	union {
		float bottom;
		float height;
	};} sensorbounds;
float timelen=300;
union bounds_t{
	float array[4];
	struct {float xmin,ymin, xmax,ymax;};
	} ;
float listitemlen;

int numlist=0;
float smallerlen;

int duration=8*60*60;
extern bool fixatex,fixatey;
int showstream=1;
int showscans=1;
int showhistories=1;
int shownumbers=1;
int showmeals=0;
int invertcolors=0;
int showui=false;
float valuesize=0;

float facetimefontsize,facetimey;
void resetcurvestate();
static enum FontType {
	CHINESE,
	HEBREW,
	REST
	} chfontset=REST;
//static bool chfontset=false;

bool chinese();
bool hebrew() ;
static void initfont() { 
LOGAR("initfont");
if(!genVG) {
	LOGAR("genVG==null");
	return;
	}
if(chinese()) {
	font=whitefont=blackfont = nvgCreateFont(genVG, "dance-bold","/system/fonts/NotoSansCJK-Regular.ttc");

	menufont = nvgCreateFont(genVG, "regular", "/system/fonts/NotoSerifCJK-Regular.ttc");
//TODO free font ???
	chfontset=CHINESE;
	}

	else  {

#ifdef USE_HEBREW
if(hebrew())  {
	auto fallback = nvgCreateFont(genVG, "dance-bold","/system/fonts/DroidSans.ttf");



	font=whitefont=blackfont = nvgCreateFont(genVG, "dance-bold","/system/fonts/NotoSansHebrew-Regular.ttf");
	nvgAddFallbackFontId(genVG, font,fallback);


//	auto menufallback = nvgCreateFont(genVG, "regular","/system/fonts/NotoSerif.ttf");

	menufont=nvgCreateFontMem(genVG, "regular", (unsigned char *)fontfile, sizeof(fontfile), 0);
	int fallback2 = nvgCreateFont(genVG, "regular", "/system/fonts/NotoSerifHebrew-Regular.ttf");
	nvgAddFallbackFontId(genVG,menufont, fallback);
	nvgAddFallbackFontId(genVG, menufont,fallback2);



	chfontset=HEBREW;
}

else  
#endif
{
	chfontset=REST;

constexpr const char standardfonts[][41]= {
"/system/fonts/Roboto-Black.ttf",
"/system/fonts/SourceSansPro-Bold.ttf",
"/system/fonts/NotoSerif-Bold.ttf",
"/system/fonts/DroidSans-Bold.ttf",
"/system/fonts/SourceSansPro-SemiBold.ttf",
"/system/fonts/Roboto-Regular.ttf",
};



constexpr const char menufonts[][41]={
"/system/fonts/Roboto-Medium.ttf",
"/system/fonts/SourceSansPro-SemiBold.ttf",
"/system/fonts/NotoSerif.ttf",
"/system/fonts/SourceSansPro-Regular.ttf",
"/system/fonts/Roboto-Regular.ttf",
"/system/fonts/DroidSans.ttf"};

	for(const char *name:standardfonts)  {
		if((blackfont = nvgCreateFont(genVG, "dance-bold", name))!=-1)
			break;
		}
	if((whitefont= nvgCreateFont(genVG, "dance-bold", "/system/fonts/Roboto-Regular.ttf"))==-1)
		whitefont=blackfont;
	int fallback;
	for(const char *name:menufonts)  {
		if((fallback = nvgCreateFont(genVG, "regular", name))!=-1)
			break;
		}
#ifdef MENUARROWS
	menufont=nvgCreateFontMem(genVG, "regular", (unsigned char *)fontfile, sizeof(fontfile), 0);
	nvgAddFallbackFontId(genVG,menufont, fallback);
#endif
		/*

//int nvgCreateFontMem(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData);
	*/
	if(invertcolors)
		font=whitefont;
	else
		font=blackfont;
		}
		}

	nvgFontFaceId(genVG,font);
	nvgFontSize(genVG, headsize);
	constexpr const char smaller[]="<";
	bounds_t bounds;
	nvgTextBounds(genVG, 0,  0, smaller,smaller+sizeof(smaller)-1, bounds.array);
	smallerlen=bounds.xmax-bounds.xmin;

	nvgTextMetrics(genVG, nullptr,nullptr, &headheight);
	headheight*=0.7;
	nvgFontSize(genVG, smallsize);
	nvgTextMetrics(genVG, nullptr,nullptr, &smallfontlineheight);
	constexpr const char timestring[]="29:59";
	nvgTextBounds(genVG, 0,  0, timestring,timestring+sizeof(timestring)-1, bounds.array);
	timelen=bounds.xmax-bounds.xmin;

	const char listitem[]="39-08-2028 09-59 RRRRRRRRRRR 999.9";     
	nvgTextBounds(genVG, 0,  0, listitem,listitem+sizeof(listitem)-1, bounds.array);
	listitemlen=bounds.xmax-bounds.xmin+smallsize;

	constexpr const char exampl[]="0M0063KNUJ0";
	float xhalf=dwidth/2;
	float yhalf=dheight/2;
	nvgFontSize(genVG, mediumfont);
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	 nvgTextBounds(genVG, xhalf,  yhalf,exampl, exampl+sizeof(exampl)-1,(float *)&sensorbounds);
	 sensorbounds.right-=sensorbounds.left;
	 sensorbounds.bottom-=sensorbounds.top;
	 sensorbounds.left-=xhalf;
	 sensorbounds.top-=yhalf;
	 LOGGER("sensorbounds.left=%.1f\n",sensorbounds.left);
	valuesize=sensorbounds.right*2;
	 fixatex=settings->data()->fixatex;
	 fixatey=settings->data()->fixatey;
	 if(fixatex)
	 	duration=settings->data()->duration;
	createcolors();
	invertcolors=settings->data()->invertcolors;
         startincolors=startbackground*invertcolors;
	 }



void	initopengl(int started)  {
	if(!started) {
	    resetcurvestate();
	    }
	if(::genVG) { //Why should it be recreated?
#ifdef NANOVG_GLES2_IMPLEMENTATION
 nvgDeleteGLES2
#else 
 nvgDeleteGLES3
#endif
	(::genVG); 
	::genVG=nullptr;
	}


    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

   decltype(::genVG)	genVG = 

#ifdef NANOVG_GLES2_IMPLEMENTATION
	nvgCreateGLES2
#else
	nvgCreateGLES3
#endif


	(NVG_ANTIALIAS | NVG_STENCIL_STROKES
#ifndef NDEBUG
	| NVG_DEBUG
#endif

	);

	if (genVG == nullptr) {
		LOGSTRING("Could not init nanovg.");
		return ;
		}
	::genVG=genVG;
   	initfont();	
	 }


bool alarmongoing=false;


void resizescreen(int widthin, int heightin,int initscreenwidth) {
	width=widthin;
	height=heightin;
	LOGGER("resize(%d,%d)\n",width,height);
	dheight=height-dtop-dbottom,dwidth=width-dleft-dright; //Display area for graph in pixels

	textheight=density*48;
	int times=ceil(height/textheight);
	textheight=height/times;
	menutextheight=density*48;

	extern const int maxmenulen;
	const float maxmenu= (float)dheight/maxmenulen;
	if(menutextheight>maxmenu)
		menutextheight=maxmenu;
		
	LOGGER("menutextheight=%f\n", menutextheight);

	float facetimelen=2.0f*dwidth/3.0f;
	LOGGER("facetimelen=%.1f\n",facetimelen);
	facetimefontsize=smallsize*facetimelen/timelen;
	LOGGER("facetimefontsize=%.1f\n",facetimefontsize);
	float straal=dwidth*0.5f;
	facetimey=(straal-sqrt(pow(straal,2.0)-pow(facetimelen*.5,2.0)))*.70;

	LOGGER("facetimey=%.1f\n",facetimey);


}
//s/^\([^=]*\)=.*;/static float \1;/g
static float historyStrokeWidth;
static float numcircleStrokeWidth;
static float lowGlucoseStrokeWidth;
extern float pollCurveStrokeWidth;
float pollCurveStrokeWidth;
float hitStrokeWidth;
static float TrendStrokeWidth;
static float glucoseLinesStrokeWidth;
static float timeLinesStrokeWidth;
static float dayEndStrokeWidth;
static float nowLineStrokeWidth;
static float pointRadius;
float foundPointRadius,arrowstrokewidth;
void setfontsize(float small,float menu,float density,float headin) {
float head=headin
#ifdef WEAROS
*0.7
#endif
;
LOGGER("density=%.1f, head=%.1f, small=%.1f\n",(double)density,(double)head,(double)small); 
::smallsize=small;
::menusize=menu;
::density=density;
::headsize=head;
::midsize=head/3;
::mediumfont= headsize/6;
historyStrokeWidth=3*density;
numcircleStrokeWidth=5/2*density;
lowGlucoseStrokeWidth=2.5*density;
pollCurveStrokeWidth=3*density;
hitStrokeWidth=10*density;
TrendStrokeWidth=15/2*density;
glucoseLinesStrokeWidth=1.5*density;
timeLinesStrokeWidth=glucoseLinesStrokeWidth;
dayEndStrokeWidth=2*density;
nowLineStrokeWidth=density*2;
pointRadius=4*density;
foundPointRadius=8*density;

 arrowstrokewidth=5*density;
//dbottom=dtop=dright=dleft=foundPointRadius;
}
/*
#include <EGL/egl.h>
void sizechanged() {
EGLDisplay display= eglGetCurrentDisplay ();
decltype(auto) surface=eglGetCurrentSurface(EGL_DRAW);
    EGLint width;
    EGLint height;
    eglQuerySurface(display,surface,EGL_WIDTH,&width);
    eglQuerySurface(display,surface,EGL_HEIGHT,&height);
	LOGGER("EGL: width=%d, height=%d\n",width,height);
}

*/
// Adds line segment from the last point in the path to the specified point.

int daystr(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	return sprintf(buf,"%s %02d-%02d-%d",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
	}
inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	}
/*
inline int mktmmin(const struct tm *tmptr) {
	if(tmptr-> tm_sec<30)
		return tmptr->tm_min;
	return tmptr->tm_min+1;
	} */
bool hourmin(const time_t tim,char buf[6]) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	 snprintf(buf,6,"%02d:%02d",stm->tm_hour,mktmmin(stm));
	 if(stm->tm_hour||stm->tm_min)
	 	return false;
	return true;
	}
int largedaystr(const time_t tim,char *buf) {
        LOGAR("largedaystr");
	struct tm stmbuf;
	localtime_r(&tim,&stmbuf);
 	return sprintf(buf,"%02d:%02d %s %02d %s %d",stmbuf.tm_hour,mktmmin(&stmbuf),usedtext->daylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year);
	}
//		strftime(tbuf, maxbuf,"%H:%M %a %e %b %Y", &tmbuf);

//static constexpr const int glucosetype=0xffffffff;//std::numeric_limits<int>::max();
  static constexpr const int glucosetype=0x40000000;//std::numeric_limits<int>::max();
  static constexpr const int nosearchtype=0x20000000;//std::numeric_limits<int>::max();
	static constexpr const int historysearchtype=2|glucosetype;
	static constexpr const int scansearchtype=1|glucosetype;
	static constexpr const int streamsearchtype=4|glucosetype;
struct  {
int type;float under;float above;int frommin; int tomin;
uint32_t count;
float amount;
std::vector<int> ingredients;
bool operator()  (const Num *num) const {
	if(num->type>count)
		return false;
	if((type<0||num->type==type)) {
		float val=num->value;
		if(val>=under&&val<=above&&righttime(num->time)) {
			extern int carbotype;
			if(type==carbotype&&ingredients.size()>0) {
				return (num->mealptr>0)&& meals->datameal()->matchmeals(ingredients,amount,num->mealptr);
				}
			return  true;
			}
		}
	return false;
	}



bool operator()  (const Num &num) const {
	return operator()(&num);
	}

bool righttime(time_t tim)const {
	if(frommin<0&&tomin<0)
		return true;
	struct tm stm;
	localtime_r(&tim,&stm);
	int minutes=60*stm.tm_hour+ mktmmin(&stm);
	if(frommin<=tomin||tomin<0) {	
		if(minutes>=frommin&&(tomin<0||tomin>=minutes) ){
			return  true;
			}

		return false;
		}
	if(minutes<tomin|minutes>frommin)
		return true;
	return false;
	}
bool operator() (const ScanData *g) const {
//	if((type&scansearchtype)!=scansearchtype) return false;
	if(!g)
		return false;
	uint32_t glu=g->g*10.0;
	if(g->t&&glu&&glu>=under&&glu<=above&&righttime(g->t)) {
		return  true;
		}
	return false;
	}
bool operator() (const Glucose *g) const {
//	if((type&historysearchtype)!=historysearchtype) return false;
	if(!g||!g->valid())
		return false;
	uint32_t glu=g->getsputnik();

	if(glu>=under&&glu<=above&&righttime(g->gettime())) {
		return  true;
		}
	return false;
	}
} searchdata={.type=nosearchtype};
template <typename T>
bool searchhit(const T *ptr) {
	return searchdata(ptr);
	}
//template bool searchhit(const Num *ptr);

template   bool searchhit<Num>(const Num *ptr); 
/*
findminenmax:
getminmax(
	uint32_t  gmin=6000, gmax=0;
	for(auto pos=firstpos;pos<=lastpos;pos++) {
		auto glu=hist->sputnikglucose(pos);
		gmin=std::min(glu,gmin);
		gmax=std::max(glu,gmax);
		}
daylabel:
	char buf[128];
	auto endstr=daystr(hist.timeatpos((1+lastpos+firstpos)/2),buf);
	nvgFillColor(genVG, datecolor);
	nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
	nvgFontSize(genVG, headsize);
	nvgText(genVG, dwidth/2+dleft,dtop+dheight/2, buf, buf+endstr);

		*/



pair<const ScanData*,const ScanData*> getScanRange(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {

	ScanData scanst{.t=start};
	const ScanData *endscan= scan+len;
	auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
  	const ScanData *low=lower_bound(scan,endscan, scanst,comp);
	if(low==endscan) {
		return {endscan,endscan};
		}
	scanst.t=end;
  	const ScanData *high=lower_bound(low,endscan, scanst,comp);

	return {low,high};
	}

extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) ;
std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) {
	auto hists= sensors->inperiod(start,endt) ;
	vector<pair<const ScanData*,const ScanData*>> polldata;
	polldata.reserve(hists.size());
	uint32_t timeiter=start;
	LOGSTRING("getsensorranges: \n");
	for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
		auto his=sensors->getSensorData(hists[i]);
		std::span<const ScanData> 	poll=his->getPolldata();
#ifndef NDEBUG
		auto wastimeiter=timeiter;
#endif
		auto ran=getScanRange(poll.data(),poll.size(),timeiter,endt);
		if(ran.first==ran.second)
			continue;

		for(const ScanData *striter=ran.second-1;striter>=ran.first;striter--) {
			if(striter->valid())	{
				timeiter=striter->t;
				ran.second=striter+1;
				break;
				}
			}
#ifndef NDEBUG
		constexpr const int  maxbuf=150;
		char buf[maxbuf];
		int len=datestr(wastimeiter,buf);

		const char tus1[]=" : ";
		constexpr const int tus1len=sizeof(tus1)-1;
		memcpy(buf+len,tus1,tus1len);

		len+=tus1len;
		len+=datestr(poll.data()->t,buf+len);

		memcpy(buf+len,tus1,tus1len);
		len+=tus1len;
		len+=datestr(ran.first->t,buf+len);
		const char tus[]=" - ";
		constexpr const int tuslen=sizeof(tus)-1;
		memcpy(buf+len,tus,tuslen);
		len+=tuslen;
		len+=datestr((ran.second-1)->t,buf+len);
		buf[len++]='\n';
		logwriter(buf,len);
#endif
		polldata.push_back(ran);
		}
	return polldata;
	}
//static uint32_t pollgapdist=5*60;
static uint32_t pollgapdist=330;
pair<const ScanData*,const ScanData*> getScanRangeRuim(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {
	auto [low,high]= getScanRange(scan,len,start,end);
	const ScanData *endscan= scan+len;
	/*
	ScanData scanst{.t=start};
	const ScanData *endscan= scan+len;
	auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
  	const ScanData *low=lower_bound(scan,endscan, scanst,comp);
	if(low==endscan) {
		return {endscan,endscan};
		}
	scanst.t=end;
  	const ScanData *high=upper_bound(low,endscan, scanst,comp);*/

	if(low>scan&&(low->t-(low-1)->t)<pollgapdist)
		low--;
	if(high<endscan&&((high+1)->t-high->t)<pollgapdist)
		high++;
	return {low,high};
	}

static void		sidenum(const float posx,const float posy,const char *buf,const int len,const bool hit) {
		int align= NVG_ALIGN_MIDDLE;
		float valx=posx;
		const float afw=hit?1.14:0.64;;
		 if((posx-dleft)>(dwidth/2)) {
			align|=NVG_ALIGN_RIGHT;
			valx-=smallsize*afw;
			}
		else {
			align|=NVG_ALIGN_LEFT;
			valx+=smallsize*afw;
			}
		nvgTextAlign(genVG,align);
		nvgText(genVG, valx,posy, buf, buf+len);
		}

static uint32_t getmaxlabel() { return settings->getlabelcount(); }


static int *numheights=nullptr;
int shownlabels;

//y=A+x*D
//x=(y-A)/D;
jfloat tapx=-700,tapy;
bool selshown=false;
//#include "numdisplay.h"
//vector<NumDisplay*> numdatas;

#include "numdisplayfuncs.h"
extern vector<NumDisplay*> numdatas;
int typeatheight(const float h) {
//	const float gr= density*24;
	const float gr= density*24;
	const int maxl= settings->getlabelcount();
	for(int i=0;i<maxl;i++) {
		if(numheights[i]>=0) {
			float th= numtypeheight(i);
			if(fabsf(h-th)<gr)
				return i;
			}
		}
	return -1;	
	}


float getfreey() {
	const int nrlabs=getmaxlabel();
	static const int mid=nrlabs/2-1;
	static const float midh=numtypeheight(mid);
	static  float boven=numtypeheight(mid+1);
	return (boven+midh)/2.0f;
	}


static bool glucosepointinfo(time_t tim,uint32_t value,   float posx, float posy) {
	if((!selshown&&nearby(posx-tapx,posy-tapy))) {
		constexpr int maxbuf=60;
		char buf[maxbuf];
		struct tm tmbuf;
		 struct tm *tms=localtime_r(&tim,&tmbuf);

		int len=snprintf(buf,maxbuf,"%02d:%02d", tms->tm_hour,mktmmin(tms));
		nvgFontSize(genVG, smallsize);
		nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
		float cor=((posy-dtop)<(dheight/2))?smallsize:-smallsize;
		nvgText(genVG, posx,posy+cor*.92, buf, buf+len);
		char *buf2=buf+len;
		*buf2++='\n';
	 	len=snprintf(buf2,maxbuf-len-1,gformat, gconvert(value));
		sidenum(posx,posy,buf2,len,false);
		
	//	nvgText(genVG, posx,posy+cor*.92*2, buf, buf+len);
#ifndef WEAROS
		if(speakout) {
			speak(buf);
			}
#endif

		selshown=true;
		return true;
		}
	return false;
	}
static bool glucosepoint(time_t tim,uint32_t value,   float posx, float posy) {
	nvgCircle(genVG, posx,posy,pointRadius);
	return glucosepointinfo(tim,value,posx,posy);
	}

static void endstep() ;
static bool emptytap=false;
template <class TX,class TY> bool showScan(NVGcontext* genVG,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex) {

	nvgFillColor(genVG,*getcolor(colorindex));
	nvgBeginPath(genVG);
	bool search=scansearchtype==(scansearchtype&searchdata.type);
	for(const ScanData *it=low;it!=high;it++) {
		if(it->valid()) {
			const uint32_t tim= it->t;
			const auto glu=it->g*10;
			const auto posx= transx(tim),posy=transy(glu);
			if(search&&searchdata(it)) 
				nvgCircle(genVG, posx,posy,foundPointRadius);
			else {
				if(glucosepoint(tim,glu,posx,posy))
					lasttouchedcolor=colorindex;
				}
			}
		}
	nvgFill(genVG);
	return true;
	}
	//			nvgCircle(genVG, posx,posy,;
	

static void makecircle(float posx,float posy) {
	nvgBeginPath(genVG);
	nvgCircle(genVG, posx,posy,pointRadius);
	nvgFill(genVG);

	}

template <class TX,class TY> void showlineScan(NVGcontext* genVG,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex
#ifdef SI5MIN
,bool isSibionics
#endif
) {
	bool search=streamsearchtype==(streamsearchtype&searchdata.type);
//   uint32_t dif=isSibionics?450:pollgapdist;
#ifdef SI5MIN
   uint32_t dif=isSibionics?8*60:pollgapdist;
#else
   uint32_t dif=pollgapdist;
#endif

	if(search) {
		nvgBeginPath(genVG);
//		nvgStrokeColor(genVG, yellow); nvgFillColor(genVG, yellow);
		nvgStrokeColor(genVG, *getyellow()); nvgFillColor(genVG, *getyellow());
		nvgStrokeWidth(genVG, hitStrokeWidth);
		bool restart=true,first;
		uint32_t late=0;
		bool washit=false;
		float prevx=-1.0f,prevy;
		for(const ScanData *it=low;it!=high;it++) {
			if(it->valid()&&searchdata(it)) {
					const uint32_t tim= it->t;
					const auto glu=it->g*10;
					const auto posx= transx(tim),posy=transy(glu);
					if(washit) {
						if(!restart&&tim>late) {
							nvgStroke(genVG);
							if(first)
								makecircle(prevx,prevy);
							restart=true;
							}
						}
					else {
						washit=true;	
						restart=true;
						}
					if(restart) {
						nvgBeginPath(genVG);
						 nvgMoveTo(genVG, posx,posy);
						 restart=false;
						 first=true;
						 }
					else {
						first=false;
						nvgLineTo( genVG,posx,posy);
						}
					late=tim+dif;
					prevx=posx;
					prevy=posy;
					}
			else {
				if(washit&&!restart) {
					nvgStroke(genVG);
					if(first)
						makecircle(prevx,prevy);
					}
				else
					washit=false;
				restart=true;
				}
			}
		if(washit) {	
			if(!restart)
				nvgStroke(genVG);
			if(first)
				makecircle(prevx,prevy);
			}
		}
	bool restart=true;
	nvgBeginPath(genVG);
	const NVGcolor *col=getcolor(colorindex);
	nvgStrokeColor(genVG, *col);
	nvgFillColor(genVG,*col);
	nvgStrokeWidth(genVG, pollCurveStrokeWidth);
	uint32_t late=0;
	float startx=-1000,starty=-1000;
	for(const ScanData *it=low;it!=high;it++) {
		if(it->valid()) {
			const uint32_t tim= it->t;
			const auto glu=it->g*10;
			const auto posx= transx(tim),posy=transy(glu);

			if(!restart&&tim>late) {
				nvgStroke(genVG);
				if(startx>=0) {
					nvgBeginPath(genVG);
					nvgCircle(genVG, startx,starty,pollCurveStrokeWidth);
					nvgFill(genVG);
					 }
				restart=true;
				}
			if(restart) {
				nvgBeginPath(genVG);
				 nvgMoveTo(genVG, posx,posy);
				 startx=posx,starty=posy;
				 restart=false;
				 }
			else {
				 startx=starty=-1000.0f;
				nvgLineTo( genVG,posx,posy);
				}

			late=tim+dif;

			if(glucosepointinfo(tim,glu, posx, posy) ) {
				nvgLineTo( genVG,posx,posy);
				nvgStroke(genVG);
				nvgBeginPath(genVG);
				nvgCircle(genVG, posx,posy,pointRadius*1.3);
				nvgFill(genVG);
				nvgBeginPath(genVG);
				nvgMoveTo(genVG, posx,posy);
				lasttouchedcolor=colorindex;
				}
			}
		else {
			/*
			if(!restart) {
				nvgStroke(genVG);
				restart=true;
				} */
			}
		}

		nvgStroke(genVG);
		if(startx>=0) {
			nvgBeginPath(genVG);
			nvgCircle(genVG, startx,starty,pollCurveStrokeWidth);
			nvgFill(genVG);
			 }
		}

pair<int32_t,int32_t> histPositions(const SensorGlucoseData  * hist, const uint32_t starttime, const uint32_t endtime) {
	int32_t firstmog=hist->getstarthistory();
	int32_t lastmog= hist->getAllendhistory()-1;
	LOGGER("histPositions first=%u last=%u\n",firstmog,lastmog);
	if(firstmog>=lastmog)
		return {firstmog,lastmog};
	uint32_t begin=hist->getstarttime();
	int sdisp=starttime-begin;
	int period=hist->getinterval();
	int off=sdisp/period;	
	int32_t	firstpos=firstmog+(uint32_t)((off>0)?off:0);
	if(firstpos>lastmog)
		firstpos=lastmog;
	for(;firstpos>firstmog;--firstpos) {
		auto tim=hist->timeatpos(firstpos);
		if(tim&&tim<=starttime)
			break;
		}
	for(;firstpos<lastmog&&!hist->timeatpos(firstpos);++firstpos) {
		}
	uint32_t firsttime=hist->timeatpos(firstpos);

	int lastscreen=firstpos+(endtime-firsttime)/period;
	int32_t lastpos=(lastscreen>lastmog)?lastmog:lastscreen;
	while(lastpos<lastmog&&hist->timeatpos(lastpos)<endtime)
		lastpos++;

	return {firstpos,lastpos};
	}

template <class TX,class TY> void histcurve(NVGcontext* genVG,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex) {
	const NVGcolor *col=getcolor(colorindex);
	nvgStrokeColor(genVG, *col);
	nvgFillColor(genVG,*col);
	 bool restart=true;
	 float startx=-3000.0f,starty=-3000.0f;
	for(auto pos=firstpos;pos<=lastpos;pos++) {
		const Glucose *histglu=hist->getglucose(pos);
		if(histglu->valid()) {
			const uint32_t tim=histglu->gettime(),glu=histglu->getsputnik();
			auto posx=xtrans(tim),posy=ytrans( glu);
			bool oncurve=glucosepointinfo(tim,glu, posx, posy);
			if(restart) {
				if(oncurve) {
					nvgBeginPath(genVG);
					nvgCircle(genVG, posx,posy,pointRadius*1.3);
					nvgFill(genVG);
					lasttouchedcolor=colorindex;
					}
				nvgBeginPath(genVG);
				 nvgMoveTo(genVG, posx,posy);
				 startx=posx,starty=posy;
				 restart=false;
				 }
			else {
				nvgLineTo( genVG, posx,posy);
				 startx=-3000.0f,starty=-3000.0f;
				if(oncurve) {
					nvgStroke(genVG);
					nvgBeginPath(genVG);
					nvgCircle(genVG, posx,posy,pointRadius*1.3);
					nvgFill(genVG);
					nvgBeginPath(genVG);
					nvgMoveTo(genVG, posx,posy);
					lasttouchedcolor=colorindex;
					}
				}

			}
		else {
			if(!restart) {
				nvgStroke(genVG);
				if(startx>=0.0f) {
					nvgBeginPath(genVG);
					nvgCircle(genVG, startx,starty,historyStrokeWidth);
					nvgFill(genVG);
					}
				restart=true;
				}
			}
		}
	if(!restart) {
		nvgStroke(genVG);
		if(startx>=0.0f) {
			nvgBeginPath(genVG);
			nvgCircle(genVG, startx,starty,historyStrokeWidth);
			nvgFill(genVG);
			}
		}
	if((searchdata.type&historysearchtype)==historysearchtype) {
		nvgBeginPath(genVG);
		for(auto pos=firstpos;pos<=lastpos;pos++) {
			const Glucose *glu=hist->getglucose(pos);
			if(searchdata(glu)) {
				const auto tim=glu->gettime();
				if(tim) {
					const auto sput=glu->getsputnik();
					auto xc=xtrans(tim);
					auto yc= ytrans(sput);
					nvgCircle(genVG,xc,yc,foundPointRadius);
					}
				}
			}
		nvgFill(genVG);
		}
	}


extern uint32_t getnumlasttime();
uint32_t maxstarttime() ;
uint32_t maxtime() {
	const uint32_t numt=getnumlasttime();
	const uint32_t sent= sensors->timelastdata(); 
	#ifdef NOLOG
	time_t tim=sent;
	LOGGER("sensors->timelastdata()=%u %s",sent,ctime(&tim));
	#endif
	return max(numt,sent);
	}
	
static uint32_t getnumfirsttime() {
	uint32_t first=UINT32_MAX;

	for(auto el:numdatas)  {
		auto mog=el->getfirsttime();
		if(mog<first)
			first=mog;	
		}
	return first;
	}
	
uint32_t mintime() {
	uint32_t sent= sensors?sensors->timefirstdata():UINT32_MAX;
	uint32_t numt=getnumfirsttime();
	return min(numt,sent);
	}
int gmin=2*180;
int grange=8*180;
uint32_t starttime;
int diffcurrent=0;
extern void setstarttime(uint32_t);
bool doclamp=false;
void setdiffcurrent() {
	//diffcurrent=(uint64_t)time(nullptr)-starttime;
	auto now=time(nullptr);
	diffcurrent=now-starttime;
	if(diffcurrent>(duration*5/6)) {
		doclamp=false;
		}
	doclamp=true;
	LOGGER("now=%u starttime=%u diffcurrent=%d\n",now,starttime,diffcurrent);
	}
extern bool nowclamp;
void setstarttime(uint32_t newstart) {
	starttime=newstart;
	if(nowclamp) {
		setdiffcurrent();
		}
	}
uint32_t maxstarttime() {
//	return maxtime()-duration/2;
	float duraf=((float)valuesize/dwidth);
	LOGGER("dwidth=%f valuesize=%f duraf=%f\n",(double)dwidth,(double)valuesize,(double)duraf);
//	return time(nullptr)-((duraf<=0.65)?(duration*0.45):0);
//	float subtr=0.55f - (55.0f*(duraf - 0.33f))/60.0;
//	float subtr=0.8525 - 0.916667*duraf;
	float subtr=0.91 - duraf*1.2f;
	return time(nullptr)-subtr*duration;
//	return time(nullptr)-((duraf<=0.65)?(duration*0.55):0);
	}
uint32_t minstarttime() {
	uint32_t mini=mintime();
	if(mini<duration)
		return mini;

	return mini-duration/2;
	}
/*

	return mini-duration;
	*/
void begrenstijd() {
	auto maxstart= maxstarttime();
	if(starttime>maxstart)
		setstarttime(maxstart);
	else {
		auto minstart= minstarttime();
		if(starttime<minstart)
			setstarttime(minstart);
		}
	}

#include <memory>
uint32_t settime=0;
uint32_t setend=0;

pair<float,float> drawtrender(NVGcontext* genVG,const std::array<uint16_t,16> &trend,const float x,const float y,const float w,const float h) {
	auto minel=std::min_element(trend.begin(),trend.end());
	auto maxel=std::max_element(trend.begin(),trend.end());
	 const int low=minel-trend.begin();
	 const int high=maxel-trend.begin();
	 if(low<0||high<0)
	 	return {0,dtop+dheight/2};
	const float lowval=*minel;
	const float highval=*maxel;
	const float mid=(lowval+highval)/2.0;
	LOGGER("width=%.0f, height=%.0f\n",w,h);
	LOGGER("low=%.0f,high=%.0f,mid=%.0f\n",lowval,highval,mid);
	constexpr float hglurange=2*convfactor;
	const auto gety=[y,h,mid](const short val)->float  { return y+h/2.0-(((val-mid)/hglurange)*h);};
	const int step=w/(trend::num-1);
	nvgBeginPath(genVG);
	 nvgStrokeWidth(genVG, TrendStrokeWidth);
//	nvgStrokeColor(genVG, white);
	nvgStrokeColor(genVG, *getblack());
	int i=0;
	unsigned short glu0;
	for(;!(glu0=trend[i]);i++)
		if(i>=(trend.size()-3))
			return {0,dtop+dheight/2};
	float pos0=gety(glu0);
	float posx= x+i*step;
	 nvgMoveTo(genVG,posx ,pos0);
	LOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu0/convfactor,glu0,posx,pos0);
	posx+=step;
	float posy;
	i++;
	for(;i<trend.size();i++,posx+=step) {
		short glu=trend[i];
		if(glu) {
			posy=gety(glu);
			LOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu/convfactor,glu,posx,posy);
			nvgLineTo( genVG,posx ,posy);
			}
		}
	LOGSTRING("\n");
	nvgStroke(genVG);
	return std::pair<float,float>({pos0,posy});
	}

static void startstep(const NVGcolor &col);
struct {
float left,top,right,bottom;
} menupos;
void showok(bool good,bool up) {
	nvgFontSize(genVG,headsize/4 );
	nvgTextAlign(genVG,NVG_ALIGN_RIGHT|(up?NVG_ALIGN_TOP:NVG_ALIGN_BOTTOM));
	const float fromtop= mediumfont*2.0f;
	float ypos=dtop+(up?fromtop:(dheight-fromtop));
	float xpos=dwidth+dleft-mediumfont*3.0f;

	const char *ok=good?"OK":"ESC";
	const int oklen=good?2:3;
	nvgTextBounds(genVG, xpos,ypos ,ok , ok+oklen, (float *)&menupos);
	nvgText(genVG, xpos,ypos,ok,ok+oklen);
	menupos.left-=mediumfont;
	menupos.right+=mediumfont;
	menupos.bottom+=mediumfont;
	menupos.top-=mediumfont;
	}
static bool		showerror(NVGcontext* genVG,const string_view str1,const string_view str2) {
	startstep(*getyellow());
	nvgFontSize(genVG, midsize);
	nvgFillColor(genVG, *getblack());
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_BOTTOM);
	nvgText(genVG, dleft+dwidth/10,dtop+dheight/3, str1.begin(), str1.end());
	nvgFontSize(genVG, midsize*.8);
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
	nvgTextBox( genVG, dleft+dwidth/10, dtop+dheight/2, dwidth*8/10, str2.begin(), str2.end());

	if(settings->data()->speakmessages) {
		char buf[str1.size()+str2.size()+2+10];
		memcpy(buf,str1.data(),str1.size());
		char *ptr=buf+str1.size();
		*ptr++='\n';
		memcpy(ptr,str2.data(),str2.size());
		ptr[str2.size()]='\0';
		LOGGER("speak %s\n",buf);
		speak(buf);

		}

	showok(true,false);
	return true;
	}
	/*
static bool		showerror(NVGcontext* genVG,const string_view str1,const string_view str2) {
	}*/
static void		scanwait(NVGcontext* genVG) {
	startstep(*getwhite());
	nvgFontSize(genVG, headsize);
	nvgFillColor(genVG, *getblack());
	nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
//	std::string_view str1="Scanned";
	const std::string_view str1=usedtext->scanned;
	nvgText(genVG, dleft+dwidth/2,dtop+dheight/2, str1.begin(), str1.end());
	endstep();
	}
#include "gluconfig.h"
int mkshowlow(char *buf, const int maxbuf) {
		return snprintf(buf,maxbuf,"%.*f>",gludecimal,gconvert(glucoselowest*10));
	}
int mkshowhigh(char *buf, const int maxbuf) {
	return snprintf(buf,maxbuf,"%.*f<",gludecimal,gconvert(glucosehighest*10));
	}
int getglucosestr(uint32_t nonconvert,char *glucosestr,int maxglucosestr) {
	if(nonconvert<glucoselowest) {
		return mkshowlow(glucosestr, maxglucosestr) ;
		}
	else {
		if(nonconvert>glucosehighest) {
			return mkshowhigh(glucosestr, maxglucosestr) ;
			}
		else {
			const float convglucose= gconvert(nonconvert*10);
			return snprintf(glucosestr,maxglucosestr,gformat,convglucose);
			}
		}
	}

static void	showscanner(NVGcontext* genVG,const SensorGlucoseData *hist,int scanident,time_t nu) {
	const ScanData &last=*hist->getscan(scanident);
	const bool isold=(nu-last.t)>=maxbluetoothage;
	startstep(isold?getoldcolor():*getwhite());
	float x= dwidth+dleft;
	constexpr int maxbuf=50;
	char buf[maxbuf*2];
	time_t tim=last.t;
	struct tm tmbuf;
	 struct tm *tms=localtime_r(&tim,&tmbuf);
	int len=snprintf(buf,maxbuf,"%02d:%02d ", tms->tm_hour,mktmmin(tms));
	char *buf1=buf+len;
	--len;
	const int32_t gluval=last.g;
	int len1;
	float endtime=x,sensleft=0.0f;

	if(gluval<glucoselowest) {
		len1=mkshowlow(buf1,maxbuf);
		endtime-=smallerlen;
		sensleft=smallerlen;
		}
	else {
		if(gluval>glucosehighest) {
			len1=mkshowhigh(buf1,maxbuf);
			endtime-=smallerlen;
			}
		else {
			len1=snprintf(buf1,maxbuf,gformat, gconvert(gluval*10.0f));
			}
		}

	float bounds[4];
	nvgTextAlign(genVG,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
	nvgFontSize(genVG, headsize);
	nvgTextBounds(genVG, x,dtop+dheight/2 , buf1, buf1+len1, bounds);
	nvgFillColor(genVG, *getblack());
	auto [first,y]=drawtrender( genVG,hist->gettrendsbuf(scanident),dleft,dtop,bounds[0]-dleft,dheight);
	float th=(bounds[3]-bounds[1])/2.0;
	if(y<th) 
		y=th;
	else
		if((dheight-(y-dtop))<th)
			y=dheight-th;

	nvgText(genVG, x,y, buf1, buf1+len1);
	const bool showabove=y>(dheight/2);
	const float yunder=y+(showabove?-1:1)*headsize/2.0;
	nvgFontSize(genVG,mediumfont );
	nvgText(genVG, endtime,yunder, buf, buf+len);
	const sensorname_t *sensorname=hist->othershortsensorname();
	nvgFontSize(genVG,headsize*.134f );
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    const auto sensorx=bounds[0] -sensleft;
	nvgText(genVG,sensorx,yunder, sensorname->begin(), sensorname->end());
	const bool showdate=(nu-last.t)>=60*60*12;
	constexpr const int maxdatebuf=30;
	int datelen;
	char datebuf[maxdatebuf];
	if(isold) {
		if(showdate) {
			float datey=yunder+(showabove?-1:1)*sensorbounds.height;
			nvgTextAlign(genVG,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
			datelen=snprintf(datebuf,maxdatebuf,"%s %d %s %04d",usedtext->speakdaylabel[tmbuf.tm_wday],tmbuf.tm_mday,usedtext->monthlabel[tmbuf.tm_mon],1900+tmbuf.tm_year);
			nvgText(genVG,dleft+dwidth,datey, datebuf, datebuf+datelen);
			}
		nvgStrokeWidth(genVG, TrendStrokeWidth);
		nvgStrokeColor(genVG, *getwhite());
		nvgBeginPath(genVG);
	 	nvgMoveTo(genVG,dleft ,dtop) ;
		nvgLineTo( genVG, dwidth,dheight);
		nvgStroke(genVG);
		nvgBeginPath(genVG);
	 	nvgMoveTo(genVG,dwidth ,dtop) ;
		nvgLineTo( genVG, dleft,dheight);
		nvgStroke(genVG);
		}
#ifndef WEAROS

	if(settings->data()->speakmessages) {
		char value[300];
		char *ptr=value;;
		if(isold) {
			if(showdate) {
				memcpy(ptr,datebuf,datelen);
				ptr+=datelen;
				*ptr++='\n';
				*ptr++='\n';
				}
			memcpy(ptr,buf,len);
			ptr+=len;
			*ptr++='\n';
			}
		auto trend=usedtext->trends[last.tr];
		memcpy(ptr,trend.data(),trend.size());
		ptr+=trend.size();
		*ptr++='\n';
		*ptr++='\n';
		memcpy(ptr,buf1,len1+1);
		speak(value);
		}
#endif

	showok( (last.g>70&&last.g<=140), ((y-dtop)<(dheight/2))?false:true);
	}

void setextremes(pair<int,int> extr) {
	auto [gminin,gmaxin]=extr;
	setend=0;
	const uint32_t gmaxmax=settings->graphhigh();
	const uint32_t gminmin=settings->graphlow();
	if(gmaxin<gmaxmax)
		gmaxin=gmaxmax;
	if(gminin>gminmin)
		gminin=gminmin;
	grange=gmaxin-gminin;
	gmin=gminin;
	}

static pair<int,int> getextremes(const vector<int> &hists, const pair<const ScanData *,const ScanData*> **scanranges, int scannr,const pair<int32_t,int32_t> *histpositions) {
	int gmax=0;
	int gmin=6000;
	const int histlen=hists.size();
	for(int i=0;i<histlen;i++) {
		if(1||showhistories) {
			for(auto pos=histpositions[i].first,last=histpositions[i].second;pos<=last;pos++) {
				int glu=sensors->getSensorData(hists[i])->sputnikglucose(pos);
				if(glu) {
					if(glu>gmax)
					     gmax=glu;
					if(glu<gmin)
					     gmin=glu;
					}
				}
			}
		for(int j=0;j<scannr;j++) {
			const pair<const ScanData *,const ScanData*> *srange=scanranges[j];
			for(const ScanData *it=srange[i].first,*last=srange[i].second;it<last;it++) {
				if(it->valid()) {
					int glu=it->g*10;
					if(glu>gmax)
						gmax=glu;
					 if(glu<gmin)
						 gmin=glu;
					}
				}
			}
		}
	return {gmin,gmax};
	}
template <class LT> void glucoselines(const float last,const float smallfontlineheight,const int gmax,const LT &transy) {
	nvgStrokeWidth(genVG, glucoseLinesStrokeWidth);
	nvgStrokeColor(genVG, *getgray());
	const double yscale=transy(1)-transy(0);
	const float mindisunit=smallsize*1.5;
	const float minst=abs(mindisunit/yscale);
	bool ismmolL=settings->usemmolL();
	const double unit=ismmolL?0.5*convfactor:100;
	const double unit2=unit*2;

	uint32_t step=minst<=unit?unit:ceilf(minst/unit2)*unit2;
	float startld;
	nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);

	if(settings->data()->levelleft) {
		startld = timelen*.4;
		}
	else  {
		startld =  dwidth/2+dleft;
		}


	uint32_t keer=floorf(ceil(gmin)/step);
	uint32_t startl=keer*step;
//	const float endline=(dleft+dwidth)>nupos?nupos:(dwidth+dleft);
	const float endline=last;
	LOGGER("glucoselines: unit=%f unit2=%f step=%d (%g) startl=%d (%g)\n",unit,unit2,step,gconvert(step),startl,gconvert(startl));
	for(auto y=startl+step;y<gmax;y+=step) {
		float dy=transy(y);
		nvgBeginPath(genVG);
	 	nvgMoveTo(genVG,dleft ,dy) ;
		nvgLineTo( genVG, endline,dy);
		nvgStroke(genVG);
		if(dy>smallfontlineheight) {
			constexpr const int  bufsize=50;
			char buf[bufsize];
#ifdef CONV18
			int len=snprintf(buf,bufsize,"%g",gconvert(y));
#else
			int len=snprintf(buf,bufsize,gformat,gconvert(y));
			if(ismmolL)  {
				if(buf[len-1]=='0') 
					len-=2;
				}
#endif
			if(len>bufsize)
				len=bufsize;
			nvgText(genVG, startld,dy, buf, buf+len);
			}

		}
	}
struct displaytime {
	const uint32_t tstep;
	const uint32_t first;
	const uint32_t last;
	};
template <class LT>
const displaytime getdisplaytime(const uint32_t nu,const uint32_t starttime,const uint32_t endtime, const LT &transx) {
	const float xscale=transx(1)-transx(0);
	const float mindisunit=smallsize*3;
	const  float minst=abs(mindisunit/xscale);
	const uint32_t tstep=(minst<=60*15)?60*15:((minst<=60*30)?60*30:ceilf((minst/(60.0*60)))*(60*60));
	const uint32_t first=uint32_t(ceilf(starttime/(double)tstep))*tstep;	
	const uint32_t endhier=(nu<endtime)?(nu+tstep-59):(endtime-1);
	const uint32_t last=uint32_t(floorf(endhier/double(tstep)))*tstep;	
	LOGGER("getdisplaytime xscale=%.1f %u %u %u\n",xscale,tstep,first,last);
	return {tstep,first,last};
}
template <class LT>
void timelines(const displaytime *disp, const LT &transx ,uint32_t nu) {

	const uint32_t tstep=disp->tstep;
	const uint32_t first=disp->first;
	const uint32_t last= disp->last;
	#ifdef WEAROS
	const uint32_t numlast= (disp->last>nu)?(disp->last-tstep):disp->last;
	#endif
	nvgFillColor(genVG, *getblack());
	nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
	const float timehight=
	#ifdef WEAROS
		smallfontlineheight*1.6
	#else
		0
	#endif
	;
	for(auto tim=first;tim<=last;tim+=tstep) {
		float dtim=transx(tim);
		char buf[6];
		struct tm tmbuf;
		time_t tmptime=tim;
		 struct tm *stm=localtime_r(&tmptime,&tmbuf);

	 	if(stm->tm_hour||stm->tm_min) {
			if(stm->tm_min||stm->tm_hour%3) {
				nvgStrokeWidth(genVG, timeLinesStrokeWidth);
				nvgStrokeColor(genVG, *getgray());
				}
			else {
				nvgStrokeWidth(genVG, timeLinesStrokeWidth);
				nvgStrokeColor(genVG, *getthreehour());
				}
			}
		else {
			nvgStrokeWidth(genVG, dayEndStrokeWidth);
			nvgStrokeColor(genVG, *getblack());
			}
	#ifdef WEAROS
		 if(tim<=numlast)  
	#endif
		 {
		 	snprintf(buf,6,"%02d:%02d",stm->tm_hour,mktmmin(stm));
			nvgText(genVG, dtim,timehight, buf, buf+5);
			}
		nvgBeginPath(genVG);
		nvgMoveTo(genVG,dtim ,0) ;
		nvgLineTo( genVG, dtim,dheight);
		nvgStroke(genVG);
		}
	}

template <class LT> void epochlines(uint32_t first,uint32_t last, const LT &transx) {
		time_t startin=first;

		struct tm tmbuf;
		 struct tm *stm=localtime_r(&startin,&tmbuf);
		auto hour=stm->tm_hour;
		if(stm->tm_min) {
			startin+=(60-stm->tm_min)*60;
			hour++;
			}
		
		time_t start=startin+(24-hour)*60*60;
		nvgStrokeWidth(genVG, dayEndStrokeWidth);
		nvgStrokeColor(genVG, *getblack());
		for(time_t t=start;t<last;t+=(24*60*60)) {
			float dtim=transx(t);
		//	LOGGER("%ld\n",t);
			nvgBeginPath(genVG);
			nvgMoveTo(genVG,dtim ,0) ;
			nvgLineTo( genVG, dtim,dheight);
			nvgStroke(genVG);
			}
		nvgStrokeWidth(genVG, timeLinesStrokeWidth);
		nvgStrokeColor(genVG, *getthreehour());
		const int inthree=hour%3;
		start=startin+(inthree?((3-inthree)*60*60):0);
		LOGGER("startin=%ld start=%ld last=%d inthree=%d\n",startin,start,last, inthree);
		for(time_t t=start;t<last;t+=(3*60*60)) {
			float dtim=transx(t);
			nvgBeginPath(genVG);
			nvgMoveTo(genVG,dtim ,0) ;
			nvgLineTo( genVG, dtim,dheight);
			nvgStroke(genVG);
			}
	}
extern std::vector<int> usedsensors;
extern void setusedsensors() ;
extern void setusedsensors(uint32_t nu) ;
//extern std::span<streamdat> laststream;
void setmaxsensors(size_t sensornr) {
	setusedsensors();
	}


//float highglucose=1350;
//float lowglucose=702;
uint32_t lastsensorends() {
		int lastsen=sensors->last();
		if(lastsen>=0) {
			const sensor *sen=sensors->getsensor(lastsen);
//			time_t enddate= (14*24)*60*60+sen->starttime;
			return (14*24)*60*60+sen->starttime;
			
			//return sen->maxtime();
			}
		return 0;
		}
//void	showbluevalue(const float dlast,const time_t nu,const int xpos) 

void drawarrow(NVGcontext* genVG, float rate,float getx,float gety) {
		if(!isnan(rate)) {
			if(glnearnull(rate))
				rate=.0f;
			if(rate<=0.0f)
				gety-=headheight/12.5f;
			float x1=getx-density*40;
			float y1=gety+rate*density*30;

			long double rx=getx-x1;
			long double ry=gety-y1;
			double rlen= sqrt(pow(rx,2) + pow(ry,2));
			 rx/=rlen;
			 ry/=rlen;

			long double l=density*12;

			double addx= l* rx;
			double addy= l* ry;
			double tx1=getx-2*addx;
			double ty1=gety-2*addy;
			double xtus=getx-1.5*addx;
			double ytus=gety-1.5*addy;
			double hx=ry;
			double hy=-rx;
			double sx1=tx1+l*hx;
			double sy1=ty1+l*hy;
			double sx2=tx1-l*hx;
			double sy2=ty1-l*hy;
			nvgBeginPath(genVG);
			nvgStrokeColor(genVG, *getblack());
			nvgStrokeWidth(genVG, arrowstrokewidth);
			nvgMoveTo(genVG,x1,y1) ;
			nvgLineTo( genVG, xtus,ytus);
			nvgStroke(genVG);
			nvgBeginPath(genVG);
			nvgFillColor(genVG, *getblack());
			nvgMoveTo(genVG,sx1,sy1) ;
			nvgLineTo( genVG, getx,gety);
			nvgLineTo( genVG, sx2,sy2);
			nvgLineTo( genVG, xtus,ytus);
			nvgClosePath(genVG);
			nvgFill(genVG);

			}
	}
struct shownglucose_t {
const char *errortext=nullptr;
int glucosetrend;
float glucosevalue=0;
float glucosevaluex=-1,glucosevaluey=-1;
} ;
std::vector<shownglucose_t> shownglucose;
#ifndef NOLOG
//#define TESTVALUE
#endif
static void showvalue(const ScanData *poll,const sensorname_t *sensorname, float getx,float gety,int index,uint32_t nu) {
	LOGGER("showvalue %s\n",sensorname->data());
	float sensory= gety+headsize/3.1;
	nvgFillColor(genVG, *getblack());
	nvgFontSize(genVG,mediumfont );
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	nvgText(genVG, getx,sensory, sensorname->begin(), sensorname->end());
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
	constexpr const int maxhead=11;
	char head[maxhead];
#ifdef TESTVALUE
   static int values[]={230,184};
	const auto nonconvert= values[index];
#elif 1
	const auto nonconvert= poll->g;
#else
	const uint32_t nonconvert= 40;
#endif
	nvgFontSize(genVG, headsize*.8);
	shownglucose[index].glucosevaluex=getx;
shownglucose[index].glucosevaluey=sensory;
	if(nonconvert<glucoselowest) {
const				float valuex=getx;

		 int gllen=mkshowlow(head, maxhead) ;
		nvgText(genVG,valuex,gety, head, head+gllen);
		}
	else {
		if(nonconvert>glucosehighest) {
		float valuex=getx-density*14.0f;
		 int gllen=mkshowhigh(head, maxhead) ;
			nvgText(genVG,valuex ,gety, head, head+gllen);
			}
		else {
#if 0
			const float convglucose= 27.8f;
#else
			const float convglucose= gconvert(nonconvert*10);
#endif

		shownglucose[index].glucosevalue=convglucose;
		shownglucose[index].glucosetrend=poll->tr;

			float valuex=getx-(convglucose>=10.0f?density*20.0f:0.0f);
			int gllen=snprintf(head,maxhead,gformat,convglucose);
			nvgText(genVG,valuex ,gety, head, head+gllen);
#ifdef TESTVALUE
const float trends[]={-3,0};
			const float rate=trends[index];
#else
			const float rate=poll->ch;
#endif
			drawarrow(genVG,rate,valuex-10*density,gety);
			}
		}

	}

	
//static bool	streamvalueshown=false;
bool bluetoothEnabled();
float				getboxwidth(const float x) {
					return std::max((float)(dwidth-x-smallsize),dwidth*.25f);
					}

//#define DOTEST 1
static int showerrorvalue(const SensorGlucoseData *sens,const time_t nu,float getx,float gety,int index) {
 
	shownglucose[index].glucosevalue=0;
	getx-=headsize/3;
	shownglucose[index].glucosevaluex=getx;
	shownglucose[index].glucosevaluey=gety+headsize*.5;
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
	nvgFontSize(genVG,headsize/6 );
	if(settings->data()->nobluetooth) {
		LOGAR("nobluetooth");
		extern bool hasnetwork();
		if(hasnetwork()) {
			return 1;
			}
		else {
			return 2;
			}
		}
	else {
		LOGAR("!nobluetooth");
		{
			if(!bluetoothEnabled()) {
				return 3;
				} 
			else {
           if((nu-sens->receivehistory)<60) {
					static char buf[256];
					auto past=usedtext->receivingpastvalues;
					memcpy(buf,past.data(),past.size());
					char *ptr=buf+past.size();
					ptr+=sprintf(ptr,": %d",sens->pollcount());
					nvgTextBox(genVG,  getx, gety, getboxwidth(getx),buf, ptr);
					shownglucose[index].errortext=buf;
            }
            else {
				if(sens->sensorerror) {
					const std::string_view sensorerror= sens->replacesensor?usedtext->streplacesensor: usedtext->stsensorerror;
					char buf[sensorerror.size()+17];
					int senslen= sens->showsensorname().size();
					memcpy(buf,sens->showsensorname().data(),senslen);
					memcpy(buf+senslen,sensorerror.data(), sensorerror.size());
					auto boxwidth= getboxwidth(getx);
					nvgTextBox(genVG,  getx, gety, boxwidth, buf, buf+sensorerror.size()+senslen);
					shownglucose[index].errortext=sensorerror.data();
					}
				else {
				   int state=sens->getinfo()->patchState;
				if(state&&state!=4){
					auto format= state>4?usedtext->endedformat:usedtext->notreadyformat;
					static char buf[256];
					int len=snprintf(buf,sizeof(buf)-1,format.data(),sens->showsensorname().data(),state);
					nvgTextBox(genVG,  getx, gety, getboxwidth(getx),buf, buf+len);
					shownglucose[index].errortext=buf;
					} 
				else {
					char buf[usedtext->noconnectionerror.size()+17];
					int senslen= sens->showsensorname().size();
					memcpy(buf,sens->showsensorname().data(),senslen);
					memcpy(buf+senslen,usedtext->noconnectionerror.data(), usedtext->noconnectionerror.size());
					nvgTextBox(genVG,  getx, gety, getboxwidth(getx),buf, buf+usedtext->noconnectionerror.size()+senslen);
					shownglucose[index].errortext=usedtext->noconnectionerror.data();
					}
					}
               }
				return 0;
				}
			}
	
		}
	}
#ifdef NOTALLVIES
int betweenviews=60*30;
time_t nexttimeviewed=0;
#endif
static void showlastsstream(const time_t nu,const float getx,std::vector<int> &used ) {
//LOGGER("showlaststream %d\n",used.size());
	int success=false;
	bool neterror=false,usebluetoothoff=false,bluetoothoff=false,otherproblem=false;
	static int failures=0;
	++failures;
	const auto usedsize=used.size();
	shownglucose.resize(usedsize);

	for(int i=0;i<usedsize;i++) {
		shownglucose[i].glucosevaluex=-1;
		const int sensorindex=used[i];
		SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
		int yh=i*2+1;
#ifdef WEAROS
		float gety=smallsize*.5f+dtop+dheight*yh/(usedsize*2.0f);
#else
		float gety=smallsize*1.4f+dtop+(dheight-smallsize*.8f)*yh/(usedsize*2.0f);
#endif
		const ScanData *poll=hist->lastpoll();
		if(poll) {
			LOGSTRING("poll!=null\n");
			int age=nu-poll->t;
			if(age<maxbluetoothage) {
				LOGSTRING("age<maxbluetoothage\n");
				if(!poll->valid())
					return;
				failures=0;
				nvgBeginPath(genVG);
				 nvgFillColor(genVG,getoldcolor());
				float relage=(float)age/(float)maxbluetoothage;
				float sensory= gety+headsize/3.1f;
				nvgRect(genVG, getx+sensorbounds.left, sensorbounds.top+sensory, relage*sensorbounds.width, sensorbounds.height);
				nvgFill(genVG);
				showvalue(poll,hist->othershortsensorname(),getx,gety,i,nu);
				success=true;
				if(hist->isLibre2()) {
					 if(settings->data()->libreIsViewed&&!hist->getinfo()->libreviewsendall) {
#ifdef NOTALLVIES
						if(poll->t>nexttimeviewed) 
#endif
						{

							const int addnum= hist->pollcount()-1;
							if(hist->viewed.empty()||hist->viewed.back()!=addnum) {
								hist->viewed.push_back(addnum);

#ifdef NOTALLVIES
								nexttimeviewed=poll->t+betweenviews;
								LOGGER("add %d nextime=%s",addnum,ctime(&nexttimeviewed));
#endif
								}
							}
						}
					}
				}
			else {
				LOGAR("age>=maxbluetoothage");
				switch(showerrorvalue(hist,nu,getx,gety,i)) {
					case 1: neterror=true;break;
					case 2: usebluetoothoff=true;break;
					case 3: bluetoothoff=true;break;
					default: otherproblem=true;
					};
				LOGAR("AFgter showerrorvalue(hist,nu,getx,gety)) ");
				}
			}
		else {
			LOGSTRING("poll==null\n");
			time_t starttime=hist->getstarttime();
			auto wait= nu-starttime;
			LOGGER("wait=%lu starttime=%lu %s",wait,starttime,ctime(&starttime));
			if(wait<(60*60)) {
				float usegetx=getx-headsize/3;
				nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
				nvgFontSize(genVG,headsize/6 );
				getboxwidth(usegetx);
				const char *bufptr;
				int ends;

				if(hist->isSibionics()) {
					static constexpr const std::string_view siwait{"Waiting for connection"sv};
					bufptr=siwait.data();
					ends=siwait.size();
					}
				else {
					const bool isInitialised=(!hist->isLibre2())||sensors->getsensor(sensorindex)->initialized;
					LOGGER("wait<(60*60) isInitialised=%d\n",isInitialised);
					static char buf[256];
					int minutes=60-(wait/60);
					ends=sprintf(buf,isInitialised?usedtext->readysec.data():usedtext->readysecEnable.data(),minutes);
					bufptr=buf;
					}
				nvgTextBox(genVG,  usegetx, gety, getboxwidth(usegetx), bufptr,bufptr+ends);
				shownglucose[i].errortext=bufptr;
				shownglucose[i].glucosevalue=0;
				shownglucose[i].glucosevaluex=usegetx;
				shownglucose[i].glucosevaluey=gety+headsize*.5;
				}
			else   {
				LOGAR("age>=maxbluetoothage");
				switch(showerrorvalue(hist,nu,getx,gety,i)) { //TODO: integrate with same above
					case 1: neterror=true;break;
					case 2: usebluetoothoff=true;break;
					case 3: bluetoothoff=true;break;
					default: otherproblem=true;
					};
				LOGAR("Afgter showerrorvalue(hist,nu,getx,gety)) ");
				}
			}

		}
	if(!success&&!otherproblem) {
		int i=0;
		shownglucose.resize(1);
		LOGAR("!success&&!otherproblem) ");
		int newgetx=getx-headsize/3;
		nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
		nvgFontSize(genVG,headsize/4 );
		float gety=smallsize*.5f+dtop+dheight/2.0f;
		if(neterror) {
//			nvgText(genVG,newgetx ,gety, usedtext->networkproblem.begin(), usedtext->networkproblem.end());
			nvgTextBox(genVG,  newgetx, gety, getboxwidth(newgetx), usedtext->networkproblem.begin(), usedtext->networkproblem.end());
				shownglucose[i].glucosevalue=0;
			       shownglucose[i].glucosevaluex=newgetx;
       				shownglucose[i].glucosevaluey=gety+headsize*.5;
				shownglucose[i].errortext=usedtext->networkproblem.data();


			}
		else { if(usebluetoothoff) {
		   nvgTextBox(genVG,newgetx ,gety, getboxwidth(newgetx),usedtext->useBluetoothOff.begin(), usedtext->useBluetoothOff.end());
				shownglucose[i].glucosevalue=0;
			       shownglucose[i].glucosevaluex=newgetx;
       				shownglucose[i].glucosevaluey=gety+headsize*.5;
			shownglucose[i].errortext=usedtext->useBluetoothOff.data();
		   }
		   else {
		   	if(bluetoothoff) {
				nvgTextBox(genVG,newgetx ,gety, getboxwidth(newgetx),usedtext->enablebluetooth.begin(), usedtext->enablebluetooth.end());
				shownglucose[i].glucosevalue=0;
			       shownglucose[i].glucosevaluex=newgetx;
       				shownglucose[i].glucosevaluey=gety+headsize*.5;
				shownglucose[i].errortext=usedtext->enablebluetooth.data();
				}
				}
				}
		}
	if(failures>2) {
		LOGAR("failures>3" );
		for(int i=0;i<used.size();i++) {
			if(SensorGlucoseData *hist=sensors->getSensorData(used[i])) {
				LOGSTRING("set waiting=true\n");
                		hist->waiting=true;
				}
			}
		}


	LOGAR(" end showlastsstream");
	}



void	showbluevalue(const time_t nu,const int xpos,std::vector<int> &used) {
LOGGER("showbluevalue %zd\n",used.size());
		nvgFontSize(genVG, smallsize);
		nvgFillColor(genVG, *getblack());

		nvgBeginPath(genVG);
		nvgStrokeColor(genVG, dooryellow);
		nvgStrokeWidth(genVG, nowLineStrokeWidth);
		nvgMoveTo(genVG,xpos ,dtop) ;
		nvgLineTo( genVG, xpos,dheight+dtop);
		nvgStroke(genVG);
		#ifndef WEAROS
		{
		float down=0;

		const float timex=xpos+nowLineStrokeWidth;
		nvgTranslate(genVG, timex,down);
		nvgRotate(genVG,-NVG_PI/2.0);
//		constexpr int maxhead=54;
		constexpr int maxhead=80;
		char head[maxhead];
		memcpy(head,usedtext->sensorends.data(),usedtext->sensorends.size());


		if(const auto *sens=sensors->getSensorData()) {
			if(time_t enddate=sens->officialendtime()) {
				const int tstart=usedtext->sensorends.size();
				char *endstr=head+tstart;
				int end= datestr(enddate,endstr); 
				nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
				nvgText(genVG, -dheight/2+down-smallfontlineheight,dwidth-timex, std::begin(head), head+end+tstart);
				}
			}
		nvgResetTransform(genVG);
		}
		#endif
		const float getx= xpos+headsize*.9f+8*dwidth/headsize;

		const float datehigh=smallfontlineheight*.72;
		
		nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
		{
		constexpr int maxbuf=120;
		char tbuf[maxbuf];
         const int datlen=largedaystr(nu,tbuf) ;
		const float timex =
			getx
		#ifdef WEAROS
			-timelen
		#endif
		;
/*
#ifndef WEAROS	
		double getiob(uint32_t);
		if(const auto iob=getiob(nu)) {
			datlen+=snprintf(tbuf+datlen,maxbuf-datlen," IOB: %.2f",iob);
			}
#endif */
		nvgText(genVG, timex,datehigh, tbuf, tbuf+datlen);

#ifndef WEAROS	
	if( settings->data()->IOB) {
		double getiob(uint32_t);
		int len=snprintf(tbuf,maxbuf,"IOB: %.2f",getiob(nu));
		nvgText(genVG, timex,2*smallfontlineheight, tbuf,tbuf+len);
		}
#endif

		LOGGER("xpos=%d dwidth=%.1f headsize=%.1f density=%.1f getx=%.1f timex=%.1f\n",xpos,dwidth,headsize, density,getx,timex);
		}
	showlastsstream(nu, getx,used) ;
	}

void	showsavedomain(const float last, const float dlow,const float dhigh) {
	nvgBeginPath(genVG);
	nvgFillColor(genVG, unsavecolor);
	nvgRect(genVG, dleft, dtop, last-dleft, dhigh);
	nvgFill(genVG);

	nvgBeginPath(genVG);
	nvgFillColor(genVG, unsavecolor);
	nvgRect(genVG, dleft, dlow, last-dleft, dheight+dtop);
	nvgFill(genVG);
	}
void showunsaveredline(const float last,const float dlow) {
	nvgBeginPath(genVG);
	nvgStrokeWidth(genVG, lowGlucoseStrokeWidth);

	nvgStrokeColor(genVG, lowlinecolor);
	nvgMoveTo(genVG, dleft,dlow) ;
	nvgLineTo( genVG,last ,dlow);
	nvgStroke(genVG);
	}
void	showsaverange(const float last, const float dlow,const float dhigh) {
	showsavedomain(last,dlow,dhigh) ;
	showunsaveredline(last,dlow) ;
	}
		
void 	showdates(time_t nu,uint32_t starttime,time_t endtime) {
   LOGGER("duration=%d\n",duration);
	int32_t timdis=nu-starttime;
constexpr const int grens=
#ifdef WEAROS
1
#else
3
#endif
;
LOGGER("timdis=%d duration=%d grens=%d\n",timdis,duration,grens);
if(timdis>0&&((duration/timdis)<grens)) {
	   LOGGER("timdis=%d larger than zero\n",timdis);
		const float datehigh=smallfontlineheight*
#ifdef WEAROS
		.71;
#else
		1.5;
		#endif

		char tbuf[70];
uint32_t showtime=
	#ifdef WEAROS
	(endtime+starttime)/2
	#else
	starttime
	#endif
	;

		daystr(showtime,tbuf);
		nvgFillColor(genVG, *
		#ifdef WEAROS
		getdarkgray()
		#else
		getblack()
		#endif
		);
	float xpos;
	#ifdef WEAROS
		xpos= dwidth/2+dleft;
		nvgTextAlign(genVG,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
	#else
		xpos= settings->data()->levelleft?timelen*.75:0;
		nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	#endif

		LOGGER("displaytime %s\n",tbuf);
		nvgText(genVG,xpos ,datehigh, tbuf, NULL);
#ifndef WEAROS
		if(nu>=endtime) {
			daystr(endtime,tbuf);
			nvgTextAlign(genVG,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
			nvgText(genVG, dwidth+dleft,datehigh, tbuf, NULL);
			}
#endif
		}
	}
auto gettrans(uint32_t starttime,uint32_t endtime) {
	double interval=endtime-starttime;
	const double usedleft=0.0;
	const double usedwidth=dwidth-2*usedleft;
	const double usedtop=pointRadius;
	const double usedheight=dheight-2*usedtop;
	const int gmax=gmin+grange;
	const double xscale=usedwidth/interval,xmove=usedleft-((double)starttime)*usedwidth/interval;
	const double yscale= -usedheight/grange,ymove= usedtop+usedheight*gmax/grange;

	const auto transx=[xscale,xmove](uint32_t x) {return x*xscale + xmove;};
       	const auto transy=[yscale,ymove](uint32_t y) {return y*yscale + ymove;};
//	return pair<decltype(transx),decltype
	return make_pair(transx,transy);
	}
void showlines(int gm,int gmax) {
		uint32_t endtime=starttime+duration;
		gmin=gm;
		grange=gmax-gmin;
		const auto [transx,transy]= gettrans(starttime, endtime);
	       displaytime disp=getdisplaytime(UINT_MAX,starttime,endtime, transx);
		const float dlast=dleft+dwidth;
		timelines(&disp,  transx,UINT32_MAX);
		if(disp.tstep>(60*60))
			epochlines(starttime,disp.last,transx);
		glucoselines(dlast,smallfontlineheight,gmax,transy) ;
		showunsaveredline(dlast,transy(settings->targetlow()));
		int yhigh=transy(settings->targethigh());
		nvgBeginPath(genVG);
		nvgStrokeWidth(genVG, lowGlucoseStrokeWidth);
		nvgStrokeColor(genVG, dooryellow);
		nvgMoveTo(genVG, dleft,yhigh) ;
		nvgLineTo( genVG,dwidth,yhigh);
		nvgStroke(genVG);
		}
		

pair<const ScanData *,const ScanData*> *scanranges=nullptr;
pair<const ScanData *,const ScanData*> *pollranges=nullptr;
pair<int32_t,int32_t> *histpositions=nullptr;
int histlen=0;
vector<int> hists;

int displaycurve(NVGcontext* genVG,time_t nu) {
	::starttime=(doclamp)?(nu-diffcurrent):(::starttime);
	const uint32_t starttime=::starttime;
	const uint32_t endtime=starttime+duration;


	mealpos.clear();
	LOGSTRING("display\n");
	hists= sensors->inperiod(starttime,endtime) ;
	histlen=hists.size();
	delete[] scanranges;
	scanranges=new pair<const ScanData *,const ScanData*> [histlen];
	delete[] pollranges;
	pollranges=new pair<const ScanData *,const ScanData*> [histlen];
	delete[] histpositions;
	histpositions=new std::remove_reference_t<decltype(histpositions[0])>[histlen];
#ifdef SI5MIN
   bool sibionics[histlen];
#endif
	LOGSTRING("before getranges\n");
	for(int i=histlen-1;i>=0;--i) {
		auto his=sensors->getSensorData(hists[i]);
		if(!his)  {
			LOGSTRING("getSensorData==null\n");
			sleep(1);
			return 0;
			}
        	LOGGER("%s\n",his->othershortsensorname()->data());
		std::span<const ScanData> 	scan;
		//if(showscans) 
		{
			scan=his->getScandata();
			scanranges[i] =getScanRange(scan.data(),scan.size(),starttime,endtime) ;
			}
		//if(showstream) 
		{
			scan=his->getPolldata();
			pollranges[i] =getScanRangeRuim(scan.data(),scan.size(),starttime,endtime) ;
#ifdef SI5MIN
         sibionics[i]=his->isSibionics();
#endif
			}
//		if(showhistories)
			histpositions[i]= histPositions(his, starttime,  endtime); 
		 }
	LOGGER("Before numdatas[i]->getInRange(%u,%u)\n",starttime,endtime);
	for(int i=0;i< numdatas.size();i++) 
		numdatas[i]->extrum=numdatas[i]->getInRange(starttime, endtime) ;
	const pair<const ScanData *,const ScanData*> *scanpoll[]= {scanranges,pollranges};
	LOGSTRING("Before getextremes\n");
	if((setend<starttime||settime>=endtime)) {
	   auto extr=getextremes(hists,scanpoll,2,histpositions);
	   for(int i=0;i<numdatas.size();i++)  {
	   	       LOGGER("%d before extremenums \n",i);
			extr  = numdatas[i]->extremenums(extr);
			}
	   setextremes(extr) ;
	   }
	LOGSTRING("before gettrans\n");
	int  gmax = gmin+grange;
	const auto [transx,transy]= gettrans(starttime, endtime);
displaytime disp=getdisplaytime(nu,starttime,endtime, transx);
	const float dlast=nu<endtime?transx(disp.last):dleft+dwidth;
	LOGSTRING("before showsaverange\n");
	showsaverange(dlast,transy(settings->targetlow()),transy(settings->targethigh()));

	nvgFontSize(genVG, smallsize);
	LOGSTRING("before showNums\n");
	const int catnr=settings->getlabelcount();

	showdates(nu,starttime,endtime) ;

	int nupos=transx(nu); 
	timelines(&disp,  transx,nu);
	if(disp.tstep>(60*60))
		epochlines(starttime,endtime<nu?endtime:disp.last,transx);
	glucoselines(dlast,smallfontlineheight,gmax,transy) ;

//		nvgCircle(genVG, posx,posy,foundPointRadius);

	LOGSTRING("before showhistories\n");
	const int colorsleft=nrcolors-catnr;
	const auto segcolor=[catnr,colorsleft,colorseg=colorsleft/3](int index,int seg) {
		 return catnr+(index+colorseg*seg)%colorsleft;
		 };
	if(showhistories) {
		nvgStrokeWidth(genVG, historyStrokeWidth);
		for(int i=histlen-1;i>=0;i--) {
			int index= hists[i];
			int colorindex=segcolor(index,2);
			 histcurve(genVG,sensors->getSensorData(index), histpositions[i].first, histpositions[i].second,transx,transy,colorindex); 
			 }
		}
	LOGSTRING("before showstream\n");
	if(showstream)   {
		nvgStrokeWidth(genVG, pollCurveStrokeWidth);
		for(int i=histlen-1;i>=0;i--) {
			const int index= hists[i];
//			int  colorindex=(index+nrcolors/4)%nrcolors;
			int colorindex=segcolor(index,0);
			showlineScan(genVG,pollranges[i].first,pollranges[i].second,transx,transy,colorindex
#ifdef SI5MIN
         ,sibionics[i]
#endif
         );
			 }
		}
	LOGSTRING("before showscans\n");
	if(showscans) {
		for(int i=histlen-1;i>=0;i--) {
			const int index=hists[i];
//			const int colorindex=(index+nrcolors*2/4)%nrcolors;
			int colorindex=segcolor(index,1);
			 if(!showScan(genVG,scanranges[i].first,scanranges[i].second,transx,transy,colorindex))
				return 1;
			 }
		 }
	if(shownumbers||showmeals)  {
		bool was[catnr];
		memset(was,0,sizeof(was));
		for(auto el:numdatas) 
			el->showNums(genVG, transx,  transy,was) ;
		}

	if(nu<endtime&&(dwidth-smallfontlineheight)>nupos) {
			showbluevalue(nu, nupos,usedsensors);
			LOGAR("end display curve");
		}
	else
		shownglucose.resize(0);

 return 0;
}

static void startstep(const NVGcolor &col) {
		glViewport(0, 0, width, height);
		glClearColor(col.r,col.g, col.b, col.a);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		nvgBeginFrame(genVG, width, height, 1.0);
		if(invertcolors)
			font=whitefont;
		else
			font=blackfont;
		nvgFontFaceId(genVG,font);
		nvgLineCap(genVG, NVG_ROUND);
 		nvgLineJoin(genVG, NVG_ROUND);
		}
static void endstep() {
    nvgEndFrame(genVG);
    glEnable(GL_DEPTH_TEST);
}

bool restart=false;
static bool showoldscan(NVGcontext* genVG,uint32_t ) ;

static void defaulterror(NVGcontext* genVG,int scerror)   {
		char buf[50];
		errortype *error=usedtext->scanerrors;
		size_t len=snprintf(buf,50,error->first.data(),scerror);
		showerror(genVG,error->second,{buf,len});
		}
static bool errorpair(const errortype &error) {
	return showerror(genVG,error.first,error.second);
	}
int badscanMessage(int kind) {
	const uint32_t nu=time(nullptr);
	if(!showoldscan(genVG,nu)) {
		LOGGER("javabadscan	%d: \n",kind);
		const int scerror= kind&0xff;
		switch(scerror) {
			case 0xFA: {
//				showerror(genVG,"FreeStyle Libre 3, Scan error", "Try again");
				errorpair(usedtext->libre3scanerror);
				};
				break;
			case 0xFB:
				errorpair(usedtext->libre3wrongID);
				break;
//				showerror(genVG,"Error, wrong account ID?","Specify in Settings->Libreview the same account used to activate the sensor");break;
		 	case 0xFC: {
				errorpair(usedtext->libre3scansuccess);
//				showerror(genVG,"FreeStyle Libre 3 sensor", "Glucose values will now be received by Juggluco");
				};break;
			case 0xFD: {
				errorpair(usedtext->unknownNFC);
//				showerror(genVG,"Unrecognized NFC scan Error", "Try again");
				};break;
			case 0xFE: {
				errorpair(usedtext->nolibre3);
//				showerror(genVG,"FreeStyle Libre 3 sensor","Not supported by this version of Juggluco"  );
				};break;
			case 0xff: {
				scanwait(genVG); 	
				return 2;
				};
			case 5: {
				errortype *error=usedtext->scanerrors+scerror;
				const int bufsize=error->second.size()+5;
				char buf[bufsize];
				size_t len=snprintf(buf,bufsize,error->second.data(),kind>>8);
				showerror(genVG,error->first,{buf,len});
				LOGGER("%s\n",buf);
				};break;
			case 0:
			case 15:
			case 9: defaulterror(genVG,scerror);
				break;
			case 12: restart=true;
			default: 
			 if(scerror>0x10) {
				defaulterror(genVG,scerror);
				}
			else {
				errorpair(usedtext->scanerrors[scerror]);
//				errortype *error=usedtext->scanerrors+scerror; showerror(genVG,error->first,error->second);
				}
			}
		}
	endstep();	
	return 1;
	}
static int nrmenu=0,selmenu=0;

static void showtext(NVGcontext* genVG ,time_t nu,int tapx) ;


struct lastscan_t scantoshow={-1,nullptr}; 

static bool showoldscan(NVGcontext* genVG,uint32_t nu) {
	if(scantoshow.scan) {
		if((nu-scantoshow.showtime)<60) {
			numlist=0;
		      const SensorGlucoseData *hist=sensors->getSensorData(scantoshow.sensorindex);
		      showscanner(genVG,hist,scantoshow.scan-hist->beginscans(),nu) ;
		      return true;
		      }
		  else {
			scantoshow={-1,nullptr}; 
		  	}
		}
	return false;
	}
int getmenu(int tapx) ;
#include "displayer.h"
#ifndef WEAROS
std::unique_ptr<Displayer> displayer;
#endif

extern void mkheights() ;
void	updateusedsensors(uint32_t nu) {
	static int wait=0;
	static int32_t waslast=-1;
	int newlast=sensors->last();
	if(waslast!=newlast||!wait) {
		LOGSTRING("updateusedsensors\n");
		waslast=newlast;
		setusedsensors(nu);
		wait=100;
		mkheights(); 
		}
	else
		wait--;
	
	}

//__attribute__((__visibility__("default"))) extern bool skipdisplay;
//bool skipdisplay=true;

//#define WEAROS
#ifndef WEAROS
extern bool showpers;
extern void showpercentiles(NVGcontext* genVG) ;
#endif
void  calccurvegegs();
void resetcurvestate() {
#ifndef WEAROS
   displayer.reset();
  #endif
  scantoshow={-1,nullptr}; 
    numlist=0;
#ifndef WEAROS
    showpers=false;
#endif
 selshown=false;
nrmenu=0;
 selmenu=0;
 emptytap=false;
 nrmenu=0,selmenu=0;
  calccurvegegs();
    }

static void withredisplay(NVGcontext* genVG,uint32_t nu)  {
/*
if(rotation!=0.0) {
	LOGGER("rotate %f\n",rotation);
	nvgTranslate(genVG, dheight/2.0f,dwidth/2.0f);
	nvgRotate(genVG,rotation);
	} */
    startstep(*getwhite());
#ifndef WEAROS
    if(showpers) {
		showpercentiles(genVG);
		}
	else 
#endif
	{

#ifdef WEAROSx
int oldtapx=tapx;
tapx=-8000;
#endif

	    if( !displaycurve(genVG,nu)&&( ((tapx
#ifdef WEAROSx

	    =oldtapx
#endif
	    		)>=0&&!selshown&&(selmenu=getmenu(tapx),true))||nrmenu)) {
		  showtext( genVG ,nu,selmenu) ;
		   }
		}
	tapx=-8000;


LOGAR("end withredisplay");
}
/*
void withoutredisplay(NVGcontext* genVG,uint32_t nu,uint32_t endtime)  {

	    if( (tapx>=0&&!selshown&&(selmenu=getmenu(tapx),true))||nrmenu) 
			  showtext( genVG ,nu,selmenu) ;
	 else  {
		    startstep(*getwhite());
		    if(showpers) {
				showpercentiles(genVG);
				}
		else
			displaycurve(genVG,nu,starttime, endtime);
		}
	tapx=-8000;
} */
time_t lastviewtime=0;
int onestep() {
	LOGAR("onestep");
	time_t nu=time(nullptr);
	lastviewtime=nu;
	updateusedsensors(nu);

	selshown=false;
	int ret=0;
	emptytap=false;

	void		shownumlist();
	if(showoldscan(genVG,nu)) {
		ret=1;
		}
	else
	{
#ifndef WEAROS
		if(numlist) {
			shownumlist();
			}
		else
#endif

		{
			withredisplay(genVG,nu);
		}
	}
#ifndef WEAROS
	if(displayer)
		ret=displayer->display();
#endif
	endstep();	
	return ret;
	}

extern void render() ;


int getalarmcode(const uint32_t glval,SensorGlucoseData *hist) ;
extern void     processglucosevalue(int sendindex,int newstart) ;

static bool dohealth(int sensorindex) {
    static int thesensor=sensorindex;
    if(!settings->data()->healthConnect)
        return false;
    if(usedsensors.size()==1)  {
        return true;
    }
    if(sensorindex==thesensor)
        return true;
    auto en=usedsensors.end();
    if(std::find(usedsensors.begin(),en,thesensor)==en) {
        thesensor=sensorindex;
        return true;
    }
    return false;
}
void     processglucosevalue(int sendindex,int newstart) {
//	if(!streamvalueshown) return;

extern	bool hasnotiset();
	if(settings) {
		if(!sensors)
			return;
		if(SensorGlucoseData *hist=sensors->getSensorData(sendindex)) {
			if(newstart>=0) {
				LOGGER("newstart=%d\n",newstart);
				hist->backstream(newstart);
				}
			if(const ScanData *poll=hist->lastpoll()) {
				const time_t tim=poll->t;
				if(!poll->valid()) {
					LOGGER("invalid value %s ",ctime(&tim));
					return;
					}
				const time_t nutime=time(nullptr);
				const int dif=nutime-tim;
				if(dif<maxbluetoothage) {
					if(!usedsensors.size())
						setusedsensors(nutime);


					const float glu= gconvert(poll->g*10);
					const int alarm=getalarmcode(poll->g,hist);
					
					sensor *senso=sensors->getsensor(sendindex);
				        bool wasnoblue=settings->data()->nobluetooth;
					int64_t startsensor=hist->getstarttime()*1000LL;
					LOGGER("processglucosevalue finished=%d,doglucose(%s,%d,%f,%f,%d,%lld,%d,%lld)\n", senso->finished,hist->shortsensorname()->data(),poll->g,glu,poll->ch,alarm,tim*1000LL,wasnoblue,startsensor);
					if(senso->finished) {
						senso->finished=0;
						backup->resensordata(sendindex);
						}
					settings->data()->nobluetooth=true;
					float rate=poll->ch;
extern void telldoglucose(const char *name,int32_t mgdl,float glu,float rate,int alarm,int64_t mmsec,bool wasnoblue,int64_t startsensor,intptr_t) ;

					telldoglucose(hist->shortsensorname()->data(),poll->g,glu,rate,alarm,tim*1000LL,wasnoblue,startsensor,dohealth(sendindex)?reinterpret_cast<intptr_t>(hist):0LL);

				//	wakeuploader();
extern				void wakewithcurrent();
					wakewithcurrent();

					}
				else {
					LOGGER("processglucosevalue too old %s ",ctime(&tim));
					LOGGER("dist=%d, dif=%d nu %s",maxbluetoothage,dif,ctime(&nutime));
					}
				}
			}
		else {
			LOGGER("processglucosevalue no sensor %d\n",sendindex);
			}
		}

	}


       #include <unistd.h>
          #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include <string.h>
//extern string_view filesdir;

extern std::string_view globalbasedir;
extern pathconcat sensorbasedir;
extern pathconcat logbasedir;


//int  abbottinit(std::string_view filesdir,JNIEnv *env=nullptr,jobject thiz=nullptr);
//int  abbottinit(std::string_view filesdir,JNIEnv *env=nullptr,jobject thiz=nullptr);
extern int  abbottinit();
void mkheights() {
	if(!settings)
		return;
	LOGSTRING("mkheights() \n");
	const int maxl= settings->getlabelcount();
	delete[] numheights;
	numheights=new int[maxl];
	int nr=0;
	for(int i=0;i<maxl;i++) {
		if(settings->getlabelweightmgperL(i)==0.0f) {
			numheights[i]=nr++;
			}
		else
			numheights[i]=-1;
		}
	shownlabels=nr;
	}

#include "net/backup.h"
#include "datbackup.h"
extern void setuseit();
extern void setusenl();
extern void setuseru() ;
extern void setusees();

extern void setusepl();
extern void setusede();

extern void setusezh() ;
extern void setuseuk() ;
extern void setusebe();
extern void setusefr();

extern void setusept() ;
extern void setuseiw() ;
extern void setuseeng() ;
extern std::string_view localestr;
extern bool hour24clock;
char localestrbuf[10];
std::string_view localestr;
bool hour24clock=true;

#define mklanguagenum2(a,b) a|b<<8
#define mklanguagenum(lang) mklanguagenum2(lang[0],lang[1])

bool chinese() {
	const int16_t lannum=mklanguagenum(localestrbuf);
	switch(lannum) {
		case mklanguagenum("ZH"):
		case mklanguagenum("zh"):
		return true;
		}
	return false;
	}

#ifdef USE_HEBREW
bool hebrew() {
	const int16_t lannum=mklanguagenum(localestrbuf);
	switch(lannum) {
		case mklanguagenum("IW"):
		case mklanguagenum("iw"):
		return true;
		}
	return false;
	}
#endif

#include "destruct.h"
void  setlocale(const char *localestrbuf,const size_t len) {
	LOGGER("locale=%s\n",localestrbuf);
	localestr={localestrbuf,len};
	/*
	#if defined(SIBIONICS) && !defined(WEAROS)
	destruct dest([] { usedtext->menustr0[5]=usedtext->sibionics;});
	#endif
	*/
	const int16_t lannum=mklanguagenum(localestrbuf);
	switch(lannum) {
		case mklanguagenum("DE"):
		case mklanguagenum("de"):
			setusede();
			break;
		case mklanguagenum("FR"):
		case mklanguagenum("fr"):
			setusefr();
			break;
		case mklanguagenum("IT"):
		case mklanguagenum("it"):
			setuseit();
			break;
		case mklanguagenum("NL"):
		case mklanguagenum("nl"):
			setusenl();
			break;

#ifdef SPANISH
		case mklanguagenum("ES"):
		case mklanguagenum("es"):
			setusees();
			break;
#endif
		case mklanguagenum("PT"):
		case mklanguagenum("pt"):
			setusept();
			break;
		case mklanguagenum("PL"):
		case mklanguagenum("pl"):
			setusepl();
			break;
		case mklanguagenum("BE"):
		case mklanguagenum("be"):
			setusebe();
			break;
		case mklanguagenum("UK"):
		case mklanguagenum("uk"):
			setuseuk();
			break;

#ifdef USE_RUSSIAN 
		case mklanguagenum("RU"):
		case mklanguagenum("ru"):
			setuseru() ;
			break;
#endif
#ifdef USE_HEBREW
		case mklanguagenum("IW"):
		case mklanguagenum("iw"):
			setuseiw();
			if(chfontset!=HEBREW) {
				initfont();
				}
			break;
#endif
		case mklanguagenum("ZH"):
		case mklanguagenum("zh"):
			setusezh();
			if(chfontset!=CHINESE) {
				initfont();
				}
			return; 
		default: setuseeng();
		};
	if(chfontset!=REST) {
		initfont();
	 	} 
#ifndef NOLOG
extern	void logstrings(const string_view lang) ;
//	logstrings(localestr);
#endif
	}

#include "strconcat.h"
#if 0
void logstrings(const string_view lang) {
extern		pathconcat logbasedir;
	strconcat file(string_view(""),logbasedir,"/","strings.",lang);
	FILE *fp=fopen(file.data(),"w");
	if(!fp)  {
		LOGGER("open %s failed\n",file.data());
		return;
		}
	fprintf(fp, R"(<string name="system_ui">%s</string>)" "\n", usedtext->menustr0[0].data());
    	fprintf(fp,R"(<string name="watch">%s</string>)" "\n", usedtext->menustr0[2].data());
    	fprintf(fp,R"(<string name="sensor">%s</string>)" "\n", usedtext->menustr0[3].data());
    	fprintf(fp,R"(<string name="settings">%s</string>)" "\n", usedtext->menustr0[4].data());
    	fprintf(fp,R"(<string name="aboutname">%s</string>)" "\n", usedtext->menustr0[5].data());
    	fprintf(fp,R"(<string name="export">%s</string>)" "\n", usedtext->menustr1[0].data());
    	fprintf(fp,R"(<string name="mirror">%s</string>)" "\n", usedtext->menustr1[1].data());
    	fprintf(fp,R"(<string name="new_amount">%s</string>)" "\n", usedtext->menustr1[2].data());
    	fprintf(fp,R"(<string name="list">%s</string>)" "\n", usedtext->menustr1[3].data());
    	fprintf(fp,R"(<string name="statistics">%s</string>)" "\n", usedtext->menustr1[4].data());
    	fprintf(fp,R"(<string name="talk">%s</string>)" "\n", usedtext->menustr1[5].data());
    	fprintf(fp,R"(<string name="floatname">%s</string>)" "\n", usedtext->menustr1[6].data());

    	fprintf(fp,R"(<string name="last_scan">%s</string>)" "\n", usedtext->menustr2[0].data());

    	fprintf(fp,R"(<string name="date">%s</string>)" "\n", usedtext->menustr3[2].data());

    	fprintf(fp,R"(<string name="day_back">%s</string>)" "\n", usedtext->menustr3[3].data());
    	fprintf(fp,R"(<string name="day_later">%s</string>)" "\n", usedtext->menustr3[4].data());
    	fprintf(fp,R"(<string name="week_back">%s</string>)" "\n", usedtext->menustr3[5].data());
    	fprintf(fp,R"(<string name="week_later">%s</string>)" "\n", usedtext->menustr3[6].data());
	fclose(fp);
}
#endif

void  calccurvegegs() {
	LOGSTRING("start calccurvegegs\n");
	mkheights(); 
	starttime=maxtime()-4*duration/5;
	setusedsensors();
	LOGSTRING("end calccurvegegs\n");
	}

void		numendbegin() ;
void flingX(float vol) {
#ifndef WEAROS
	if(numlist)  {
		LOGSTRING("flingX\n");
		if(vol<0) {
			numlist=1;
 			numendbegin() ;
			}
		return;
		}
#endif

//	starttime-=(duration*1.2*vol/dwidth);
	setstarttime(starttime-(duration*1.2*vol/dwidth));
#ifndef WEAROS
	if(!showpers)
#endif
		begrenstijd() ;
	}

bool				numpageforward() ;
			void	scrollnum() ;


bool numpagepast() ;

int translate(float dx,float dy,float yold,float y) {
static bool ybezig=false;
	auto absdy=fabsf(dy);
	if(fabsf(dx)>absdy) {
#ifndef 	WEAROS
		if(numlist) {
			auto tim = std::chrono::system_clock::now();
			static decltype(tim) oldtim{};
		        std::chrono::duration<double> dif=tim-oldtim;
			const double grens=.8;
			
			if(dif.count()>grens) {
				if(dx<-10) 
					numpagepast();
				else
					if(!numpageforward())
						return 0;
				oldtim=tim;
				return 1;
				}
			else
				return 0;

			}
		else
#endif
		{
			ybezig=false;
			setstarttime(starttime+1.2*(dx/dwidth)*duration);
			#ifndef WEAROS
			if(!showpers)
			#endif
				begrenstijd() ;
			return 1;
			}
		}

	else {	
		{
		if(fixatey)
			return 0;
		if(ybezig||(dheight/absdy)<convfactor) {
			ybezig=true;
			dy*=-1;
			float grens=dheight/2.0;

			if(y<grens&&yold<grens) {
				grange*=dheight/(dheight-dy*1.4);
				settime=starttime;
				setend=starttime+duration;
				}
			else if(y>grens&&yold>grens) {
				int gmax=gmin+grange;
				grange*=dheight/(dheight+dy*1.4);
				gmin=gmax-grange;
					settime=starttime;
					setend=starttime+duration;
					}
			if(grange<180)
				grange=180;
			return 1;
			}
			}
		}
	return 0;
	}
void xscaleGesture(float scalex,float midx) {
	if(fixatex)
		return;

	double rat=((midx-dleft)/dwidth);
	double oldduration=duration;
	uint32_t focustime=rat*oldduration+starttime;
	duration=(int)round(oldduration/pow(scalex,5.0));
	LOGGER("xscale scale=%f mid=%f oldduration=%f newduration=%d\n",scalex,midx,oldduration,duration);
	setstarttime(focustime-rat*duration);
	auto maxstart= maxstarttime();
	if(
#ifndef WEAROS
	!showpers&&
#endif

	starttime>maxstart)
		setstarttime(maxstart);
	setend=0;

	
	}


void prevscr() {
	setstarttime(starttime-duration);
	auto minstart= minstarttime();
	if(starttime<minstart)
		setstarttime(minstart);
	}
void  nextscr() {
	setstarttime(starttime+duration);
#ifndef WEAROS
	if(!showpers) 
#endif

	{
		auto maxstart= maxstarttime(); 
		if(starttime>maxstart) 
			setstarttime(maxstart);
		}
	}
static int64_t menutap(float x,float y) ;

bool inbutton(float x,float y) {
	return !(x<menupos.left||x>=menupos.right|| y<menupos.top||y>=menupos.bottom) ;
	}

struct {
	float x=-300.0f,y=-300.0f;
	std::chrono::time_point<std::chrono::steady_clock>  time;
	} prevtouch;



void pressedback() {
	scantoshow={-1,nullptr}; 
	LOGSTRING("true\n");

#ifndef WEAROS
	displayer.reset();
#endif
	}
bool isbutton(float x,float y) {
	LOGSTRING("isbutton ");
	if(!inbutton(x,y)) {
		LOGSTRING("false\n");
		return false;
		}
	if(restart) {
		LOGSTRING("restart\n");
		exit(1);	
		}
	if(inbutton(prevtouch.x,prevtouch.y)) {
			 auto nutime = chrono::steady_clock::now();
			 if(chrono::duration_cast<chrono::milliseconds>(nutime - prevtouch.time).count()<450) // don't close immediately if OK sits on pressed point
				return true;
			}
	scantoshow={-1,nullptr}; 
	LOGSTRING("true\n");
#ifndef WEAROS
	displayer.reset();
#endif
//	lastscan=nullptr;
	return true;
	}

#include "numiter.h"
NumIter<Num> *numiters=nullptr;
int basecount;

extern void speak(const char *message) ;
#ifndef NDEBUG
#define lognum(x)
#else
void lognum(const Num *num) {
		constexpr int maxitem=80;
		char item[maxitem];
		time_t tim=num->time;
		int itemlen=datestr(tim,item);
		if(num->type< settings->getlabelcount()) {
			item[itemlen++]=' ';
			decltype(auto) lab=settings->getlabel(num->type);
			memcpy(item+itemlen,lab.data(),lab.size());
			itemlen+=lab.size();
			item[itemlen]='\0';
			}
		LOGGER("%s %.1f\n",item,num->value);
		}
#endif	
int numfrompos(const float x,const float y) ;
vector<mealposition> mealpos;
static int verbosedate(time_t tim,char *buf,int maxbuf=256) {
	struct tm tmbuf;
	struct tm *stm=localtime_r(&tim,&tmbuf);
	const auto wdaynr= stm->tm_wday;
	const char *dayname=usedtext->speakdaylabel[wdaynr];
	return snprintf(buf,maxbuf,"%s %d %s %d",dayname,stm->tm_mday,usedtext->monthlabel[stm->tm_mon],1900+stm->tm_year);
	}

static void speakdate(time_t tim) {
	constexpr const int maxbuf=256;
	char buf[maxbuf];
	verbosedate(tim,buf,maxbuf);
	LOGGER("speakdate %s\n",buf);
	speak(buf);
	}
int64_t screentap(float x,float y) {

#ifndef WEAROS
	if(!showpers ) 
	{

		if(numlist)  {

			if(int index=numfrompos(x,y);index>=0) {
				const Num *num=numiters[index].prev();
				if(!numdatas[index]->valid(num))
					return -2LL;

				int pos=num-numdatas[index]->startdata();
				int base=numdatas[index]->getindex();
				return (static_cast<jlong>(pos)<<16)|((static_cast<jlong>(base)&0xf)<<8)|0xe;
				}
			else
				return -2LL;
			}
#else
	{
#endif

		if(emptytap) {
			return -1LL;
			}
		if(nrmenu)  {
			return menutap(x,y);
			}
		if(showmeals) {
			const float crit= (density*10);
			for(mealposition &p:mealpos) {
				if((y>=p.mealstarty&&y<p.mealendy)&&abs(x-p.mealx)<crit)
					return (static_cast<jlong>(p.mealpos)<<16)|((static_cast<jlong> (p.mealbase)&0xf)<<8)|0xe;
				}

			}
#ifndef WEAROS
		if(speakout) {
			for(auto &el:shownglucose) {
				LOGGER("x=%f [%f,%f] y=%f [%f,%f] trend=%d\n", x,el.glucosevaluex,
					   (el.glucosevaluex + headsize), y, (el.glucosevaluey - headsize),el.glucosevaluey,
					  el.glucosetrend);
				if (el.glucosevaluex > 0 && x > el.glucosevaluex && x < (el.glucosevaluex + headsize*1.2f) &&
					y < el.glucosevaluey && y > (el.glucosevaluey - headsize*.8f)) {
					if(el.glucosevalue > 0) {
						constexpr const int maxvalue = 80;
						char value[maxvalue];
						auto trend = usedtext->trends[el.glucosetrend];
						memcpy(value, trend.data(), trend.size());
						char *ptr = value + trend.size();
						*ptr++ = '\n';
						*ptr++ = '\n';
						snprintf(ptr, maxvalue, gformat, el.glucosevalue);
						speak(value);
						return -1LL;
					} else {
						const char *error=el.errortext;
						if(error)  {
							speak(error);
							return -1LL;
							}
					}
				}
			}
		}
#endif
		}
#ifndef WEAROS
	if(speakout) {
		 const float hgrens=menutextheight;
		if(y<hgrens) {
			 const float wgrens=menutextheight*1.5f;
			if(x<wgrens) {
				speakdate(starttime);
				return -1LL;
				}
			else {
			 	if(x>(dwidth-wgrens)){
					const time_t endtime=starttime+duration;
					const time_t nu=time(nullptr); 
					if(endtime>nu)
						speakdate(nu);
					else
						speakdate(endtime);
					return -1LL;
					}
				}
			}
		}
	const float wgrens=density*10;
	const float rgrens=dwidth-wgrens;

	if(x<wgrens)  {
			prevscr();
			return -1LL;
			}
		
	else 
	    if (x > rgrens)  {
		nextscr();
		return -1LL;
		}
	    
	if(showpers)  {

extern bool showsummarygraph;
		if(showsummarygraph) {
			showsummarygraph=false;
	      		fixatey=settings->data()->fixatey;
			return 1+4*0x10;
			}
		}
	else
#endif
	{
		tapx=x;tapy=y;
		}
	return -1LL;
	}

Num newnum;
#include "numhit.h"
NumHit newhit={nullptr,&newnum};

int  hitremove(int64_t ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	 jint res=num->numdisplay->numremove(const_cast<Num*>(num->hit));
	 if(numlist) {
	 	for(int i=0;i<basecount;i++) {
			numiters[i].end=numdatas[i]->end()-1;
			}
	 	}

	 return res;
	}
extern Numdata *getherenums();
Numdata *getherenums() {
	return newhit.numdisplay;
	}
template <class TX,class TY> NumHit *nearbynum(const float tapx,const float tapy,const TX &transx,  const TY &transy) {
	 for(auto el:numdatas) 
		if(const Num *hit=el->getnearby(transx,transy,tapx,tapy)) {
			return new  NumHit({el,hit});
			}
	 return nullptr;
	}

template <class TX,class TY> const ScanData * nearbyscan(const float tapx,const float tapy,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy) {
	for(const ScanData *it=low;it!=high;it++) {
		if(it->valid()) {
			const uint32_t tim= it->t;
			const auto glu=it->g*10;
			const auto posx= transx(tim),posy=transy(glu);
			if(nearby(posx-tapx,posy-tapy))  {
				return it;
				}
			}
		}
	return nullptr;
	}
void showOK(float xpos,float ypos) {
	nvgFontSize(genVG,headsize/4 );

	const char ok[]="OK";
	const int oklen=sizeof(ok)-1;
	nvgTextBounds(genVG, xpos,ypos ,ok , ok+oklen, (float *)&menupos);

	nvgText(genVG, xpos,ypos,ok,ok+oklen);
	menupos.left-=mediumfont;
	menupos.right+=mediumfont;
	menupos.bottom+=mediumfont;
	menupos.top-=mediumfont;
	}
template <typename  TI,typename TE>
void textbox(const TI &title,const TE &text) {
	float w=dwidth*0.6;
// 	nvgRoundedRect(genVG,  x,  y,  w,  h,  r);
//	x+=smallsize;
	nvgFontFaceId(genVG,font);
	bounds_t bounds;
	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	nvgFontSize(genVG, smallsize);
	nvgTextLineHeight(genVG, 1.7);
	 nvgTextBoxBounds(genVG, 0,  0, w,begin(text), end(text), bounds.array);
	nvgBeginPath(genVG);
	float width= bounds.xmax-bounds.xmin+ smallsize;
	float height= bounds.ymax-bounds.ymin+sensorbounds.height*2;
	float x=(dwidth-width)/2;
	float y=(dheight-height)/2;
	nvgFillColor(genVG, red);
 	nvgRoundedRect(genVG,  x-smallsize, y-smallsize,  width+2*smallsize, height+2*smallsize, dwidth/60 );
	nvgFill(genVG);
	nvgFillColor(genVG, *getblack());
	nvgTextBox(genVG,  x,  y+sensorbounds.height+smallsize, width, begin(text),end(text));
	nvgFontSize(genVG, mediumfont);

	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
   auto *beg=begin(title);
   if(*beg) {
      int siz=sizear(title);
      nvgText(genVG, x,y, beg,beg+siz);
      }
	nvgTextAlign(genVG,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	showOK(x+width,y);
	}

class histgegs:public Displayer {
   const int sensorindex;
	const SensorGlucoseData *hist;
	time_t nu;
#ifndef WEAROS
strconcat text;
#endif
public:
	histgegs(const int sensorindex,const SensorGlucoseData *hist): sensorindex(sensorindex),hist(hist)/*,glu(glu),tim(tim)*/,nu(time(nullptr))
#ifndef WEAROS
    ,text(getsensorhelp(usedtext->menustr0[3],": ","\n","\n"," "))
#endif
    {


	 prevtouch.time = chrono::steady_clock::now();
	LOGGER("histgegs %s",ctime(&nu));
	} 
strconcat  getsensorhelp(string_view starttext,string_view name1,string_view name2,string_view sep1,string_view sep2) {
	char starts[50],ends[50],pends[50];
   const sensor *sensor=sensors->getsensor(sensorindex);
	time_t stime=hist->getstarttime(),etime= hist->officialendtime(),reallends=hist->isLibre3()?etime:sensor->maxtime();
	char lastscanbuf[50],lastpollbuf[50];
	time_t lastscan=hist->getlastscantime();
	time_t lastpolltime=hist->getlastpolltime();
	return strconcat(string_view(""),starttext ,name1,hist->showsensorname(),name2,usedtext->sensorstarted,sep2,string_view(starts, datestr(stime,starts)),!hist->isLibre2()?"":sep1,!hist->isLibre2()?"":usedtext->lastscanned,!hist->isLibre2()?"":sep2,!hist->isLibre2()?"":string_view(lastscanbuf,datestr(lastscan,lastscanbuf)),lastpolltime>0?strconcat(string_view(""),sep1,usedtext->laststream,sep2):"",lastpolltime>0?string_view(lastpollbuf,datestr(lastpolltime,lastpollbuf)):"",nu<etime?strconcat(string_view(""),sep1,usedtext->sensorends,sep2):"",
nu<etime?string_view(ends, datestr(etime,ends)):string_view("",0),sep1,usedtext->sensorexpectedend,sep2,string_view(pends, datestr(reallends,pends)));;
	}

#ifndef WEAROS
virtual int display() override {
//	textbox(usedtext->history,getsensorhelp(hist->isLibre3()?usedtext->history3info:usedtext->historyinfo,"","\n","\n"," "));
//	textbox(usedtext->history,text);
	textbox("",text);
	return 1;
	}
void speak() {
	LOGGER("speak %s\n",text.data());
	::speak(text.data());
   }
#else
virtual int display() override {
	return 1;
	}
#endif

};
//histgegs gegs(
strconcat getsensortext(const int sensorindex,const SensorGlucoseData *hist) {
		histgegs gegs(sensorindex,hist);
		return gegs.getsensorhelp("","<h1>","</h1>","<br><br>","<br>");
		}

static void showhistory(const int sensorindex,const SensorGlucoseData *hist,const float tapx, const float tapy) {
#ifdef WEAROS
						histgegs gegs(sensorindex,hist);

extern void callshowsensorinfo(const char *text);
						callshowsensorinfo(gegs.getsensorhelp("","<h1>","</h1>","<br><br>","<br>").data());
#else
						::prevtouch.x=tapx;
						::prevtouch.y=tapy;
						LOGGER("x=%.1f, y=%.1f\n",tapx,tapy);
						histgegs *gegs=new histgegs(sensorindex,hist);
						if(speakout) gegs->speak();
						displayer.reset(gegs);
#endif
      }

template <class TX,class TY> 
bool nearbyhistory( const float tapx,const float tapy,  const TX &transx,  const TY &transy) {
	for(int i=histlen-1;i>=0;i--) {
      const int sensorindex= hists[i];
		const SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
		const auto [firstpos,lastpos]=histpositions[i];
			for(auto pos=firstpos;pos<=lastpos;pos++) {
				uint32_t tim,glu;
				if((tim=hist->timeatpos(pos))&&( glu=hist->sputnikglucose(pos))) {
					auto posx=transx(tim),posy=transy( glu);
					if(nearby(posx-tapx,posy-tapy)) {
                  showhistory(sensorindex,hist,tapx,tapy);
						return true;
						}
					}
				}
		}
	return false;
	}

static bool  inmenu(float x,float y) ;


	static bool speakmenutap(float x,float y) ;

static int largepausedaystr(const time_t tim,char *buf) {
        LOGAR("largedaystr");
	struct tm stmbuf;
	localtime_r(&tim,&stmbuf);
 //	return sprintf(buf,"%02d:%02d\n%s %02d %s %d",stmbuf.tm_hour,mktmmin(&stmbuf),usedtext->speakdaylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year);
 	return sprintf(buf,"%s %02d %s %d\n%02d:%02d",usedtext->speakdaylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year,stmbuf.tm_hour,mktmmin(&stmbuf));
	}
void speaknum(const Num *num) {
	char buf[256];
	char *ptr=buf;
	auto label=settings->getlabel(num->type);
	memcpy(ptr,label.data(),label.size());
	ptr+= label.size();
	*ptr++='\n';
	ptr+=sprintf(ptr,"%g",num->value);
	*ptr++='\n';
	*ptr++='\n';
	int len=largepausedaystr(num->gettime(),ptr);
	LOGGERN(buf,ptr-buf+len);
	speak(buf);
	}
int64_t longpress(float x,float y) {
	LOGSTRING("longpress\n");
#ifndef WEAROS
	if(showpers)
		return 0LL;
	if(numlist)  {
		if(speakout) {
			if(int index=numfrompos(x,y);index>=0) {
				const Num *num=numiters[index].prev();
				if(numdatas[index]->valid(num)) {
					speaknum(num);
					}
				}
			}
		return 0LL;
		}
#endif
#ifndef WEAROS
	if(speakout) {
		if(speakmenutap(x,y))
			return 0LL;
		}
	else
#endif
	{
		if(inmenu(x,y)) {
			return 0LL;
			}
	  	}
	const uint32_t endtime=starttime+duration;
	const auto [transx,transy]= gettrans(starttime, endtime);
	if(shownumbers)
		if(NumHit *hit= nearbynum(x,y,transx,transy))
			return reinterpret_cast<jlong>(hit);
	if(showhistories&& nearbyhistory( x,y,  transx,  transy) ) {
		return 0LL;
		}
	if(showscans) {
		for(int i=histlen-1;i>=0;i--) {
			if(const ScanData *scan=nearbyscan(x,y,scanranges[i].first,scanranges[i].second,transx,transy)) {
				LOGGER("longpress scan %.1f\n",scan->g/convfactordL);
				int index=hists[i];
				scantoshow={index,scan,static_cast<uint32_t>(time(nullptr))};
				return 0LL;
				}
			 }
		 }
	if(showstream) {
		for(int i=histlen-1;i>=0;i--) {
			if(const ScanData *poll=nearbyscan(x,y,pollranges[i].first,pollranges[i].second,transx,transy)) {
				LOGGER("longpress poll %.1f\n",poll->g/convfactordL);
                const int sensorindex= hists[i];
		      const SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
            showhistory(sensorindex,hist,x,y);
				return 0LL;
				}
			 }
		 }

	int type=typeatheight(y); 
	if(type>=0)  {
		newnum.time=starttime+duration*(x-dleft)/dwidth;
		newnum.type=type;
		newnum.value=NAN;
		return reinterpret_cast<jlong>(&newhit);
		}
	return 0LL;
	}


static uint32_t timeend() {
	return starttime+duration;	
	}

uint32_t starttimefromtime(uint32_t pos) {
	return starttime+floor(((double)pos-starttime)/duration)*duration;
	}


void numpagenum(const uint32_t tim) ;

static void highlightnum(const Num *num) {
	uint32_t tim=num->time;
#ifndef WEAROS
	if(numlist)
		numpagenum(tim) ;
	else
#endif
		setstarttime(starttimefromtime(tim));
	}

static uint32_t glucosesearch(uint32_t starttime,uint32_t endtime) ;
static uint32_t glucoseforwardsearch(uint32_t starttime,uint32_t endtime) ;

template <int N>
const Num *findpast() {
	const Num  *hit=nullptr;
	const int len=numdatas.size();
	int i=0;
	for(;i<len;i++) {
		if((hit=numdatas[i]->findpast<N>()))
			break;
		}
	if(hit) {
		for(;i<len;i++) {
			if(const Num *mog=numdatas[i]->findpast<N>();mog&&mog->time>hit->time)
				hit=mog;
			}
		}
	return hit;
	}

template <int N=1>
const Num *findforward()  {
	const Num  *hit=nullptr;
	const int len=numdatas.size();
	int i=0;
	for(;i<len;i++) 
		if((hit=numdatas[i]->findforward<N>()  ))
			break;

	if(hit) {
		for(;i<len;i++)  {
			if(const Num *mog=numdatas[i]->findforward<N>();mog&&mog->time<hit->time)
				hit=mog;
			}
		}
	return hit;
	}


int nextpast() {
	if(searchdata.type&glucosetype) {
		 return glucosesearch(0,starttime-1);
		}
	else {
		if(const Num *hit=findpast<0>()) {
			highlightnum(hit);
			return 0;
			}
		}
	return 1;
	}


int nextforward() {
	if(searchdata.type&glucosetype) {
		return glucoseforwardsearch(starttime+duration, std::numeric_limits<uint32_t>::max());
		}
	else {
		if(const Num *hit=findforward()) {
			highlightnum(hit);
			return 0;
			}
		}
	return 1;			
	}
void stopsearch() {
	searchdata.type=nosearchtype;
	}
static const ScanData * findScan(const ScanData *start,const ScanData *en) {
	for(const ScanData *it=en-1;it>=start;--it) {
		if(searchdata(it))
			return it;
			/*
		int32_t glu=it->g;
		if(glu&&glu>=low&&glu<=high)
			return it;
			*/
		}
	return nullptr;
 }
static const Glucose * findhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
	for(auto pos=lastpos;pos>=firstpos;--pos)  {
		const Glucose *g=hist->getglucose(pos);
		if(searchdata(g))
			return g;

		}
	return nullptr;
	}
static void glucosesel(uint32_t tim) {
	if(tim>starttime&&tim<timeend())
		return;
	setstarttime(starttimefromtime(tim));
	}
#ifndef NDEBUG
void logglucose(const char *str,const Glucose *glu) {
	const time_t tim=glu->gettime();
	LOGGER("%s: %d %.1f %s",str, glu->id,glu->getsputnik()/100.0f,ctime(&tim));
	}
#else
#define	logglucose(x,y)
#endif
static uint32_t glucosesearch(uint32_t starttime,uint32_t endtime) {
	LOGGER("glucosesearch(%u,%u)\n",starttime, endtime);
	uint32_t hittime=starttime;
	const Glucose *histhit=nullptr;
	const ScanData *scanhit=nullptr;
	for(int it=sensors->last();it>=0;it--) {
		const sensor *sen=sensors->getsensor(it);
		LOGGER("Sensor %s\n",sen->name);
		if(sen->starttime>endtime)
			continue;
		if(sen->endtime&&sen->endtime<starttime)
			break;
		if(auto his=sensors->getSensorData(it)) {
			int32_t lastpos= his->getAllendhistory()-1;
			uint32_t tim=0;
			int32_t firstpos = his->getstarthistory();
			if(lastpos<firstpos) {
				goto skiphistory;
				}
			for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
				if(lastpos<=firstpos)
					break;
				}
			if(tim<hittime)  {
				//continue;
				goto skiphistory;
				}
			if((searchdata.type&historysearchtype)==historysearchtype) {
				int endpos;
				if(tim<endtime)
					endpos=lastpos;
				else {
					int period=his->getinterval();
					endpos=lastpos-(tim-endtime)/period;
					if(endpos<1)
						endpos=1;	
					}
				while(endpos<lastpos&&	his->timeatpos(endpos)<endtime)
					endpos++;
				uint32_t tmptim;
				while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
					endpos--;
					if(endpos<=firstpos)
						goto skiphistory;
					}

				int startpos=his->gettimepos(hittime);
				if(startpos<1) 
					startpos=1;
				else {
					
					while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=hittime))
						startpos--;
					while(startpos<endpos&&	his->timeatpos(startpos)<hittime)
						startpos++;
					}
				 const Glucose *mog=findhistory(his,startpos,endpos); 
				 if(mog&&mog->gettime()>hittime) {
					histhit=mog;
					hittime=mog->gettime();
					logglucose("glucosesearch mog ",mog);
					}
				}
			skiphistory:
			if((searchdata.type&scansearchtype)==scansearchtype) {
				std::span<const ScanData> 	scan=his->getScandata();
				auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
				const ScanData *mogscan=findScan(under,above);
				if(mogscan&&mogscan->t>hittime) {
					scanhit=mogscan;
					hittime=mogscan->t;
					}
					}
			if((searchdata.type&streamsearchtype)==streamsearchtype) {
				std::span<const ScanData> 	scan=his->getPolldata();
				auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
				const ScanData *mogscan=findScan(under,above);
				if(mogscan&&mogscan->t>hittime) {
					scanhit=mogscan;
					hittime=mogscan->t;
					}
				   }
		}
	   }

	uint32_t res;
	if(!histhit)	{
		if(!scanhit)  {
			LOGSTRING("no hist and no scanhit\n");
			return 1;
			}
		LOGSTRING("no scan hist and but scanhit\n");
		res=scanhit->t;
		}
	else {
		LOGSTRING("hist hit\n");
		if(!scanhit||histhit->time>scanhit->t)
			res=histhit->time;
		else
		 	res=scanhit->t;
		 }
	LOGGER("glucosesearch found %d\n",res);
	glucosesel(res);
	return 0;
	}

static const ScanData * findforwardScan(const ScanData *start,const ScanData *en) {
	for(const ScanData *it=start;it<en;++it) {
		if(searchdata(it))
			return it;
		}
	return nullptr;
 }
static const Glucose * findforwardhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
	for(auto pos=firstpos;pos<=lastpos;++pos)  {
		const Glucose *g=hist->getglucose(pos);
		if(searchdata(g))
			return g;

		}
	return nullptr;
	}

static uint32_t glucoseforwardsearch(uint32_t starttime,uint32_t endtime) {
	uint32_t hittime=endtime;
	const Glucose *histhit=nullptr;
	const ScanData *scanhit=nullptr;
	for(int it=0;it<=sensors->last();it++) {
		const sensor *sen=sensors->getsensor(it);
		LOGGER("Sensor %s\n",sen->name);
		if(sen->starttime>hittime)
			break;
		if(sen->endtime<starttime)
			continue;
		auto his=sensors->getSensorData(it);
		int32_t lastpos= his->getAllendhistory()-1;
		int32_t firstpos= his->getstarthistory();
		uint32_t tim=0;
		if(lastpos<firstpos) {
			goto skiphistory;
			
			}
		for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
			if(lastpos<=firstpos)
				break;
			}
				
		if(tim<starttime) {
			goto skiphistory;
			}

	if((searchdata.type&historysearchtype)==historysearchtype) {
		int endpos;
		if(tim<hittime)
			endpos=lastpos;
		else {
			int period=his->getinterval();
			endpos=lastpos-(tim-hittime)/period;
			if(endpos<1)
				endpos=1;	
			}
		while(endpos<lastpos&&	his->timeatpos(endpos)<hittime)
			endpos++;
		uint32_t tmptim;
		while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
			endpos--;
			if(endpos<=firstpos)
				goto skiphistory;
			}

		int startpos=his->gettimepos(starttime);
		if(startpos<1) 
			startpos=1;
		else {
		      uint32_t tmptim;
			while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=starttime))
				startpos--;
			while(startpos<endpos&&	his->timeatpos(startpos)<starttime)
				startpos++;
			}
		 const Glucose *mog=findforwardhistory(his,startpos,endpos); 
		 if(mog&&mog->gettime()<hittime) {
			histhit=mog;
			hittime=mog->gettime();
			}
		}
	skiphistory:
	if((searchdata.type&scansearchtype)==scansearchtype) {
		const std::span<const ScanData> 	scan=his->getScandata();
		auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
		const ScanData *mogscan=findforwardScan(under,above);
		if(mogscan&&mogscan->t<hittime) {
			scanhit=mogscan;
			hittime=mogscan->t;
			}
	   	    }

	if((searchdata.type&streamsearchtype)==streamsearchtype) {
		const std::span<const ScanData> 	scan=his->getPolldata();
		auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
		const ScanData *mogscan=findforwardScan(under,above);
		if(mogscan&&mogscan->t<hittime) {
			scanhit=mogscan;
			hittime=mogscan->t;
			}
	   	    }





	   }



	uint32_t res;
	if(!histhit)	{
		if(!scanhit) 
			return 1;
		res=scanhit->t;
		}
	else {
		if(!scanhit||histhit->time<scanhit->t)
			res=histhit->time;
		else
		 	res=scanhit->t;
		 }

	glucosesel(res);
	return 0;
	}
int searchcommando(int type, float under,float above,int frommin,int tomin,bool forward,const char *regingr,float amount) {
if(type&glucosetype) {
	searchdata={type ,backconvert(under), backconvert(above), frommin, tomin,0};
	return forward?glucoseforwardsearch(starttime, std::numeric_limits<uint32_t>::max()):glucosesearch(0,starttime+duration);
	}
auto maxlab=getmaxlabel();
if(type>=maxlab)
	type=0x80000000;
searchdata={ type, under, above, frommin, tomin,maxlab};
if(regingr!=nullptr&&type==carbotype) {
	meals->datameal()->searchingredients(regingr,searchdata.ingredients);
	if(searchdata.ingredients.size()==0) {
		searchdata.type=nosearchtype;
		return 4;
		}
	searchdata.amount=amount;
	}
else
	searchdata.ingredients.clear();
if(const Num *hit=forward?findforward<0>():findpast<1>()) {
	highlightnum(hit);
	return 0;
	}
searchdata.type=nosearchtype;
return 1;
}

static constexpr const int day=60*60*24;
void prevdays(int nr) {
	//starttime=starttimefromtime(starttime-nr*day);
	setstarttime(starttime-nr*day);
	auto minstart= minstarttime();
	if(starttime<minstart)
		setstarttime(minstart);
	}
void nextdays(int nr) {
	//starttime=starttimefromtime(starttime+day*nr);
	setstarttime(starttime+day*nr);
#ifndef WEAROS
	if(!showpers) 
#endif
	{
		auto maxstart= maxstarttime(); 
		if(starttime>maxstart) 
			setstarttime(maxstart);
		}
	}

//constexpr int hourminstrlen=20;
//static char hourminstr[hourminstrlen]="00:00        ";
//static char hourminstr[hourminstrlen]="00:00       ";
#ifdef WEAROS
#define hourtext "00:00           "
#else
#define hourtext "00:00             "
#endif
//constexpr const int hourtextlen=sizeof(hourtext)-1;
char hourminstr[hourminstrlen]=hourtext;
void setnowmenu(time_t nu) {
	hourmin(nu,hourminstr);
	const int ulen=usedsensors.size();
	if(ulen>0) {
		for(int i=0;i<ulen;) {
			if(const auto *lastin=sensors->getSensorData(usedsensors[i++])->lastpoll()) {
				for(;i<ulen;i++) {
					if(const auto *lastsen=sensors->getSensorData(usedsensors[i])->lastpoll();lastsen&&(lastsen->t>lastin->t)) {
						lastin=lastsen;
						}
					}
				if(lastin->t>(nu-maxbluetoothage)) {
					auto nonconvert= lastin->g;
				//	auto nonconvert= 500;
const int  trend=lastin->tr;
//const int  trend=5;

constexpr const char arrows[][sizeof("→")]{"",
"↓",
"↘",
"→",
"↗",
"↑"}; 

#if __NDK_MAJOR__ >= 26

constexpr const int trendoff=
#ifdef WEAROS
0
#else
1
#endif
;
	char *ptr=hourminstr+

#ifdef WEAROS
	6;
#else
	7;
#endif
	for(auto iter=hourminstr+5;iter<ptr;++iter)
		*iter=' ';

	const int trendlen=sizeof(arrows[trend])-1;
	memcpy(ptr,arrows[trend],trendlen);
	static std::to_chars_result res={.ptr=nullptr};
	char *oldres=res.ptr;
	auto value=gconvert(nonconvert*10);
	res=std::to_chars(ptr+trendlen+trendoff,hourminstr+hourminstrlen,value,std::chars_format::fixed,gludecimal);
	/*
	if(gludecimal)  {
		auto round=roundf(value*10.0f)/10.0f;
		res=std::to_chars(ptr+trendlen+trendoff,hourminstr+hourminstrlen,round);
		 if(res.ptr[-2]!='.') {
			memcpy(res.ptr,".0",2);
			res.ptr+=2;
			}
		}
	else {
		res=std::to_chars(ptr+trendlen+trendoff,hourminstr+hourminstrlen,value);
		}
*/
	for(auto it=res.ptr;it<oldres;++it)
		*it=' ';
					LOGGER("new hourminstr=%s\n", hourminstr);
#else
	static		int oldend=0;
	auto aftertime=hourminstr+5;
#ifdef WEAROS
				int endpos=snprintf(aftertime,hourminstrlen-5," %s%.*f",arrows[trend],gludecimal,gconvert(nonconvert*10));
#else
			int endpos=snprintf(aftertime,hourminstrlen-5,"  %s %.*f",arrows[trend],gludecimal,gconvert(nonconvert*10));
#endif
	aftertime[endpos]=' ';
	for(int i=endpos+1;i<oldend;i++)
		aftertime[i]=' ';
	
	oldend=endpos;

					LOGGER("old hourminstr=%s\n", hourminstr);
#endif
					return ;

					}
				}
			}
		}
	memset(hourminstr+5,' ',hourminstrlen-6); 
	}
//int notify=false;

#define arsizer(x) sizeof(x)/sizeof(x[0])

#ifdef WEAROS

const int *menuopt0[]={nullptr,nullptr,&invertcolors, nullptr,nullptr};
const int **optionsmenu[]={menuopt0,nullptr};
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr2)};
int getmenulen(const int menu) {
	int len=menulen[menu];
//	if(menu==1&&settings->staticnum()) return len-1;
	if(!menu&&!alarmongoing)
		return len-1;
		
	return len;	
	}

void setfloatptr() {
	}
#else
int menus=0;
const int *menuopt0[]={&showui,&menus,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};


const int *menuopt0b[]={nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
const int *menuopt1[]={nullptr,&showscans, &showstream,&showhistories, &shownumbers,&showmeals,&invertcolors};
const int **optionsmenu[]={menuopt0,menuopt0b,menuopt1,nullptr};
#define arsizer(x) sizeof(x)/sizeof(x[0])
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr1),arsizer(jugglucotext::menustr2),arsizer(jugglucotext::menustr3)};
int getmenulen(const int menu) {
	int len=menulen[menu];
	if(!menu&&!alarmongoing)
		return len-1;
	return len;	
	}
void setfloatptr() {
	menuopt0b[6]=&settings->data()->floatglucose;
	}

void setnewamount() {

/*
	int newamountoff=
#ifdef WEAROS
	3;
#else
	2;
#endif
	if(settings->staticnum()) {
		usedtext->menustr1[newamountoff]=nothing;
		}
	else
		usedtext->menustr1[newamountoff]=usedtext->newamount;
		*/
	}
#endif


constexpr const int maxmenulen= *std::max_element(std::begin(menulen),std::end(menulen));
constexpr int maxmenu=arsizer(jugglucotext::menustr);
#ifdef WEAROS
static_assert(maxmenu==2);
#else
static_assert(maxmenu==4);
#endif
int getmenu(int tapx) {
	return tapx*maxmenu/dwidth;
	}
#ifndef WEAROS
static bool speakmenutap(float x,float y) {
	if(x<menupos.left||x>=menupos.right) {
		return false;
		}
	float dist=(menupos.bottom-menupos.top)/nrmenu;
	int item=(y-menupos.top)/dist;
	if(item>=0&&item<nrmenu) {
		LOGGER("menuitem %d\n",item);
		auto options=optionsmenu[selmenu];
		auto label=usedtext->menustr[selmenu][item];
		if(!options||!options[item]) 
			speak(label.data());
		else {
			constexpr const int maxbuf=256;
			char buf[maxbuf];
			memcpy(buf,label.data(),label.size());
			char *ptr=buf+label.size();
			*ptr++= '\n';
			if(*options[item])
				strcpy(ptr,usedtext->checked);
			else
				strcpy(ptr,usedtext->unchecked);
			speak(buf);
			}
		return true;
		}
	return false;	
	}
#endif
static const float  getsetlen(NVGcontext* genVG,float x, float  y,const char * set,const char *setend,bounds_t &bounds) {
	 	nvgTextBounds(genVG, x,  y, set,setend, bounds.array);
		return bounds.xmax-bounds.xmin;
		}
static void showtext(NVGcontext* genVG ,time_t nu,int menu) {
LOGAR("showtext");
#ifdef WEAROS
	if(menu==1) {
		setnowmenu(nu);
//		setnewamount();
		}
#else
	if(menu==3)
		setnowmenu(nu);
	if(menu==1)
		setnewamount();
#endif
	string_view *menuitem=usedtext->menustr[menu];
	nrmenu=getmenulen(menu);
	constexpr const float randsize=
	#ifdef WEAROS
	10
	#else
	16
	#endif
	;
	 float xrand=randsize*density;
	 float yrand=randsize*density;
//	float menutextheight=density*48;
	float menuplace= dwidth/ maxmenu;
	float x=xrand+menu*menuplace,starty=yrand,y=starty;

	nvgTextAlign(genVG,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);

	bounds_t bounds;

	nvgFontFaceId(genVG,menufont);
	nvgFontSize(genVG, menusize);
	 nvgTextBounds(genVG, x,  y, menuitem[0].data(),menuitem[0].data()+menuitem[0].size(), bounds.array);
//	nvgText(genVG, x,y, menuitem[0].data(), menuitem[0].data()+menuitem[0].size());
	 float maxx=bounds.xmax;
	 float maxwidth=bounds.xmax-bounds.xmin;
	 for(int i=1;i<nrmenu;i++) {
		y+=menutextheight;
	 	nvgTextBounds(genVG, x,  y, menuitem[i].data(),menuitem[i].data()+menuitem[i].size(), bounds.array);
	 	if(maxx<bounds.xmax)
			maxx=bounds.xmax;
		 float maxwidthone=bounds.xmax-bounds.xmin;
		 if(maxwidthone>maxwidth)
		 	maxwidth=maxwidthone;
		}
	float height=y+bounds.ymax-bounds.ymin;
	nvgBeginPath(genVG);
	 nvgFillColor(genVG, *getmenucolor());
//	 nvgFillColor(genVG, white);
	 float mwidth=maxx-x+2*xrand;
//	 float minmenu=128*density;
	 float minmenu=
#ifdef WEAROS
	 80
#else
	128
#endif

	 		*density;
	 float maxmenu=280*density;
	 if(mwidth<minmenu)
	 	mwidth=minmenu;
	else
		if(mwidth>maxmenu)
			mwidth=maxmenu;
	 x+=(menuplace-mwidth)/2;
	 #ifdef WEAROS
	 if(menu==0)
		 x+=xrand;
	 #endif
	 menupos={ x-xrand, starty-yrand,x-xrand+ mwidth, height+yrand};
	 nvgRect(genVG, x-xrand, starty-yrand, mwidth, height-starty+2*yrand);
	nvgFill(genVG);
#ifdef WEAROS
if(menu==0) {
	nvgTextAlign(genVG,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	x+=maxwidth;
	}
#endif



	y=starty;
//	 nvgFillColor(genVG, *getwhite());
	 nvgFillColor(genVG, *getmenuforegroundcolor());
//	 nvgFillColor(genVG, black);
	 for(int i=0;i<nrmenu;i++) {
		nvgText(genVG, x,y, menuitem[i].data(), menuitem[i].data()+menuitem[i].size());
		y+=menutextheight;
		}

	if(const int **options=optionsmenu[menu]) {
		y=starty;
		const char set[]="[x]";
		const char unset[]="[ ]";
		constexpr int len=3;
		float xpos;
#ifdef WEAROS
		if(menu==0) {
			xpos=x;
			}
		else 
#endif
		{
		static const float  dlen=getsetlen(genVG, x,  y, set,set+len, bounds);
		 xpos=x-2*xrand+mwidth-dlen;
		 }
		 for(int i=0;i<nrmenu;i++) {
		 	if(const int *optr=options[i]) {
				const char *op=*optr?set:unset;
				nvgText(genVG, xpos ,y,op ,op+len );
				}
			y+=menutextheight;
			}
		}

	LOGAR("end showtext");
	}


extern bool makepercetages() ;
//alignas(sizeof(char *)) char buffer[1024];
//#include <thread>
/*
extern		void			testbackup();
void testbackup() {
 	int ind=backup->newupdate("192.168.1.71","12345");
	if(ind<0) {
	     LOGSTRING("no room for other host\n");
	     return;
	     }
//	     int ind=0;
	   backup->update(ind);
	}
	*/
extern bool setbluetoothon;
extern void setbuffer(char *);


extern Backup *backup;
inline jlong menuel(int menu,int item) {
	return menu+item*0x10LL;
	}
#ifdef WEAROS
static int64_t doehier(int menu,int item) {
	switch(menu) {
		case 0: 
			switch(item) {
				case 0 :  nrmenu=0;return 1LL*0x10+1;				
				case 1 : nrmenu=0;return 3LL*0x10;
				extern void setinvertcolors(bool val) ;
				case 2: invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return -1LL;
					break;
				case 3: nrmenu=0; return 4LL*0x10;
				case 4: nrmenu=0; return menuel(0,7);

				};break;
		case 1: 
			nrmenu=0;
			switch(item) {
				case 0: return 2LL*0x10+3;
//				case 1: return 1LL*0x10+3;
				case 1: {
					auto max=time(nullptr);
					setstarttime(max-duration*3/5);
					return -1LL;
					}
				case 2: prevdays(1); return -1LL;
				case 3: 
//					if(settings->staticnum()) return -1LL;
					return  menuel(1,2);
				};
		}
	return -1LL;
	}
#else
static int64_t doehier(int menu,int item) {
	switch(menu) {
		case 0: 
			switch(item) {
				case 0: 
					showui=!showui;
					settings->setui(showui);
					break;
				default:
					nrmenu=0;
					break;
				};break;
		case 1: switch(item) {
//				case 0: notify=!notify;return menu|item*0x10|(notify<<8);
				case 2: nrmenu=0;
//					if(settings->staticnum()) return -1LL;
					break;
				case 3:	nrmenu=0; 
				   if(!numlist) {
				      void numiterinit();
				      numiterinit();
				      numlist=1;
				      }
					break;

#ifdef PERCENTILES
				case 4:  nrmenu=0; if(!makepercetages())
						return -1LL;
					;break;
#endif
				case 6:break;
				default:
					nrmenu=0;
					break;
				};break;
		case 2:
			switch(item) 	{
				case 0: {
					nrmenu=0;
					int lastsensor=sensors->lastscanned();
					if(lastsensor>=0) {
						const SensorGlucoseData *hist=sensors->getSensorData(lastsensor);
						if(hist) {
							const ScanData *scan= hist->lastscan();
							const uint32_t nu= time(nullptr);
							if(scan&&scan->valid()&&((nu-scan->t)<(60*60*5)))
								scantoshow={lastsensor,scan,nu};
							}
						}
					}; return -1ll;
				case 1:showscans=!showscans;return -1ll;
				case 2:showstream=!showstream;return -1ll;
				case 3:showhistories=!showhistories;return -1ll;
				case 4: shownumbers=!shownumbers;return -1ll;
				case 5: showmeals=!showmeals;return -1ll;
				extern void setinvertcolors(bool val) ;

				case 6: invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return -1ll;
				};break;
		case 3: {
		nrmenu=0;
		switch(item) {
			case 0: {
			auto max=time(nullptr);
		//	starttime=starttimefromtime(max);
//			if((starttime+duration)<max) 
			setstarttime(max-duration*3/5);
			return -1;
				};
			case 3: prevdays(1); return -1;
			case 4: nextdays(1);return -1;
			case 5: prevdays(7); return -1;
			case 6: nextdays(7);return -1;		
			default: break;
			};
			};break;

		default: nrmenu=0;
		}
	return menu+item*0x10;
	}
#endif

static bool  inmenu(float x,float y) {
	if(!nrmenu)
		return false;
	if(x<menupos.left||x>=menupos.right) {
		return false;
		}
	float dist=(menupos.bottom-menupos.top)/nrmenu;
	if(dist<=0)
		return false;
	int item=(y-menupos.top)/dist;
	if(item>=0&&item<nrmenu) 
		return true;
	return false;
	}

static int64_t menutap(float x,float y) {
	if(x<menupos.left||x>=menupos.right) {
		nrmenu=0;
		return -1LL;
		}
	float dist=(menupos.bottom-menupos.top)/nrmenu;
	int item=(y-menupos.top)/dist;
	if(item>=0&&item<nrmenu) {

		LOGGER("menuitem %d\n",item);
	//	return doehier(getmenu(x),item);
		return doehier(selmenu,item);
		}
	nrmenu=0;
	return -1LL;	
	}





#ifndef WEAROS
void settoend() ;
void shownumiters() ;
#include "oldest.h"
bool numpagepast() {
	if(!numiters)
		return false;
	if(numdatas.size()<basecount)
		return false;
	bool onstart=true;
	for(int i=0;i<basecount;i++) {
		if(getpageoldest(i)>numiters[i].begin) {
			onstart=false;
			}
		}
	if(!onstart) {
		for(int i=0;i<basecount;i++) {
			setpagenewest(i,getpageoldest(i));
			setpageoldest(i,nullptr);
			}
		}
	return onstart;
//	shownumfromtop();
}

extern int nrcolumns;
int nrcolumns=1;
int numfrompos(const float x,const float y) {
	int rows=dheight/textheight;

	int ind= ((nrcolumns!=1&&x>(dleft+dwidth/2))?rows:0)+ (y-dtop)/textheight;
	LOGGER("rows=%d, ind=%d\n",rows,ind);
	int i=0,index;
	for(int i=0;i<basecount;i++) {
		numiters[i].iter=getpageoldest(i);
		}
	do {
		index=ifindoldest(numiters,0,basecount,notvali);
		} while(i++<ind);
	return index;
	}
bool numpageforward() {
	if(!numiters)
		return false;
	if(numdatas.size()<basecount)
		return false;
	LOGSTRING("Page forward\n");
	bool noend=false;
	for(int i=0;i<basecount;i++) {
	    const Num*newst=getpagenewest(i);
	    if(newst<=numiters[i].end) {
	    	noend=true;
		}
	   }
	if(noend) {
		for(int i=0;i<basecount;i++) {
		    const Num*newst=getpagenewest(i);
		    setpageoldest(i,newst);
		    }
		  }
	return !noend;
	}
//s/numiters.i..pageoldest.\([^;]*\);/setpageoldest(i,\1);/g
void  setitertobottom(NumIter<Num> *numiters, const int nr) {
	for(int i=0;i<nr;i++) {
		setpageoldest(i,numiters[i].next());
		}
	}

int numsize() {
	int basecount=numdatas.size();
	int tot=0;
	for(int i=0;i<basecount;i++)
		tot+=numdatas[i]->size();
	return tot;
}
void showfromend() {
	settoend() ;
	shownumsback(genVG, numiters,basecount);
       setitertobottom(numiters,basecount);
	}
void showfromstart() {
	if(numlist==4) {
		numlist=1;
		}
	else {
		for(int i=0;i<basecount;i++) {
			numiters[i].iter=getpageoldest(i);
			}
		}
	 shownums(genVG, numiters, basecount);
	for(int i=0;i<basecount;i++) {
		setpagenewest(i,numiters[i].iter);
		}
	}

void numpagenum(const uint32_t tim) {
	int tot=0;
	for(int i=0;i<basecount;i++)  {
		const Num *ptr=numdatas[i]->firstnotless(tim) ;
		if(ptr==numdatas[i]->end()||ptr->gettime()>tim)
			ptr--;
//		int pos=ptr-numdatas[i]->begin()+1;
		int pos=ptr-numdatas[i]->begin();
		LOGGER("pos=%d\n",pos);
		if(pos>0) {
			LOGGER("num %.1f %s\n",ptr->value, settings->getlabel(ptr->type).data());
			tot+=pos;
			}
		numiters[i].iter=ptr;
		}
	const int percol=dheight/textheight;
	const int onpage=nrcolumns*percol;
	#ifndef NOLOG
	time_t tims=tim;
	LOGGER("nrcolumns=%d percol=%d onpage=%d %s",nrcolumns,percol,onpage,ctime(&tims));
	#endif
	for(int tever=tot%onpage
#ifndef NOLOG
	, niets=LOGGER("tever=%d\n",tever)
#endif
	;tever>0;--tever) {
		ifindnewest(numiters,basecount,notvali);
		};
	int newest;
	for(newest=0;newest<basecount&& numiters[newest].iter>numiters[newest].end;newest++)
			;
	for(int i=newest+1;i<basecount;i++) {
		if(numiters[i].iter<=numiters[i].end&&numiters[i].iter->gettime()>numiters[newest].iter->gettime())
			newest=i;
		}
	for(int i=0;i<basecount;i++)
		if(i!=newest&&numiters[i].iter<=numiters[i].end)
			numiters[i].inc();
	for(int i=0;i<basecount;i++) 
		setpageoldest(i,numiters[i].iter);
	numlist=4;
	}
void shownumlist() {
	startstep(*getwhite());
	if(getpageoldest(0)!=nullptr) {
		showfromstart();
		 }
	else {	
		showfromend();
		}
	}
NumIter<Num> *mknumiters() ;


extern int getcolumns(int width);
void numiterinit() {
	nrcolumns=getcolumns(round(3.4*smallsize));
	LOGAR("numiterinit");
	basecount=numdatas.size();
	delete[] numiters;
	numiters=mknumiters() ;
	for(int i=0;i<basecount;i++) {
		setpagenewest(i, numdatas[i]->end());
		setpageoldest(i, nullptr);
		}
	numpagenum(starttime+duration/2);
	}

void numendbegin() {
	for(int i=0;i<basecount;i++) {
		setpagenewest(i, numdatas[i]->end());
	//	setpagenewest(i,numiters[i].next(numiters[i].end));
		}
	}
void settoend() {
//static bool init=true; if(init) numiterinit() ; init=false;
	for(int i=0;i<basecount;i++) {
		numiters[i].iter=getpagenewest(i)-1;
		}
	}
int onlast(int onscreen) {
	return numsize()%onscreen;
	}
//template <class T> int ifindnewest(NumIter<T> *nums,int count) ;
//extern template int ifindnewest<Num>(NumIter<Num> *,int );
/*
int numfrompos(float x) {
	settoend() ;
	float textheight=density*48;
	int ind=(dwidth -(x-dtop))/textheight;
	int i=0,index;
	do {
		index=ifindnewest(numiters,basecount);
		} while(i++<ind);
	return index;
	}
	*/
void shownumiters() {
	LOGSTRING("Iters:\n");
	for(int i=0;i<basecount;i++) {
		lognum(numiters[i].iter);
		}
	LOGSTRING("\n");
	}

void numfirstpage() {
	for(int i=0;i<basecount;i++)
		setpageoldest(i,numiters[i].begin);
	}


void endnumlist() {
	numlist=0;

	uint32_t first=UINT32_MAX,second=0;
	for(const NumDisplay *num:numdatas) {
		const Num *one=std::max(num->begin(),num->extrum.first);		
		const Num *two=std::min(num->end(),num->extrum.second);		//NODIG?
		two--;
		while(one<=two) {
			if(!num->valid(one))  {
				one++;
				continue;
				}
			if(!num->valid(two))  {
				two--;
				continue;
				}
			if(one->gettime()<first)
				first=one->gettime();
			if(two->gettime()>second)
				second=two->gettime();
			break;
			}
		}
#ifndef NDEBUG
time_t newstart=first,start=starttime;
char buf[80];
// char *ctime_r(const time_t *timep, char *buf);

	LOGGER("endnumlist %ud %ud start=%s starttime=%ud %s",first,second,ctime(&newstart),starttime,ctime_r(&start,buf));

#endif
	if(first==UINT32_MAX)
		return;
	if((starttime+duration)>=first&&starttime<second)
		return;
	setstarttime(starttimefromtime((first+second)/2));
	return;
	}
#endif
//#include <stdio.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>


int64_t openNums(std::string_view numpath,int64_t ident) {
	 NumDisplay* numdata=NumDisplay::getnumdisplay( numpath,ident,nummmaplen);
	 if(numdata) {
		numdatas.push_back(numdata);
		if(ident==0LL)
			newhit.numdisplay=numdata;
		
		}
	
	LOGGER("numdir=%s ptr=%p\n",numpath.data(),numdata);
	return reinterpret_cast<int64_t>(numdata);
	}

