#ifndef PROTOCOL_CALLBACKS_H
#define PROTOCOL_CALLBACKS_H

#include <functional>
#include "../motor/motor_state.h"

struct ProtocolCallbacks {
    std::function<void(float, float)> onSetPosition;
    std::function<void()> onStop;
    std::function<RotorPosition()> onGetPosition;
};

#endif
