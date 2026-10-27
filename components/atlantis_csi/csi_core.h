#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <atomic>
namespace atlantis {
struct Sample { uint32_t seq{}, timestamp_us{}; uint16_t len{}; uint8_t rate{8},mcs{},sgi{},aggregation{}; uint8_t source[6]{},bssid[6]{},channel{},sig_mode{},cwb{},stbc{},secondary_channel{}; int8_t rssi{},raw[128]{}; bool first_word_invalid{}; };
inline bool destination_allowed(uint32_t d,uint32_t ip,uint32_t mask) {bool priv=(d>>24)==10 || (d>>20)==0xac1 || (d>>16)==0xc0a8; return priv && mask && d!=ip && (d&mask)==(ip&mask) && (d&~mask)!=0 && (d&~mask)!=~mask;}
inline bool supported(const Sample &s) { return s.len==128 && s.sig_mode<=1 && (s.sig_mode==1 || (s.rate>=8 && s.rate<=15)) && !s.cwb && !s.stbc && !s.secondary_channel && s.channel>=1 && s.channel<=14; }
inline unsigned amplitudes(const Sample &s,float *out) { if(!supported(s)) return 0; unsigned n=0; for(unsigned i=2;i<64;i++) { if(i>26 && i<38) continue; float im=s.raw[2*i],re=s.raw[2*i+1]; out[n++]=std::sqrt(im*im+re*re); } return n; }
inline bool source_allowed(const uint8_t *src,const uint8_t *allowed,const uint8_t *ap,const uint8_t *own) {return !memcmp(src,allowed,6) && memcmp(src,ap,6) && memcmp(src,own,6);}
inline bool direct_frame(const uint8_t *f,unsigned len,const uint8_t *allowed,const uint8_t *ap,const uint8_t *source_ap=nullptr) {return f && len>=24 && (f[0]&0x0c)==8 && (f[1]&3)==1 && !memcmp(f+10,allowed,6) && !memcmp(f+4,source_ap?source_ap:ap,6);}
// One Wi-Fi-task producer, one main-loop consumer. Release/acquire publishes copied storage.
template<unsigned N> class Queue { Sample entries_[N+1]{}; std::atomic<unsigned> head_{0},tail_{0}; public:
bool push(const Sample &s) { auto h=head_.load(std::memory_order_relaxed), next=(h+1)%(N+1); if(next==tail_.load(std::memory_order_acquire)) return false; entries_[h]=s; head_.store(next,std::memory_order_release); return true; }
bool pop(Sample &s) { auto t=tail_.load(std::memory_order_relaxed); if(t==head_.load(std::memory_order_acquire)) return false; s=entries_[t]; tail_.store((t+1)%(N+1),std::memory_order_release); return true; }
};
struct ReportMeta {uint32_t receiver{},boot{},seq{},dropped{},invalid{},unsupported{},addr2_frames{},intervals_lost{},report_errors{},throttled{},channel_epoch{};};
inline int encode_report(const Sample &s,const ReportMeta &meta,bool raw,char *body,size_t capacity) {
 if(!supported(s) || !body || capacity==0)return -1;
 char values[520];unsigned pos=0;
 if(raw){for(unsigned i=0;i<128;i++)pos+=snprintf(values+pos,sizeof(values)-pos,"%02x",uint8_t(s.raw[i]));}
 else{float a[51];auto count=atlantis::amplitudes(s,a);for(unsigned i=0;i<count;i++)pos+=snprintf(values+pos,sizeof(values)-pos,"%s%.2f",i?",":"",a[i]);}
 char src[18],bssid[18];auto mac=[](char *out,const uint8_t *m){snprintf(out,18,"%02x:%02x:%02x:%02x:%02x:%02x",m[0],m[1],m[2],m[3],m[4],m[5]);};mac(src,s.source);mac(bssid,s.bssid);
 int n=snprintf(body,capacity,"{\"v\":1,\"kind\":\"%s\",\"receiver\":%lu,\"boot\":%lu,\"seq\":%lu,\"timestamp_us\":%lu,\"source\":\"%s\",\"bssid\":\"%s\",\"channel\":%u,\"sig_mode\":%u,\"cwb\":%u,\"stbc\":%u,\"secondary_channel\":%u,\"rssi\":%d,\"first_word_invalid\":%s,\"%s\":%s%s%s,\"dropped\":%lu,\"invalid\":%lu,\"unsupported\":%lu,\"addr2_frames\":%lu,\"intervals_lost\":%lu,\"report_errors\":%lu,\"rate\":%u,\"mcs\":%u,\"sgi\":%u,\"aggregation\":%u,\"capture_seq\":%lu,\"throttled\":%lu,\"channel_epoch\":%lu,\"layout\":\"esp32-lltf-51-v1\",\"association\":\"csi_header_addr2\"}",raw?"csi":"feature",(unsigned long)meta.receiver,(unsigned long)meta.boot,(unsigned long)meta.seq,(unsigned long)s.timestamp_us,src,bssid,s.channel,s.sig_mode,s.cwb,s.stbc,s.secondary_channel,s.rssi,s.first_word_invalid?"true":"false",raw?"raw":"amplitudes",raw?"\"":"[",values,raw?"\"":"]",(unsigned long)meta.dropped,(unsigned long)meta.invalid,(unsigned long)meta.unsupported,(unsigned long)meta.addr2_frames,(unsigned long)meta.intervals_lost,(unsigned long)meta.report_errors,s.rate,s.mcs,s.sgi,s.aggregation,(unsigned long)s.seq,(unsigned long)meta.throttled,(unsigned long)meta.channel_epoch);
 return n;
}

}
