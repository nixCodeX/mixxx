/// HID/Bulk Controller Mapping
///
/// This class represents a HID or Bulk controller mapping, containing the data
/// elements that make it up.

#include "controllers/osc/legacyosccontrollermapping.h"

#include "controllers/defs_controllers.h"
#include "controllers/osc/legacyosccontrollermappingfilehandler.h"

auto LegacyOscControllerMapping::saveMapping(const QString& fileName) const -> bool {
    auto handler = LegacyOscControllerMappingFileHandler{};
    return handler.save(*this, fileName);
}

void LegacyOscControllerMapping::addInputMapping(QString key, OscMapping const & mapping) {
    m_inputMappings.insert(key, mapping);
    setDirty(true);
}

void LegacyOscControllerMapping::removeInputMapping(QString key) {
    m_inputMappings.remove(key);
    setDirty(true);
}

auto LegacyOscControllerMapping::getInputMappings() const -> QMultiHash<QString, OscMapping> const & {
    return m_inputMappings;
}

void LegacyOscControllerMapping::setInputMappings(QMultiHash<QString, OscMapping> const & mappings) {
    if (m_inputMappings != mappings) {
        m_inputMappings.clear();
        m_inputMappings.unite(mappings);
        setDirty(true);
    }
}

void LegacyOscControllerMapping::addOutputMapping(ConfigKey const & key, OscMapping const & mapping) {
    m_outputMappings.insert(key, mapping);
    setDirty(true);
}

void LegacyOscControllerMapping::removeOutputMapping(ConfigKey const & key) {
    m_outputMappings.remove(key);
    setDirty(true);
}

auto LegacyOscControllerMapping::getOutputMappings() const -> QMultiHash<ConfigKey, OscMapping> const & {
    return m_outputMappings;
}

void LegacyOscControllerMapping::setOutputMappings(QMultiHash<ConfigKey, OscMapping> const & mappings) {
    if (m_outputMappings != mappings) {
        m_outputMappings.clear();
        m_outputMappings.unite(mappings);
        setDirty(true);
    }
}
