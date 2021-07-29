#pragma once

#include <QMultiHash>

#include "controllers/osc/oscmapping.h"
#include "controllers/legacycontrollermapping.h"
#include "preferences/usersettings.h"

/// This class represents an OSC controller mapping, containing the data
/// elements that make it up.
class LegacyOscControllerMapping : public LegacyControllerMapping {
  public:
    LegacyOscControllerMapping() = default;
    ~LegacyOscControllerMapping() override {}

    std::shared_ptr<LegacyControllerMapping> clone() const override {
        return std::make_shared<LegacyOscControllerMapping>(*this);
    }

    bool saveMapping(const QString& fileName) const override;

    bool isMappable() const override { return true; }

    // Input mappings
    void addInputMapping(QString address, OscMapping const & mapping);
    void removeInputMapping(QString key);
    auto getInputMappings() const -> QMultiHash<QString, OscMapping> const &;
    void setInputMappings(QMultiHash<QString, OscMapping> const & mappings);

    // Output mappings
    void addOutputMapping(ConfigKey const & key, OscMapping const & mapping);
    void removeOutputMapping(ConfigKey const & key);
    auto getOutputMappings() const -> QMultiHash<ConfigKey, OscMapping> const &;
    void setOutputMappings(QMultiHash<ConfigKey, OscMapping> const & mappings);


  private:
    // Osc input and output mappings.
    QMultiHash<QString, OscMapping> m_inputMappings;
    QMultiHash<ConfigKey, OscMapping> m_outputMappings;
};
