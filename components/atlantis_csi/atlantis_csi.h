#pragma once
#include "esphome/core/component.h"
#include "csi_core.h"
#include "esphome/components/ota/ota_backend.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
namespace esphome { namespace atlantis_csi {
std::string last_abort_detail();
std::string last_heap_detail();
class CSIReceiver : public Component, public ota::OTAGlobalStateListener {
 public:
 void set_receiver_id(uint32_t v){receiver_=v;} void set_source(uint64_t v){for(int i=5;i>=0;i--){source_[i]=v&255;v>>=8;}}
 void set_source_bssid(uint64_t v){source_ap_set_=true;for(int i=5;i>=0;i--){source_ap_[i]=v&255;v>>=8;}}
 void set_destination(const std::string &ip,uint16_t port){ip_=ip;port_=port;}
 void set_frame_counter(bool v){frame_counter_=v;}
 void set_audited(bool v){audited_=v;} void set_raw(bool v){raw_=v;}
 void on_ota_global_state(ota::OTAState state,float,uint8_t,ota::OTAComponent*) override {if(state==ota::OTA_STARTED){ota_latched_=true;stop_();}}
 void setup() override; void loop() override; void on_shutdown() override {set_enabled(false);}
 float get_setup_priority() const override {return setup_priority::AFTER_WIFI;}
 void set_enabled(bool v); bool enabled() const {return active_.load();}
 uint32_t dropped() const{return dropped_.load();} uint32_t invalid() const{return invalid_.load();}
 uint32_t unsupported() const{return unsupported_.load();} uint32_t reports() const{return reports_;}
 uint32_t report_errors() const{return report_errors_;} uint32_t fault() const{return fault_;}
 uint32_t addr2_frames() const{return addr2_frames_.load();} uint32_t intervals_lost() const{return intervals_lost_;}
 uint32_t length_rejects() const{return length_rejects_.load();}
 uint32_t header_rejects() const{return header_rejects_.load();}
 uint32_t ap_mismatches() const{return ap_mismatches_.load();}
 uint32_t channel() const{return channel_;}
 std::string bssid() const {char out[18];snprintf(out,sizeof(out),"%02x:%02x:%02x:%02x:%02x:%02x",ap_[0],ap_[1],ap_[2],ap_[3],ap_[4],ap_[5]);return out;}
 private:
 static std::atomic<CSIReceiver*> owner_; static void csi_cb(void*,wifi_csi_info_t*); static void promisc_cb(void*,wifi_promiscuous_pkt_type_t);
 bool start_(); void stop_(); void fail_(uint32_t code); void send_(const atlantis::Sample&);
 atlantis::Queue<16> queue_; std::atomic<bool> active_{false};
 std::atomic<uint32_t> dropped_{0},invalid_{0},unsupported_{0},addr2_frames_{0},seq_{0};
 std::atomic<uint32_t> length_rejects_{0},header_rejects_{0},ap_mismatches_{0};
 uint8_t source_ap_[6]{}; bool source_ap_set_{false};
 uint8_t source_[6]{},ap_[6]{},own_[6]{}; uint8_t channel_{};
 uint32_t receiver_{},boot_{},fault_{},reports_{},report_errors_{},intervals_lost_{},last_send_{},last_health_{},report_seq_{},throttled_{},channel_epoch_{};
 wifi_promiscuous_filter_t saved_filter_{}; bool filter_saved_{false};
 bool frame_counter_{true}; bool ota_latched_{false}; bool audited_{false},raw_{true},claimed_{false}; int socket_{-1}; std::string ip_; uint16_t port_{}; sockaddr_in destination_{};
};
}}
