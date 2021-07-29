#pragma once

#include "preferences/configobject.h"

#include <QString>

struct OscMapping {
    QString address;
    ConfigKey control;
    QString description;

    friend auto operator==(OscMapping const & lhs, OscMapping const & rhs) -> bool {
        return
            (  lhs.address == rhs.address
            && lhs.control == rhs.control
            && lhs.description == rhs.description
            );
    }
};

