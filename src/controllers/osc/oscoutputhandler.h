#pragma once

#include "control/controlproxy.h"
#include "controllers/osc/oscmapping.h"

class OscController;

/// Static OSC output mapping handler
///
/// This class listens to a control object and sends a midi message based on
/// the  value.
class OscOutputHandler : public QObject {
    Q_OBJECT
  public:
    OscOutputHandler
        ( OscController & controller
        , OscMapping const & mapping
        )
        : m_pController{controller}
        , m_mapping{mapping}
        , m_cos{mapping.control, this, ControlFlag::NoAssertIfMissing | ControlFlag::AllowInvalidKey}
    {
        if (!m_cos.valid()) {
            qWarning() << "OscOutputHandler invalid control key: " << mapping.control;
        }
        m_cos.connectValueChanged(this, &OscOutputHandler::controlChanged);
        update();
    }

    auto validate() { return m_cos.valid(); }
    void update() { controlChanged(0); }

  private slots:
    void controlChanged(double);

  private:
    OscController & m_pController;
    OscMapping m_mapping;
    ControlProxy m_cos;
};
