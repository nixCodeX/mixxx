#include "controllers/osc/legacyosccontrollermappingfilehandler.h"

auto LegacyOscControllerMappingFileHandler::load
    ( const QDomElement& root
    , const QString& filePath
    , const QDir& systemMappingsPath
    ) -> std::shared_ptr<LegacyControllerMapping>
{
    if (root.isNull()) {
        return nullptr;
    }

    QDomElement controller = getControllerNode(root);
    if (controller.isNull()) {
        return nullptr;
    }

    auto pMapping = std::make_shared<LegacyOscControllerMapping>();
    pMapping->setFilePath(filePath);

    // Superclass handles parsing <info> tag and script files
    parseMappingInfo(root, pMapping);
    addScriptFilesToMapping(controller, pMapping, systemMappingsPath);

    QDomElement control = controller.firstChildElement("controls").firstChildElement("control");

    // Iterate through each <control> block in the XML
    while (!control.isNull()) {
        //Unserialize these objects from the XML
        QString address = control.firstChildElement("address").text();

        QDomElement groupNode = control.firstChildElement("group");
        QDomElement keyNode = control.firstChildElement("key");
        QDomElement descriptionNode = control.firstChildElement("description");

        QString controlGroup = groupNode.text();
        QString controlKey = keyNode.text();
        QString controlDescription = descriptionNode.text();

        /*
        // Get options
        QDomElement optionsNode = control.firstChildElement("options").firstChildElement();

        MidiOptions options;

        QString strMidiOption;
        while (!optionsNode.isNull()) {
            strMidiOption = optionsNode.nodeName().toLower();

            // "normal" is no options
            if (strMidiOption == QLatin1String("invert")) {
                options.setFlag(MidiOption::Invert);
            } else if (strMidiOption == QLatin1String("rot64")) {
                options.setFlag(MidiOption::Rot64);
            } else if (strMidiOption == QLatin1String("rot64inv")) {
                options.setFlag(MidiOption::Rot64Invert);
            } else if (strMidiOption == QLatin1String("rot64fast")) {
                options.setFlag(MidiOption::Rot64Fast);
            } else if (strMidiOption == QLatin1String("diff")) {
                options.setFlag(MidiOption::Diff);
            } else if (strMidiOption == QLatin1String("button")) {
                options.setFlag(MidiOption::Button);
            } else if (strMidiOption == QLatin1String("switch")) {
                options.setFlag(MidiOption::Switch);
            } else if (strMidiOption == QLatin1String("hercjog")) {
                options.setFlag(MidiOption::HercJog);
            } else if (strMidiOption == QLatin1String("hercjogfast")) {
                options.setFlag(MidiOption::HercJogFast);
            } else if (strMidiOption == QLatin1String("spread64")) {
                options.setFlag(MidiOption::Spread64);
            } else if (strMidiOption == QLatin1String("selectknob")) {
                options.setFlag(MidiOption::SelectKnob);
            } else if (strMidiOption == QLatin1String("soft-takeover")) {
                options.setFlag(MidiOption::SoftTakeover);
            } else if (strMidiOption == QLatin1String("script-binding")) {
                options.setFlag(MidiOption::Script);
            } else if (strMidiOption == QLatin1String("fourteen-bit-msb")) {
                options.setFlag(MidiOption::FourteenBitMSB);
            } else if (strMidiOption == QLatin1String("fourteen-bit-lsb")) {
                options.setFlag(MidiOption::FourteenBitLSB);
            }

            optionsNode = optionsNode.nextSiblingElement();
        }
        */

        OscMapping inputMapping;
        inputMapping.control = ConfigKey(controlGroup, controlKey);
        inputMapping.description = controlDescription;
        //inputMapping.options = options;
        inputMapping.address = address;

        // qDebug() << "New inputMapping:" << QString::number(inputMapping.key.key, 16).toUpper()
        //          << QString::number(inputMapping.key.status, 16).toUpper()
        //          << QString::number(inputMapping.key.control, 16).toUpper()
        //          << inputMapping.control.group << inputMapping.control.item;
        pMapping->addInputMapping(inputMapping.address, inputMapping);
        control = control.nextSiblingElement("control");
    }

    qDebug() << "LegacyOscControllerMappingFileHandler: Input mapping parsing complete.";

    // Parse static output mappings

    QDomElement output = controller.firstChildElement("outputs").firstChildElement("output");

    // Iterate through each <control> block in the XML
    while (!output.isNull()) {
        // Deserialize the control components from the XML
        QDomElement groupNode = output.firstChildElement("group");
        QDomElement keyNode = output.firstChildElement("key");
        QDomElement descriptionNode = output.firstChildElement("description");

        QString controlGroup = groupNode.text();
        QString controlKey = keyNode.text();
        QString controlDescription = descriptionNode.text();

        // Unserialize output message from the XML
        OscMapping outputMapping;
        outputMapping.address = output.firstChildElement("address").text();
        outputMapping.control = ConfigKey(controlGroup, controlKey);
        outputMapping.description = controlDescription;
        // END unserialize output

        // Add the static output mapping.
        // qDebug() << "New output mapping:"
        //          << QString::number(outputMapping.output.status, 16).toUpper()
        //          << QString::number(outputMapping.output.control, 16).toUpper()
        //          << QString::number(outputMapping.output.on, 16).toUpper()
        //          << QString::number(outputMapping.output.off, 16).toUpper()
        //          << controlGroup << controlKey;

        // Use insertMulti because we support multiple outputs from the same
        // control.
        pMapping->addOutputMapping(outputMapping.control, outputMapping);

        output = output.nextSiblingElement("output");
    }

    qDebug() << "LegacyOscMappingFileHandler: Output mapping parsing complete.";

    return pMapping;
}

