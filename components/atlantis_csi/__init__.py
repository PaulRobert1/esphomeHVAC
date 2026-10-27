import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32
from esphome.const import CONF_ID
DEPENDENCIES=['wifi','esp32','ota']
ns=cg.esphome_ns.namespace('atlantis_csi')
CSIReceiver=ns.class_('CSIReceiver',cg.Component)
CONFIG_SCHEMA=cv.All(cv.Schema({cv.GenerateID():cv.declare_id(CSIReceiver),cv.Required('receiver_id'):cv.uint32_t,cv.Required('transmitter_mac'):cv.mac_address,cv.Optional('transmitter_bssid'):cv.mac_address,cv.Required('collector_ip'):cv.ipv4address,cv.Optional('collector_port',default=49153):cv.port,cv.Required('exclusive_radio_owner_audited'):cv.boolean,cv.Optional('raw_reports',default=True):cv.boolean,cv.Optional('independent_frame_counter',default=True):cv.boolean}).extend(cv.COMPONENT_SCHEMA),cv.require_framework_version(esp_idf=cv.Version(5,5,5),esp32_arduino=cv.Version(3,3,11)))
async def to_code(config):
 var=cg.new_Pvariable(config[CONF_ID]); await cg.register_component(var,config)
 cg.add_define('USE_OTA_STATE_LISTENER')
 cg.add_build_flag('-Wl,--wrap=esp_system_abort')
 cg.add_build_flag('-Wl,--wrap=esp_rom_printf')
 cg.add_build_flag('-Wl,--wrap=free')
 cg.add_build_flag('-Wl,--wrap=heap_caps_free')
 cg.add(var.set_receiver_id(config['receiver_id']))
 cg.add(var.set_source(config['transmitter_mac'].as_hex))
 if 'transmitter_bssid' in config: cg.add(var.set_source_bssid(config['transmitter_bssid'].as_hex))
 cg.add(var.set_destination(str(config['collector_ip']),config['collector_port']))
 cg.add(var.set_audited(config['exclusive_radio_owner_audited']))
 cg.add(var.set_raw(config['raw_reports']))
 cg.add(var.set_frame_counter(config['independent_frame_counter']))
 esp32.add_idf_sdkconfig_option('CONFIG_ESP_WIFI_CSI_ENABLED',True)
