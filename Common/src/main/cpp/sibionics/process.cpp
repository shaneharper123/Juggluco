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
/*      Thu Apr 04 20:15:06 CEST 2024                                                 */


#ifdef SIBIONICS
#include "config.h"
#include <numeric>
#include <bit>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "sibionics/SiContext.hpp"
#include "SensorGlucoseData.h"
#include "settings/settings.h"
#include "sensoren.h"
#include "datbackup.h"
#include "EverSense.hpp"
extern Sensoren *sensors;
#ifndef LOGGER
#define LOGGER(...) printf(__VA_ARGS__)
#endif
static int sitrend2abbott(int sitrend) {
   if(sitrend<-2)  {
      if(sitrend==-3)
            return 1;
       return 0;
      }
   if(sitrend>2)  {
      if(sitrend==3)
            return 1;
         
       return 0;
      }
   return sitrend+3; 
   }
static float sitrend2RateOfChange(int sitrend) {
   return sitrend*1.3f;
   }
static uint32_t makestarttime(int index,uint32_t eventTime) {  
   const uint32_t starttime=eventTime-index*60;
#ifndef NOLOG
   time_t tim=starttime;
   LOGGER("makestarttime(%d,%d)=%d %s",index,eventTime,starttime,ctime(&tim));
#endif
	return starttime;
   }
#ifdef MAIN
#define savejson(sens,name,  index,alg,getjson )x
#define glucoseback(glval, drate,hist) 
#else
#include "sibionics/json.h"
extern bool savejson(SensorGlucoseData *sens,std::string_view, int index,const AlgorithmContext *alg,getjson_t getjson );
extern getjson_t getjson;
extern jlong glucoseback(uint32_t glval,float drate,SensorGlucoseData *hist) ;
#ifndef NOLOG
void logbytes(std::string_view text,const uint8_t *value,int vallen) {
		int totlen=text.size()+2+vallen*3+1;
		char mess[totlen];
      memcpy(mess,text.data(),text.size());
      char *ptr=mess+text.size();
      *ptr++=':';
      for(int i=0;i<vallen;i++) {
         ptr+=sprintf(ptr," %02X",value[i]);
         }
		LOGGERN(mess,ptr-mess);
      }
#else
#define logbytes(text,value, vallen) 
#endif
#endif
#ifdef SIHISTORY
extern getjson_t getjson3;
void	 SiContext::saveSi3(SensorGlucoseData *sens,int index,uint32_t eventTime,bool save,double value,float temp,bool dosavejson) {
     if(const double newvalue=process3(index,value,temp);newvalue>0.1) {
            const int mgL=std::round(newvalue* convfactor);
         if(save) {
            const int idpos=int(round(index/(double)sens->getmininterval()));
            Glucose *item=sens->getglucose(idpos);
            item->time=eventTime;
            item->id=index;
            item->glu[1]=mgL;
            const int newend=idpos+1;
            if(newend>sens->getScanendhistory()) sens->setendhistory(newend);
            if(dosavejson||!(index%256)) {
               savejson(sens,sens->statefile3,index,algcontext3,getjson3);
               }
            }

         }
	}
#else
#define saveSi3(sens, index,eventTime, save, value, temp, last)

