#pragma once

#include "controllers/osc/legacyosccontrollermapping.h"
#include "controllers/legacycontrollermappingfilehandler.h"

class LegacyOscControllerMapping;

class LegacyOscControllerMappingFileHandler : public LegacyControllerMappingFileHandler {
  public:
    LegacyOscControllerMappingFileHandler(){};
    virtual ~LegacyOscControllerMappingFileHandler(){};

    auto save(const LegacyOscControllerMapping& mapping, const QString& fileName) const -> bool;

  private:
    virtual auto load
        ( const QDomElement& root
        , const QString& filePath
        , const QDir& systemMappingsPath
        ) -> std::shared_ptr<LegacyControllerMapping>;

    void addControlsToDocument
        ( LegacyOscControllerMapping const & mapping
        , QDomDocument* doc
        ) const;

    auto makeTextElement
        ( QDomDocument* doc
        , QString const & elementName
        , QString const & text
        ) const -> QDomElement;

    auto mappingToXML
        ( QDomDocument* doc
        , OscMapping const & mapping
        ) const -> QDomElement;
};
