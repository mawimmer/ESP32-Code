#pragma once
#include <Arduino.h>

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

namespace WLED_Bridge {

    int lastUpdate = 0;

    inline void toggleSegment(int8_t segmentID) {
        Segment& seg = strip.getSegment(segmentID);

        if(seg.getOption(SEG_OPTION_ON)) {
            seg.setOption(SEG_OPTION_ON, false);
            stateUpdated(CALL_MODE_BUTTON);

            // not needed? apparently stateUpdated() schedules interface updates
            //
            // updateInterfaces(CALL_MODE_BUTTON);
        } else {
            seg.setOption(SEG_OPTION_ON, true);
            stateUpdated(CALL_MODE_BUTTON);

            // not needed? apparently stateUpdated() schedules interface updates
            //
            // updateInterfaces(CALL_MODE_BUTTON);

        }
    }

    inline void setSegmentBrightness(int8_t segmentID, int brightness) {
        Segment& seg = strip.getSegment(segmentID);
        seg.opacity = brightness;
        stateUpdated(CALL_MODE_BUTTON);

        // not needed? apparently stateUpdated() schedules interface updates
        //
        // int timeNOW = millis();
        // if( timeNOW - lastUpdate > 200 ) {
        //     updateInterfaces(CALL_MODE_BUTTON);
        //     lastUpdate = timeNOW;
        // }

    }

    inline void setSegmentEffect(int8_t segmentID, int8_t effectID) {
        Segment& seg = strip.getSegment(segmentID);
        seg.setMode(effectID);
        colorUpdated(CALL_MODE_BUTTON); // not like stateUpdated(), colorUpdated() pushes effect changes

        // i guess needed, no statement colorUpdated() schedules interface updates like stateUpdated()
        updateInterfaces(CALL_MODE_BUTTON);

    }

    inline void setSegmentEffectConfig(int8_t segmentID, int8_t configIndex, int8_t deltaValue) {
        Segment& seg = strip.getSegment(segmentID);
     
        switch(configIndex) {
            case 0: seg.speed += deltaValue; break;
            case 1: seg.intensity += deltaValue; break;
            case 2: seg.opacity += deltaValue; break;
            case 3: seg.custom1 += deltaValue; break;
            case 4: seg.custom2 += deltaValue; break;
            case 5: seg.custom3 += deltaValue; break;
            default: return;
        }
        
        stateUpdated(CALL_MODE_BUTTON);

        // not needed? apparently stateUpdated() schedules interface updates
        //
        // updateInterfaces(CALL_MODE_BUTTON);
    }

};



