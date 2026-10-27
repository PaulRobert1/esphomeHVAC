#include <cassert>
#include "../circulation_request.h"
#include "../solar_controller.h"
#include "../heatpump_controller.h"
int main() {
 hp::CirculationRequest r; assert(!r.requested(0)); r.receive(1,100); assert(r.requested(20099)); assert(!r.requested(20100)); r.receive(2,20200);assert(!r.requested(20200));
 hp::SolarController s;assert(s.tick(60,40,55,true,true,0)==0);assert(s.tick(60,40,55,true,true,20000)==.30f);assert(s.tick(120,40,55,true,true,21000)==0);
 hp::Controller c;c.begin(100);assert(!c.compressor);assert(c.remaining(100)==300);c.tick(301000,true);assert(!c.compressor);assert(c.fault==hp::Fault::Sensor);
}
