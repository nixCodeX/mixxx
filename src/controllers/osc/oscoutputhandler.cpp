#include "controllers/midi/midioutputhandler.h"

#include <QtDebug>

#include "control/controlobject.h"
#include "controllers/controllerdebug.h"
#include "controllers/osc/osc.h"
#include "controllers/osc/osccontroller.h"
#include "moc_oscoutputhandler.cpp"

void OscOutputHandler::controlChanged(double) {
    // Don't update with out of date messages.
    auto value = m_cos.get();

    if (!m_pController.isOpen()) {
        qWarning() << "OSC device" << m_pController.getName() << "not open for output!";
    } else {
        m_pController.sendMessage(OSC::Message{m_mapping.address.toStdString(), static_cast<float>(value)});
    }
}
