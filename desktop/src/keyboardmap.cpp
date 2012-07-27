/*
    MIDI Virtual Piano Keyboard
    Copyright (C) 2008-2012, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; If not, see <http://www.gnu.org/licenses/>.
*/

#include "keyboardmap.h"

#include <QtCore/QFile>
#include <QDomDocument>
#include <QtXml/QXmlStreamWriter>
#include <QtGui/QKeySequence>
#include <QtGui/QMessageBox>

void KeyboardMap::loadFromXMLFile(const QString fileName)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        initializeFromXML(&f);
        f.close();
        m_fileName = fileName;
        //qDebug() << "Loaded Map: " << fileName;
    }
    if (f.error() != QFile::NoError) {
        reportError(fileName, tr("Error loading a file"), f.errorString());
    }
}

void KeyboardMap::saveToXMLFile(const QString fileName)
{
    QFile f(fileName);
    if (f.open(QFile::WriteOnly | QFile::Text)) {
        serializeToXML(&f);
        f.close();
        m_fileName = fileName;
        //qDebug() << "Saved Map: " << fileName;
    }
    if (f.error() != QFile::NoError) {
        reportError(fileName, tr("Error saving a file"), f.errorString());
    }
}

void KeyboardMap::initializeFromXML(QIODevice *dev)
{
    QDomDocument doc;
    
    // Read in document, checking for well-formedness
    QString errorMsg;
    int errorLine;
    int errorCol;
    if (!doc.setContent(dev, true, &errorMsg, &errorLine, &errorCol)) {
        reportError(QString(), tr("Error reading XML"), tr("Invalid XML at line %1 column %2: %3").arg(errorLine).arg(errorCol).arg(errorMsg));
        return;
    }
    clear();
    
    // Make sure it's a keymap of the right type/version
    const QDomElement root = doc.documentElement();
    const QString desiredMode = m_rawMode?"rawkeymap":"keyboardmap";
    if (root.tagName() != desiredMode) {
        reportError(QString(), tr("Error reading XML keyboard map"), tr("Type of key map (%1) doesn't match mode (%2)").arg(root.tagName()).arg(desiredMode));
        return;
    }
    if (!root.hasAttribute("version") || root.attribute("version") != "1.0") {
        reportError(QString(), tr("Error reading XML keyboard map"), tr("Keymap version not specified or not supported"));
        return;
    }
    
    // Read each mapping element
    QDomElement mapping = root.firstChildElement();
    while (!mapping.isNull()) {
        if (mapping.tagName() == "mapping") { // make sure it's actually the right tag
            const QString keyAttrName = m_rawMode?"keycode":"key"; // desired
            if (!mapping.hasAttribute(keyAttrName) || !mapping.hasAttribute("note")) {
                reportError(QString(), tr("Error reading XML keyboard map"), tr("Missing attribute for mapping"));
                return;
            }
            const QString key = mapping.attribute(keyAttrName);
            const QString sn = mapping.attribute("note");
            bool ok = false;
            const int note = sn.toInt(&ok);
            if (!ok) {
                reportError(QString(), tr("Error reading XML keyboard map"), tr("Bogus note number %1").arg(sn));
                return;
            }
            if (m_rawMode) {
                int keycode = key.toInt(&ok);
                if (!ok) {
                    reportError(QString(), tr("Error reading XML keyboard map"), tr("Bogus keycode %1").arg(key));
                    return;
                }
                insert(keycode, note);
            } else {
                QKeySequence ks(key);
                insert(ks[0], note);
            }
        } else {
            reportError(QString(), tr("Error reading XML keyboard map"), tr("Bogus element %1 in keyboard map (expected mapping)").arg(mapping.tagName()));
            return;
        }
        mapping = mapping.nextSiblingElement();
    }
}

void KeyboardMap::serializeToXML(QIODevice *dev)
{
    QXmlStreamWriter writer(dev);
    writer.setAutoFormatting(true);
    //writer.setCodec("UTF-8");
    writer.writeStartDocument();
    writer.writeDTD(m_rawMode?"<!DOCTYPE rawkeyboardmap>":"<!DOCTYPE keyboardmap>");
    writer.writeStartElement(m_rawMode ? "rawkeymap" : "keyboardmap");
    writer.writeAttribute("version", "1.0");
    foreach(int key, keys()) {
        writer.writeEmptyElement("mapping");
        if (m_rawMode)
            writer.writeAttribute("keycode", QString::number(key));
        else {
            QKeySequence ks(key);
            writer.writeAttribute("key", ks.toString(QKeySequence::PortableText));
        }
        writer.writeAttribute("note", QString::number(value(key)));
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

void KeyboardMap::copyFrom(const KeyboardMap* other)
{
    m_fileName = other->getFileName();
    m_rawMode = other->getRawMode();
    clear();
    KeyboardMap::ConstIterator it;
    for(it = other->begin(); it != other->end(); ++it)
        insert(it.key(), it.value());
}

void KeyboardMap::reportError( const QString filename,
                               const QString title,
                               const QString err )
{
    QMessageBox::warning(0, title, tr("File: %1\n%2").arg(filename).arg(err));
}
