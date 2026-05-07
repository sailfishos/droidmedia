/*
 * SPDX-FileCopyrightText: 2025-2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QtDebug>
#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QList>

#include <policy/resource-set.h>
#include <policy/resources.h>
#include <droidmediacamera.h>

#include "flashlightservice.h"

FlashlightService::FlashlightService(QObject *parent)
    : QObject(parent),
      m_resourceSet(nullptr),
      m_wantedState(false), m_flashlightAvailable(true),
      m_cameraServiceTorchActive(false), m_droidMediaInitialized(droid_media_init())
{
    // Resource-policy reports availability asynchronously after D-Bus activation.
    // Allow the first request through and let acquire()/resourcesDenied() arbitrate.
    m_resourceSet = new ResourcePolicy::ResourceSet("background", this, true, true);
    m_resourceSet->addResource(ResourcePolicy::RearFlashlightType);
    connect(m_resourceSet, &ResourcePolicy::ResourceSet::resourcesBecameAvailable,
            this, &FlashlightService::resourcesAvailableHandler);
    connect(m_resourceSet, &ResourcePolicy::ResourceSet::resourcesGranted,
            this, &FlashlightService::resourcesAcquiredHandler);
    connect(m_resourceSet, &ResourcePolicy::ResourceSet::resourcesDenied,
            this, &FlashlightService::resourcesDeniedHandler);
    connect(m_resourceSet, &ResourcePolicy::ResourceSet::lostResources,
            this, &FlashlightService::resourcesLostHandler);
    m_resourceSet->initAndConnect();

    m_timer.setInterval(2 * 60 * 1000);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &FlashlightService::deactivate);
    m_timer.start(); // just in case the user never turns the flashlight on.
}

FlashlightService::~FlashlightService()
{
    cleanup();
}

void FlashlightService::deactivate()
{
    bool needEmit = isFlashlightOn();
    cleanup();
    if (needEmit) {
        emit flashlightOnChanged(false);
    }
    qApp->quit();
}

bool FlashlightService::isFlashlightOn() const
{
    return m_cameraServiceTorchActive;
}

void FlashlightService::cleanup()
{
    if (m_cameraServiceTorchActive) {
        setCameraServiceTorch(false);
    }
}

bool FlashlightService::setCameraServiceTorch(bool active)
{
    if (!m_droidMediaInitialized)
        return false;

    if (!droid_media_camera_set_torch_mode(active)) {
        if (!active)
            m_cameraServiceTorchActive = false;
        return false;
    }

    m_cameraServiceTorchActive = active;
    return true;
}

bool FlashlightService::toggleFlashlight()
{
    if (!m_resourceSet)
        return false;

    // If someone else has acquired the flashlight resource do not even
    // try to turn the flashlight on.
    if (!m_flashlightAvailable)
        return false;

    m_wantedState = !m_wantedState;

    if (isFlashlightOn() == m_wantedState)
        return false;

    if (m_wantedState) {
        // Acquire flashlight resource, and if/when granted
        // set flaslight on.
        m_resourceSet->acquire();
    } else {
        setFlashlight(false);
        m_resourceSet->release();
    }

    return true;
}

void FlashlightService::setFlashlight(bool active)
{
    if (active) {
        if (isFlashlightOn()) {
            qWarning() << "Flashlight was already on.";
            return;
        }

        if (setCameraServiceTorch(true)) {
            m_timer.stop(); // stop the inactivity timer while the flashlight is on
            emit flashlightOnChanged(true);
            return;
        }

        qWarning() << "failed to turn on flashlight";
        m_wantedState = false;
        if (m_resourceSet)
            m_resourceSet->release();
    } else {
        // the flashlight is on.  turn it off by destroying the active backend.
        cleanup();
        m_timer.start(); // start the inactivity timer
        emit flashlightOnChanged(false);
    }
}

void FlashlightService::resourcesAvailableHandler(const QList<ResourcePolicy::ResourceType> &available)
{
    m_flashlightAvailable = available.contains(ResourcePolicy::RearFlashlightType);
}

void FlashlightService::resourcesAcquiredHandler()
{
    setFlashlight(m_wantedState);
}

void FlashlightService::resourcesDeniedHandler()
{
    m_wantedState = false;
}

void FlashlightService::resourcesLostHandler()
{
    m_wantedState = false;
    setFlashlight(false);
}
