#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace hp {
// Differential solar charging: instantaneous safety limits, slow speed changes.
struct SolarController {
  bool running=false, tank_hold=false;
  float level=0;
  uint32_t adjusted_at=0, stopped_at=0, ready_at=0;
  bool stopped_once=false, qualifying=false;
  const char *reason="Waiting for solar heat";
  float stop(uint32_t now,const char *why) {
    if(running){stopped_at=now;stopped_once=true;}
    running=false;qualifying=false;level=0;reason=why;return 0;
  }
  float tick(float collector,float tank,float setpoint,bool collector_valid,bool tank_valid,uint32_t now) {
    const bool known_tank=tank_valid&&std::isfinite(tank);
    const bool known_setpoint=std::isfinite(setpoint);
    if(known_tank&&known_setpoint){
      if(tank>=setpoint)tank_hold=true;
      else if(tank<=setpoint-2)tank_hold=false;
    }
    if(!collector_valid||!std::isfinite(collector)||!known_tank||!known_setpoint)
      return stop(now,"Waiting for valid temperatures");
    if(collector>=120)return stop(now,"Collector temperature limit");
    if(tank_hold)return stop(now,"Tank target reached");
    const float delta=collector-tank;
    if(running&&delta<=2)return stop(now,"Waiting for solar heat");
    if(!running){
      if(stopped_once&&uint32_t(now-stopped_at)<60000){qualifying=false;reason="Solar restart pause";return 0;}
      if(delta<8){qualifying=false;reason="Waiting for solar heat";return 0;}
      if(!qualifying){qualifying=true;ready_at=now;}
      if(uint32_t(now-ready_at)<20000){reason="Confirming available solar heat";return 0;}
      running=true;qualifying=false;level=.30f;adjusted_at=now;reason="Solar charging";return level;
    }
    reason="Solar charging";
    // Change at most once per collector sample interval; never catch up after a stall.
    if(uint32_t(now-adjusted_at)<10000)return level;
    adjusted_at=now;
    const float error=delta-6.0f;
    if(std::fabs(error)>.5f){
      const float change=std::max(-.03f,std::min(.02f,error*.01f));
      level=std::max(.30f,std::min(1.0f,level+change));
    }
    return level;
  }
};
}
