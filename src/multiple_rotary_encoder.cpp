#include "wledBridge.h"

// 1. Create the actual object in memory
WLED_Bridge Instance_wledBridge;

// 2. Feed the WLED Python script exactly what it is looking for
REGISTER_USERMOD(Instance_wledBridge);
