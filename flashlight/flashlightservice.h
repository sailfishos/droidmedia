/*
 * SPDX-FileCopyrightText: 2025-2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FLASHLIGHTSERVICE_H
#define FLASHLIGHTSERVICE_H

#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtDBus/QDBusContext>
#include <QList>

#include <policy/resource-set.h>
#include <policy/resource.h>

class FlashlightService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfish.flashlight.provider")
    Q_PROPERTY(bool flashlightOn READ isFlashlightOn NOTIFY flashlightOnChanged)

public:
    FlashlightService(QObject *parent = 0);
    ~FlashlightService();

    bool isFlashlightOn() const;
    Q_INVOKABLE bool toggleFlashlight();

Q_SIGNALS:
    void flashlightOnChanged(bool newOn);

private Q_SLOTS:
    void deactivate();
    void resourcesAvailableHandler(const QList<ResourcePolicy::ResourceType> &available);
    void resourcesAcquiredHandler();
    void resourcesDeniedHandler();
    void resourcesLostHandler();

private:
    void setFlashlight(bool active);
    bool setCameraServiceTorch(bool active);
    void cleanup();

    QTimer m_timer;
    ResourcePolicy::ResourceSet *m_resourceSet;
    bool m_wantedState;
    bool m_flashlightAvailable;
    bool m_cameraServiceTorchActive;
    bool m_droidMediaInitialized;
};

#endif // FLASHLIGHTSERVICE_H
