#include "controllers/osc/osccontroller.h"

#include "control/controlproxy.h"
#include "controllers/controllerdebug.h"
#include "controllers/defs_controllers.h"
#include "moc_osccontroller.cpp"
#include "util/string.h"
#include "util/time.h"
#include "util/trace.h"

OscController::OscController() {
    setDeviceCategory("OSC");
    setDeviceName("127.0.0.1");
    
    // All OSC devices are full-duplex
    setInputDevice(true);
    setOutputDevice(true);
}

OscController::~OscController() {
    if (isOpen()) {
        close();
    }
}

auto OscController::mappingExtension() -> QString {
    return OSC_MAPPING_EXTENSION;
}

void OscController::setMapping(std::shared_ptr<LegacyControllerMapping> pMapping) {
    m_pMapping = downcastAndTakeOwnership<LegacyOscControllerMapping>(std::move(pMapping));
}

auto OscController::cloneMapping() -> std::shared_ptr<LegacyControllerMapping> {
    if (!m_pMapping) {
        return nullptr;
    }
    return m_pMapping->clone();
}

auto OscController::matchMapping(const MappingInfo& mapping) -> bool {
    // Product info mapping not implemented for OSC devices yet
    Q_UNUSED(mapping);
    return false;
}

void OscController::sendMessage(OSC::Message const & message) {
    debugMessage("sending", message);
    m_pOsc->send(message);
}

auto OscController::open() -> int {
    if (isOpen()) {
        qDebug() << "OSC device" << getName() << "already open";
        return -1;
    }

    // Open device by path
    controllerDebug("Opening OSC device" << getName());

    m_pOsc = std::make_unique<OSC>("127.0.0.1", 7700, 9000); // TODO
    connect(m_pOsc.get(), &OSC::receive, this, &OscController::messageReceived);

    setOpen(true);
    startEngine();

    return 0;
}

int OscController::close() {
    if (!isOpen()) {
        qDebug() << "OSC device" << getName() << "already closed";
        return -1;
    }

    qDebug() << "Shutting down OSC device" << getName();

    // Stop controller engine here to ensure it's done before the device is closed
    //  in case it has any final parting messages
    stopEngine();

    // Close device
    controllerDebug("  Closing device");
    //hid_close(m_pHidDevice);
    setOpen(false);
    return 0;
}

void OscController::messageReceived(OSC::Message const & message) {
    debugMessage("received", message);

    triggerActivity();

    auto it = m_pMapping->getInputMappings().constFind(message.addressPattern.c_str());
    for (; it != m_pMapping->getInputMappings().constEnd() && it.key() == message.addressPattern.c_str(); ++it) {
        auto const & mapping = it.value();

        /*
        // Custom script handler
        if (mapping.options.testFlag(MidiOption::Script)) {
            ControllerScriptEngineLegacy* pEngine = getScriptEngine();
            if (pEngine == nullptr) {
                return;
            }
            pEngine->handleIncomingData(data);
            return;
        }
        */

        // Only pass values on to valid ControlObjects.
        if (auto newValue = message.toFloat()) {
            ControlProxy{mapping.control}.setParameter(*newValue);
        }
    }
}

bool OscController::applyMapping() {
    auto result = Controller::applyMapping();

    m_outputs.clear();

    auto inputMappings = m_pMapping->getInputMappings();
    auto outputMappings = m_pMapping->getOutputMappings();

    std::transform
        ( inputMappings.begin()
        , inputMappings.end()
        , std::back_insert_iterator{m_outputs}
        , [this](OscMapping const & mapping) {
            return std::make_unique<OscOutputHandler>(*this, mapping);
          }
        );

    std::transform
        ( outputMappings.begin()
        , outputMappings.end()
        , std::back_insert_iterator{m_outputs}
        , [this](OscMapping const & mapping) {
            return std::make_unique<OscOutputHandler>(*this, mapping);
          }
        );

    return result;
}

void OscController::sendBytes(QByteArray const &) {
    qWarning() << "OscController::sendBytes not implemented";
}

void OscController::debugMessage
    ( char const * prefix
    , OSC::Message const & message
    )
{
    controllerDebug
        (  prefix
        << "OSC"
        << getName()
        << ":"
        << message.addressPattern.c_str()
        << ","
        << message.debugPayload().c_str()
        );
}

auto OscController::jsProxy() -> ControllerJSProxy* {
    return new OscControllerJSProxy{this};
}