#endif
jlong SiContext::processData(SensorGlucoseData *sens,time_t nowsecs,int8_t *data,int totlen,int sensorindex) {
	logbytes("SIprocess",(uint8_t*)data,totlen);
   if(data[2] != 9||data[0] != -86 || data[1] != 85) {
      LOGGER("wrong start %d %d %d\n",data[0],data[1],data[2]);
      return 2LL;
        }
{
  const int len=totlen-1; 
   const long sum=std::accumulate(data,data+len, 0L) ;
  if(((int8_t) ((~(sum & 0xFF)) + 1)) != data[len]) {
      LOGGER("wrong sum %ld %d\n",sum,data[len]);
      return 2LL;
        }
}
   sensor *sensor=sensors->getsensor(sensorindex);
   const int multiple=data[3];
   const int maxoff=multiple*14;
   int8_t * const start=data+4;
   for(int off=0;off < maxoff;off+=14) {
      const uint16_t *one=reinterpret_cast<uint16_t*>(start+off);
      const int index=std::byteswap(one[0]);
      const double temp = std::byteswap(one[1])/ 10.0f;
      const double value = std::byteswap(one[3])/10.0;
      const int numOfUnreceived=std::byteswap(one[5]);

       const int maxid=sens->getSiIndex();
       if(index!=maxid)   {
            if(index<maxid)   {
               LOGGER("SIprocess index=%d<maxid=%d\n",index,maxid);
               return 3LL;
               }
            else {
               LOGGER("SIprocess index=%d>maxid=%d\n",index,maxid);
               int maxretry=(index-maxid)<20?2:((index-maxid)<200?5:10);
               if(sens->retried++<maxretry) {
                  return 3LL; 
                  }
               }
               }
       const int addtime=std::byteswap(one[6]);
         long  offtime=addtime  - (numOfUnreceived * 60) ;
         const bool infuture=offtime>0;
         LOGGER("Siprocess: addtime=%d added=%ld\n",addtime,offtime);
         time_t eventTime=nowsecs + offtime;
         if(infuture) {
            LOGGER("Siprocess: wrong time: %ld seconds future addtime=%d index=%d temp=%f value=%f numOfUnreceived=%d\n",offtime,addtime,index,temp, value,numOfUnreceived);
            eventTime=nowsecs;
               }
           else {
             if(maxid<10) {
                  auto starttime=makestarttime(index,eventTime);
                  sens->getinfo()->starttime=starttime;
                   sensor->starttime=starttime;
                  }
		   }
	   double newvalue=0.0;
            if((newvalue=process(index,value,temp))>1.8) {
	    	sens->getinfo()->pollinterval=newvalue-value;
	    }
	   else {
	   	if(sens->getinfo()->pollinterval<40)
		   newvalue=value+sens->getinfo()->pollinterval;
	   	}
      #ifndef NOLOG
               const long electric= std::byteswap(one[2]);
               const int status = std::byteswap(one[4]);
      #endif
               const int mgdL=std::round(newvalue*convfactordL);
               const int trend=algcontext->ig_trend;
               const float change=sitrend2RateOfChange(trend);
               const int abbotttrend=sitrend2abbott(trend);
           LOGGER("SIprocess: index=%d temp=%f electric=%ld value=%f->%f status=%d numOfUnreceived=%d addtime=%d trend=%d rate=%.2f abbotttrend=%d\n", index, temp, electric, value, mgdL/convfactordL,status, numOfUnreceived, addtime,trend,change,abbotttrend);
                 
//         	if(infuture) sens->setSiIndex(index+1);
	   if(newvalue>1.8&&newvalue<30) {
           sens->savestream(eventTime,index,mgdL,abbotttrend,change);
           sens->retried=0;
	        saveSi3(sens,index,eventTime,!infuture,value,temp,!numOfUnreceived);
           if(!numOfUnreceived)  {
                 sens->sensorerror=false;
                 if(sensor->finished) {
                        sensor->finished=0;
                        LOGGER("SIprocess finished=%d\n", sensor->finished);
                        backup->resensordata(sensorindex);
                        }
                   auto res=glucoseback(mgdL,change,sens);
                   if(!(index%5)) savejson(sens,sens->statefile,index,algcontext,getjson);
                    backup->wakebackup(Backup::wakestream);
                  extern void wakewithcurrent();
                     wakewithcurrent();

#ifdef OLDEVERSENSE 
                     if(sens->broadcastfrom<INT_MAX) {
                        sendEverSenseold(sens,sens->broadcastfrom,sens->pollcount()-1);
                        sens->broadcastfrom=INT_MAX;
                        }
#endif
                     return res;
                    }
                 else {
                   if(!infuture&&!(index%500)) {
                        savejson(sens,sens->statefile,index,algcontext,getjson);
                        backup->wakebackup(Backup::wakestream);
                        }
                      sens->receivehistory=nowsecs;
                     }
#ifdef OLDEVERSENSE 
                  const int last=sens->pollcount()-1;
                  if(last<sens->broadcastfrom) sens->broadcastfrom=last;
#endif

               }
           else {
            if(index==maxid)
	            sens->setSiIndex(maxid+1);
            LOGGER("SIprocess failed: index=%d temp=%f value=%f numOfUnreceived=%d\n", index, temp, value,numOfUnreceived);
            if(!numOfUnreceived&&!(index%5))  {
               sens->sensorerror=true;
               return 0LL;
               }
            }
      	}
     return 1LL;
    }
/*
int main() {
#include "init.cpp"
for(auto &el:data) 
      processData((int8_t*)el.data(),el.size());
   } */
#endif
