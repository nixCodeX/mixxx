#pragma once

#include "controllers/controllerenumerator.h"

/// This class enumerates the OSC controller.
class OscEnumerator : public ControllerEnumerator {
  public:
    OscEnumerator() = default;
    ~OscEnumerator() override;

    QList<Controller*> queryDevices() override;
};
