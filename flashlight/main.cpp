/*
 * SPDX-FileCopyrightText: 2025-2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flashlightservice.h"

#include <QCoreApplication>
#include <QString>
#include <QDBusConnection>
#include <QtDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("droidmedia-flashlight");

    FlashlightService flashlightService;
    QString serviceName = QLatin1String("org.sailfish.flashlight.provider");
    QString objectPath = QLatin1String("/org/sailfish/flashlight/provider");
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    bool registeredObject = sessionBus.registerObject(objectPath, &flashlightService, QDBusConnection::ExportAllContents);
    bool registeredService = sessionBus.registerService(serviceName);

    int retn = 1;
    if (!registeredService || !registeredObject) {
        qWarning() << Q_FUNC_INFO << "CRITICAL: unable to register"
                   << "sync service:" << serviceName
                   << "object path:"  << objectPath;
    } else {
        retn = app.exec();
    }

    if (registeredService) {
        sessionBus.unregisterService(serviceName);
    }
    if (registeredObject) {
        sessionBus.unregisterObject(objectPath);
    }

    return retn;
}
