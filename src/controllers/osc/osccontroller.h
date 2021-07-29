#pragma once

#include "controllers/controller.h"
#include "controllers/osc/osc.h"
#include "controllers/osc/oscoutputhandler.h"
#include "controllers/osc/legacyosccontrollermapping.h"
#include "controllers/defs_controllers.h"
#include "util/duration.h"

#include <memory>

/// OSC controller backend
class OscController final : public Controller {
    Q_OBJECT
  public:
    explicit OscController();
    ~OscController() override;

    auto jsProxy() -> ControllerJSProxy* override;

    auto mappingExtension() -> QString override;

    auto cloneMapping() -> std::shared_ptr<LegacyControllerMapping> override;
    void setMapping(std::shared_ptr<LegacyControllerMapping> pMapping) override;

    bool isMappable() const override {
        if (!m_pMapping) {
            return false;
        }
        return m_pMapping->isMappable();
    }

    bool matchMapping(const MappingInfo& mapping) override;

    void sendMessage(OSC::Message const & message);

  private slots:
    int open() override;
    int close() override;

    void messageReceived(OSC::Message const &);

    bool applyMapping() override;

  private:
    void sendBytes(QByteArray const &) override;

    void debugMessage(char const * prefix, OSC::Message const & message);

    std::unique_ptr<OSC> m_pOsc;

    std::vector<std::unique_ptr<OscOutputHandler>> m_outputs;
    std::shared_ptr<LegacyOscControllerMapping> m_pMapping;

    friend class OscControllerJSProxy;
};

class OscControllerJSProxy : public ControllerJSProxy {
    Q_OBJECT
  public:
    OscControllerJSProxy(OscController* m_pController)
        : ControllerJSProxy(m_pController)
        , m_pOscController(m_pController)
        {}

    Q_INVOKABLE void sendMessage(QString addressPattern, float arg) {
        m_pOscController->sendMessage(OSC::Message{addressPattern.toStdString(), arg});
    }

  private:
    OscController* m_pOscController;
};
