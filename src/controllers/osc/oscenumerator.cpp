#include "controllers/osc/oscenumerator.h"

#include "controllers/osc/osccontroller.h"

OscEnumerator::~OscEnumerator() {
}

QList<Controller*> OscEnumerator::queryDevices() {
    return {new OscController{}};
}
