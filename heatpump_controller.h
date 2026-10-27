#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
namespace hp {
enum class Fault {None,Overheat,Freeze,NoResponse,Flow,Sensor,HotLimit,Lockout};
enum class Phase {Idle,RestartWait,ProvingFlow,Heating,Overrun,Fault};
struct Sample {
 float value=NAN; uint32_t at=0, sequence=0;
 void update(float v,uint32_t now){value=v;at=now;++sequence;}
 bool valid(uint32_t now,uint32_t max_age,float low,float high) const {
  return sequence && std::isfinite(value) && value>=low && value<=high && uint32_t(now-at)<=max_age;
 }
};
struct Controller {
 Sample hot_out,hot_in,cold_out,cold_in,motor,tank,flow_hot,flow_cold;
 bool compressor=false,pumps=false,response_confirmed=false;
 unsigned failures=0; Fault fault=Fault::None; Phase phase=Phase::Idle;
 float min_flow=1, response_delta=0.5f, freeze_stop=-20, freeze_clear=-17;
 float tank_limit=75, outlet_limit=80;
 uint32_t response_timeout_ms=60000;
 uint32_t restart_ms=300000, overrun_ms=30000, proof_timeout_ms=45000;
 uint32_t stopped_at=0,started_at=0,proof_at=0,flow_bad_at=0,transfer_bad_at=0,recovery_at=0;
 uint32_t proof_hot_seq=0,proof_cold_seq=0;
 uint32_t proof_hot_out_seq=0,proof_hot_in_seq=0,proof_cold_out_seq=0,proof_cold_in_seq=0;
 bool flow_bad=false,transfer_bad=false,recovering=false;
 bool overheat_hold=false,freeze_hold=false,hot_hold=false;
 float base_hot=0,base_cold=0,avg_hot=NAN,avg_cold=NAN;
 uint32_t avg_hot_seq=0,avg_cold_seq=0;
 void begin(uint32_t now){compressor=false;pumps=false;stopped_at=now;phase=Phase::RestartWait;fault=failures>=3?Fault::Lockout:Fault::None;}
 bool sensors_ok(uint32_t now) const {
  return hot_out.valid(now,25000,-40,125)&&hot_in.valid(now,25000,-40,125)
   &&cold_out.valid(now,25000,-40,125)&&cold_in.valid(now,25000,-40,125)
   &&motor.valid(now,25000,-40,125)&&tank.valid(now,70000,-40,125)
   &&flow_hot.valid(now,7000,0,200)&&flow_cold.valid(now,7000,0,200);
 }
 bool flow_ok() const {return flow_hot.value>=min_flow&&flow_cold.value>=min_flow;}
 Fault physical_fault(uint32_t now) const {
  if(!sensors_ok(now))return Fault::Sensor;
  if(motor.value>=99 || (overheat_hold && motor.value>=90))return Fault::Overheat;
  if(cold_out.value<=freeze_stop || (freeze_hold && cold_out.value<=freeze_clear))return Fault::Freeze;
  if(tank.value>=tank_limit||hot_out.value>=outlet_limit
    || (hot_hold && (tank.value>=tank_limit-5 || hot_out.value>=outlet_limit-5)))return Fault::HotLimit;
  return Fault::None;
 }
 void stop(uint32_t now){if(compressor){compressor=false;stopped_at=now;}flow_bad=false;transfer_bad=false;}
 void trip(Fault f,uint32_t now){
  if(f==Fault::Overheat)overheat_hold=true;
  if(f==Fault::Freeze)freeze_hold=true;
  if(f==Fault::HotLimit)hot_hold=true;
  bool was_running=compressor, was_proving=phase==Phase::ProvingFlow;
  stop(now);if(!was_running && was_proving)stopped_at=now;fault=f;recovering=false;if(f==Fault::NoResponse||f==Fault::Flow)++failures;phase=Phase::Fault;}
 void smooth(Sample &s,float &avg,uint32_t &seq){
  if(s.sequence==seq)return;
  seq=s.sequence;
  if(!std::isfinite(s.value)){avg=NAN;return;}
  avg=std::isfinite(avg)?0.7f*avg+0.3f*s.value:s.value;
 }
 bool reset(bool demand_off,uint32_t now){
  if(!demand_off||compressor||physical_fault(now)!=Fault::None)return false;
  failures=0;fault=Fault::None;recovering=false;
  overheat_hold=false;freeze_hold=false;hot_hold=false;return true;
 }
 uint32_t remaining(uint32_t now)const{uint32_t e=now-stopped_at;return e>=restart_ms?0:(restart_ms-e+999)/1000;}
 bool heat_response()const{
  float dh=hot_out.value-hot_in.value, dc=cold_in.value-cold_out.value;
  return (dh>=response_delta&&dh-base_hot>=response_delta)
      || (dc>=response_delta&&dc-base_cold>=response_delta);
 }
 void tick(uint32_t now,bool demand){
  smooth(flow_hot,avg_hot,avg_hot_seq);smooth(flow_cold,avg_cold,avg_cold_seq);
  // Latch EACH known limit independently: an unrelated invalid sensor must
  // not mask an observed dangerous temperature.
  if(motor.valid(now,25000,-40,125) && motor.value>=99)overheat_hold=true;
  if(cold_out.valid(now,25000,-40,125) && cold_out.value<=freeze_stop)freeze_hold=true;
  if((tank.valid(now,70000,-40,125) && tank.value>=tank_limit)
    || (hot_out.valid(now,25000,-40,125) && hot_out.value>=outlet_limit))hot_hold=true;
  // A physical limit always wins, including during recovery and pump proof.
  Fault physical=physical_fault(now);
  if(sensors_ok(now)){
   if(motor.value<90)overheat_hold=false;
   if(cold_out.value>freeze_clear)freeze_hold=false;
   if(tank.value<tank_limit-5 && hot_out.value<outlet_limit-5)hot_hold=false;
  }
  if(physical!=Fault::None && (compressor||phase==Phase::ProvingFlow||demand)){
   if(fault!=physical)trip(physical,now);
  }
  if(fault!=Fault::None){
   if(failures>=3){fault=Fault::Lockout;}
   else if(physical==Fault::None){
    bool ready=true;
    if(fault==Fault::Overheat)ready=motor.value<90;
    if(fault==Fault::Freeze)ready=cold_out.value>freeze_clear;
    if(fault==Fault::HotLimit)ready=tank.value<tank_limit-5 && hot_out.value<outlet_limit-5;
    if(fault==Fault::Sensor){
     if(!recovering){recovering=true;recovery_at=now;}
     ready=uint32_t(now-recovery_at)>=30000;
    }
    // Flow and no-response faults recover into a NEW pump proof, never
    // require flow while pumps are stopped. Failure budget limits retries.
    if(ready){fault=Fault::None;recovering=false;}
   }else recovering=false;
   if(fault!=Fault::None){
    phase=Phase::Fault;
    pumps=pumps&&uint32_t(now-stopped_at)<overrun_ms;
    return;
   }
  }
  if(!demand){
   bool was_running=compressor;stop(now);
   if(phase==Phase::ProvingFlow&&!was_running)pumps=false;
   pumps=pumps&&uint32_t(now-stopped_at)<overrun_ms;
   phase=pumps?Phase::Overrun:Phase::Idle;return;
  }
  if(compressor){
   phase=Phase::Heating;pumps=true;
   if(!flow_ok()){
    if(!flow_bad){flow_bad=true;flow_bad_at=now;}
    if(uint32_t(now-flow_bad_at)>=3000){trip(Fault::Flow,now);return;}
   }else flow_bad=false;
   uint32_t run=now-started_at;
   if(!response_confirmed&&run>=response_timeout_ms){
    if(!heat_response()){trip(Fault::NoResponse,now);return;}
    response_confirmed=true;
   }
   if(response_confirmed){
    // Apply the same baseline compensation as startup: a persistent sensor
    // offset or pre-existing gradient alone is not evidence of heat transfer.
    bool transfer=heat_response();
    if(!transfer){
     if(!transfer_bad){transfer_bad=true;transfer_bad_at=now;}
     if(uint32_t(now-transfer_bad_at)>=120000){trip(Fault::NoResponse,now);return;}
    }else transfer_bad=false;
    if(run>=600000&&!transfer_bad)failures=0;
   }
   return;
  }
  if(remaining(now)){
   pumps=pumps&&uint32_t(now-stopped_at)<overrun_ms;
   phase=pumps?Phase::Overrun:Phase::RestartWait;return;
  }
  if(phase!=Phase::ProvingFlow){
   phase=Phase::ProvingFlow;pumps=true;proof_at=now;
   proof_hot_seq=flow_hot.sequence;proof_cold_seq=flow_cold.sequence;
   proof_hot_out_seq=hot_out.sequence;proof_hot_in_seq=hot_in.sequence;
   proof_cold_out_seq=cold_out.sequence;proof_cold_in_seq=cold_in.sequence;
  }
  if(uint32_t(now-proof_at)>=proof_timeout_ms){trip(Fault::Flow,now);pumps=false;return;}
  if(!flow_ok()){
   proof_hot_seq=flow_hot.sequence;proof_cold_seq=flow_cold.sequence;return;
  }
  // Two NEW measurements on EACH side; pre-start cached flow cannot pass.
  if(uint32_t(flow_hot.sequence-proof_hot_seq)<2||uint32_t(flow_cold.sequence-proof_cold_seq)<2)return;
  // Capture the baseline only after EACH loop probe has published with pumps
  // enabled. Cached stagnant-water temperatures can hide a real start response.
  if(hot_out.sequence==proof_hot_out_seq||hot_in.sequence==proof_hot_in_seq
    ||cold_out.sequence==proof_cold_out_seq||cold_in.sequence==proof_cold_in_seq)return;
  if(physical_fault(now)!=Fault::None)return;
  compressor=true;pumps=true;started_at=now;phase=Phase::Heating;
  base_hot=hot_out.value-hot_in.value;base_cold=cold_in.value-cold_out.value;
  response_confirmed=false;flow_bad=false;transfer_bad=false;
 }
 const char* fault_text()const{
  switch(fault){case Fault::None:return "None";case Fault::Overheat:return "Compressor reached 99 C cutoff; cooling below 90 C";
   case Fault::Freeze:return "Cold outlet too cold; waiting for recovery";case Fault::NoResponse:return "Heat transfer not detected";
   case Fault::Flow:return "Insufficient hot or cold flow";case Fault::Sensor:return "Invalid or stale critical sensor";
   case Fault::HotLimit:return "Tank or hot outlet temperature limit";case Fault::Lockout:return "Repeated flow/start failures; turn heating OFF and reset";}
  return "Unknown";
 }
 const char* phase_text()const{
  switch(phase){case Phase::Idle:return "Idle";case Phase::RestartWait:return "Waiting for five-minute restart delay";
   case Phase::ProvingFlow:return "Pumps running; proving flow";case Phase::Heating:return response_confirmed?"Heating":"Heating; checking startup heat transfer";
   case Phase::Overrun:return "Pump overrun";case Phase::Fault:return fault_text();}return "Unknown";
 }
};
}