auto LegacyOscControllerMappingFileHandler::save
    ( LegacyOscControllerMapping const & mapping
    , QString const & fileName
    ) const -> bool
{
    qDebug() << "Saving mapping" << mapping.name() << "to" << fileName;
    QDomDocument doc = buildRootWithScripts(mapping);
    addControlsToDocument(mapping, &doc);
    return writeDocument(doc, fileName);
}

void LegacyOscControllerMappingFileHandler::addControlsToDocument
    ( LegacyOscControllerMapping const & mapping
    , QDomDocument* doc
    ) const
{
    QDomElement controller = doc->documentElement().firstChildElement("controller");

    // The QHash doesn't guarantee iteration order, so first we sort the keys
    // so the xml output will be consistent.
    QDomElement controls = doc->createElement("controls");
    // We will iterate over all of the values that have the same keys, so we need
    // to remove duplicate keys or else we'll duplicate those values.
    auto sortedInputKeys = mapping.getInputMappings().uniqueKeys();
    std::sort(sortedInputKeys.begin(), sortedInputKeys.end());
    for (const auto& key : sortedInputKeys) {
        for (auto it = mapping.getInputMappings().constFind(key);
                it != mapping.getInputMappings().constEnd() && it.key() == key;
                ++it) {
            QDomElement controlNode = mappingToXML(doc, it.value());
            controls.appendChild(controlNode);
        }
    }
    controller.appendChild(controls);

    // Repeat the process for the output mappings.
    QDomElement outputs = doc->createElement("outputs");
    auto sortedOutputKeys = mapping.getOutputMappings().uniqueKeys();
    std::sort(sortedOutputKeys.begin(), sortedOutputKeys.end());
    for (const auto& key : sortedOutputKeys) {
        for (auto it = mapping.getOutputMappings().constFind(key);
                it != mapping.getOutputMappings().constEnd() && it.key() == key;
                ++it) {
            QDomElement outputNode = mappingToXML(doc, it.value());
            outputs.appendChild(outputNode);
        }
    }
    controller.appendChild(outputs);
}

QDomElement LegacyOscControllerMappingFileHandler::makeTextElement
    ( QDomDocument* doc
    , QString const & elementName
    , QString const & text
    ) const
{
    QDomElement tagNode = doc->createElement(elementName);
    QDomText textNode = doc->createTextNode(text);
    tagNode.appendChild(textNode);
    return tagNode;
}

QDomElement LegacyOscControllerMappingFileHandler::mappingToXML
    ( QDomDocument* doc
    , OscMapping const & mapping
    ) const
{
    QDomElement controlNode = doc->createElement("control");

    controlNode.appendChild(makeTextElement(doc, "group", mapping.control.group));
    controlNode.appendChild(makeTextElement(doc, "key", mapping.control.item));
    if (!mapping.description.isEmpty()) {
        controlNode.appendChild(
                makeTextElement(doc, "description", mapping.description));
    }

    // MIDI status byte
    controlNode.appendChild(makeTextElement(doc, "address", mapping.address));

    /*
    //Midi options
    QDomElement optionsNode = doc->createElement("options");

    // "normal" is no options
    if (!mapping.options) {
        QDomElement singleOption = doc->createElement("normal");
        optionsNode.appendChild(singleOption);
    } else {
        if (mapping.options.testFlag(MidiOption::Invert)) {
            QDomElement singleOption = doc->createElement("invert");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Rot64)) {
            QDomElement singleOption = doc->createElement("rot64");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Rot64Invert)) {
            QDomElement singleOption = doc->createElement("rot64inv");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Rot64Fast)) {
            QDomElement singleOption = doc->createElement("rot64fast");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Diff)) {
            QDomElement singleOption = doc->createElement("diff");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Button)) {
            QDomElement singleOption = doc->createElement("button");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Switch)) {
            QDomElement singleOption = doc->createElement("switch");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::HercJog)) {
            QDomElement singleOption = doc->createElement("hercjog");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::HercJogFast)) {
            QDomElement singleOption = doc->createElement("hercjogfast");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Spread64)) {
            QDomElement singleOption = doc->createElement("spread64");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::SelectKnob)) {
            QDomElement singleOption = doc->createElement("selectknob");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::SoftTakeover)) {
            QDomElement singleOption = doc->createElement("soft-takeover");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::Script)) {
            QDomElement singleOption = doc->createElement("script-binding");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::FourteenBitMSB)) {
            QDomElement singleOption = doc->createElement("fourteen-bit-msb");
            optionsNode.appendChild(singleOption);
        }
        if (mapping.options.testFlag(MidiOption::FourteenBitLSB)) {
            QDomElement singleOption = doc->createElement("fourteen-bit-lsb");
            optionsNode.appendChild(singleOption);
        }
    }
    controlNode.appendChild(optionsNode);
    */

    return controlNode;
}
