#include "atlantis_csi.h"
#include "esphome/core/hal.h"
#include "esp_random.h"
#include "esp_netif.h"
#include "esp_idf_version.h"
#include <cstdio>
#include <fcntl.h>
#if !CONFIG_IDF_TARGET_ESP32 && !CONFIG_IDF_TARGET_ESP32C3
#error Atlantis CSI supports ESP32 and ESP32-C3 only
#endif
namespace esphome { namespace atlantis_csi {
std::atomic<CSIReceiver*> CSIReceiver::owner_{nullptr};
void CSIReceiver::setup(){boot_=esp_random();ota::get_global_ota_callback()->add_global_state_listener(this);}
void CSIReceiver::fail_(uint32_t code){fault_=code;status_set_warning("CSI disabled: inspect fault diagnostic");stop_();}
void CSIReceiver::set_enabled(bool v){if(!v){stop_();return;} if(active_)return;if(ota_latched_){fault_=11;return;}fault_=0;if(!start_())return;status_clear_warning();}
bool CSIReceiver::start_(){
 if(!audited_){fail_(1);return false;} if(owner_.load() && owner_.load()!=this){fail_(2);return false;}
 bool promisc=false; if(esp_wifi_get_promiscuous(&promisc)!=ESP_OK || promisc){fail_(3);return false;}
 wifi_ap_record_t ap{}; if(esp_wifi_sta_get_ap_info(&ap)!=ESP_OK){fail_(4);return false;}
 if(esp_wifi_get_mac(WIFI_IF_STA,own_)!=ESP_OK){fail_(5);return false;}
 if(!atlantis::source_allowed(source_,source_,ap.bssid,own_)){fail_(6);return false;}
 esp_netif_ip_info_t ipinfo{};auto *netif=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
 if(!netif || esp_netif_get_ip_info(netif,&ipinfo)!=ESP_OK || !atlantis::destination_allowed(ntohl(inet_addr(ip_.c_str())),ntohl(ipinfo.ip.addr),ntohl(ipinfo.netmask.addr))){fail_(12);return false;}
 if(esp_wifi_get_promiscuous_filter(&saved_filter_)!=ESP_OK){fail_(7);return false;}filter_saved_=true;
 memcpy(ap_,ap.bssid,6); channel_=ap.primary; owner_=this;claimed_=true;
 wifi_csi_config_t cfg{}; cfg.lltf_en=true;cfg.htltf_en=true;cfg.stbc_htltf2_en=true;cfg.ltf_merge_en=true;cfg.channel_filter_en=true;cfg.manu_scale=false;
 wifi_promiscuous_filter_t filter{}; filter.filter_mask=WIFI_PROMIS_FILTER_MASK_DATA;
 if(esp_wifi_set_promiscuous_filter(&filter)!=ESP_OK || esp_wifi_set_promiscuous_rx_cb(frame_counter_?promisc_cb:nullptr)!=ESP_OK || esp_wifi_set_csi_rx_cb(csi_cb,this)!=ESP_OK || esp_wifi_set_csi_config(&cfg)!=ESP_OK){fail_(7);return false;}
 socket_=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); if(socket_<0){fail_(8);return false;}
 if(fcntl(socket_,F_SETFL,O_NONBLOCK)<0){fail_(8);return false;}
 destination_.sin_family=AF_INET;destination_.sin_port=htons(port_);destination_.sin_addr.s_addr=inet_addr(ip_.c_str());
 active_.store(true); if(esp_wifi_set_promiscuous(true)!=ESP_OK || esp_wifi_set_csi(true)!=ESP_OK){fail_(9);return false;}
 channel_epoch_++;last_send_=millis();last_health_=millis();return true;
}
void CSIReceiver::stop_(){active_.store(false);if(claimed_){esp_wifi_set_csi(false);esp_wifi_set_promiscuous(false);esp_wifi_set_csi_rx_cb(nullptr,nullptr);esp_wifi_set_promiscuous_rx_cb(nullptr);if(filter_saved_){esp_wifi_set_promiscuous_filter(&saved_filter_);filter_saved_=false;}claimed_=false;owner_=nullptr;}if(socket_>=0){::close(socket_);socket_=-1;}atlantis::Sample s;while(queue_.pop(s)){} }
void CSIReceiver::csi_cb(void *ctx,wifi_csi_info_t *info){
 auto *self=static_cast<CSIReceiver*>(ctx);if(!self || !self->active_.load())return;
 if(!info)return;
 if(!atlantis::source_allowed(info->mac,self->source_,self->ap_,self->own_))return;
 if(!info->buf || info->len!=128){self->length_rejects_++;self->invalid_++;return;}
 if(!atlantis::direct_frame(info->hdr,info->rx_ctrl.sig_len>=4 ? info->rx_ctrl.sig_len-4 : 0,self->source_,self->ap_,self->source_ap_set_?self->source_ap_:nullptr) || info->rx_ctrl.rx_state!=0){self->header_rejects_++;if(info->hdr && info->rx_ctrl.sig_len>=28 && memcmp(info->hdr+4,self->source_ap_set_?self->source_ap_:self->ap_,6))self->ap_mismatches_++;self->invalid_++;return;}
 atlantis::Sample s{}; s.len=info->len;s.rate=info->rx_ctrl.rate;s.mcs=info->rx_ctrl.mcs;s.sgi=info->rx_ctrl.sgi;s.aggregation=info->rx_ctrl.aggregation;s.sig_mode=info->rx_ctrl.sig_mode;s.cwb=info->rx_ctrl.cwb;s.stbc=info->rx_ctrl.stbc;s.secondary_channel=info->rx_ctrl.secondary_channel;s.channel=info->rx_ctrl.channel;
 if(!atlantis::supported(s)){self->unsupported_++;return;}if(s.channel!=self->channel_){self->invalid_++;return;}
 s.seq=self->seq_++;s.timestamp_us=info->rx_ctrl.timestamp;s.rssi=info->rx_ctrl.rssi;s.first_word_invalid=info->first_word_invalid;
 memcpy(s.source,info->mac,6);memcpy(s.bssid,info->hdr+4,6);memcpy(s.raw,info->buf,128);
 if(!self->queue_.push(s))self->dropped_++;
}
void CSIReceiver::promisc_cb(void *buf,wifi_promiscuous_pkt_type_t type){auto *self=owner_.load();if(!self || !self->active_.load() || !buf || type!=WIFI_PKT_DATA)return;auto *p=static_cast<wifi_promiscuous_pkt_t*>(buf);if(atlantis::direct_frame(p->payload,p->rx_ctrl.sig_len,self->source_,self->ap_,self->source_ap_set_?self->source_ap_:nullptr))self->addr2_frames_++;}
void CSIReceiver::loop(){if(!active_)return;auto now=millis();if(uint32_t(now-last_health_)>=1000){last_health_=now;wifi_ap_record_t ap{};if(esp_wifi_sta_get_ap_info(&ap)!=ESP_OK || ap.primary!=channel_ || memcmp(ap.bssid,ap_,6)){intervals_lost_++;fail_(10);return;}}
 atlantis::Sample s;for(unsigned n=0;n<16 && queue_.pop(s);n++){if(uint32_t(now-last_send_)<50){throttled_++;continue;}last_send_=now;send_(s);}}
void CSIReceiver::send_(const atlantis::Sample &s){
 char body[1400];
 atlantis::ReportMeta meta{receiver_,boot_,report_seq_++,dropped_.load(),invalid_.load(),unsupported_.load(),addr2_frames_.load(),intervals_lost_,report_errors_,throttled_,channel_epoch_};
 int n=atlantis::encode_report(s,meta,raw_,body,sizeof(body));
 if(n<=0 || n>=int(sizeof(body)) || sendto(socket_,body,n,MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&destination_),sizeof(destination_))!=n)report_errors_++;else reports_++;
}
}}
