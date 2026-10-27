// Preserve the assertion text over a software reset; no flash writes in panic context.
#include "esp_attr.h"
#include "esp_memory_utils.h"
#include <cstdint>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace {
struct AbortRecord {uint32_t magic; char text[192]; uint32_t free_caller, heap_caller, pointer; char task[24];};
volatile uint32_t free_caller=0,heap_caller=0,free_pointer=0;
__NOINIT_ATTR AbortRecord record;
}
extern "C" void __real_esp_system_abort(const char*) __attribute__((noreturn));
extern "C" void IRAM_ATTR __wrap_esp_system_abort(const char *details) {
 record.magic=0;
 record.free_caller=free_caller;record.heap_caller=heap_caller;record.pointer=free_pointer;
 const char *task=pcTaskGetName(nullptr);
 unsigned j=0;for(;task && j<23 && task[j];++j)record.task[j]=task[j];record.task[j]=0;
 unsigned i=0;
 if(details && esp_ptr_in_dram(details)) {
  volatile const char *input=details;
  for(;i<sizeof(record.text)-1 && esp_ptr_in_dram(details+i);++i){char c=input[i];record.text[i]=c;if(!c)break;}
 }
 record.text[i]=0;record.text[191]=0;record.magic=0x41524644;
 __real_esp_system_abort(details);
}
namespace esphome {namespace atlantis_csi {
std::string last_abort_detail(){
 if(record.magic!=0x41524644 || record.text[191]!=0)return "No saved abort detail";
 char detail[180];snprintf(detail,sizeof(detail),"; task=%s free=%08lx heap=%08lx ptr=%08lx",record.task,(unsigned long)record.free_caller,(unsigned long)record.heap_caller,(unsigned long)record.pointer);
 return std::string(record.text)+detail;
}
}}
#include "esp_rom_sys.h"
#include <cstdarg>
namespace {
struct HeapRecord {uint32_t magic;uint32_t address,expected,actual;};
__NOINIT_ATTR HeapRecord heap_record;
}
extern "C" int IRAM_ATTR __wrap_esp_rom_printf(const char *fmt,...) {
 va_list ap;va_start(ap,fmt);
 // Poisoning failures have three integer arguments; do not allocate under the heap lock.
 if(fmt && fmt[0]=='C' && fmt[1]=='O' && fmt[2]=='R' && fmt[3]=='R' && fmt[4]=='U' && fmt[5]=='P' && fmt[6]=='T' && fmt[7]==' ' && fmt[8]=='H' && (fmt[14]=='B' || fmt[14]=='I' || fmt[14]=='S')) {
  va_list copy;va_copy(copy,ap);heap_record.magic=0;
  heap_record.address=(uint32_t)va_arg(copy,void*);heap_record.expected=va_arg(copy,unsigned);heap_record.actual=va_arg(copy,unsigned);
  heap_record.magic=0x48454150;va_end(copy);
 }
 int n=esp_rom_vprintf(fmt,ap);va_end(ap);return n;
}
namespace esphome {namespace atlantis_csi {
std::string last_heap_detail(){
 if(heap_record.magic!=0x48454150)return "No saved heap guard failure";
 char b[128];snprintf(b,sizeof(b),"Heap guard at 0x%08lx expected 0x%08lx actual 0x%08lx",(unsigned long)heap_record.address,(unsigned long)heap_record.expected,(unsigned long)heap_record.actual);return b;
}
}}

extern "C" void __real_free(void *p);
extern "C" void __real_heap_caps_free(void *p);
extern "C" void __wrap_free(void *p){
 free_caller=(uint32_t)__builtin_return_address(0);__real_free(p);
}
extern "C" void __wrap_heap_caps_free(void *p){
 heap_caller=(uint32_t)__builtin_return_address(0);free_pointer=(uint32_t)p;__real_heap_caps_free(p);
}
