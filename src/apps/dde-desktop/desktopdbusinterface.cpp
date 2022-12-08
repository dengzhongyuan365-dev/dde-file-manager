/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "desktopdbusinterface.h"

#include <QDBusInterface>
#include <QDBusPendingCall>

using namespace dde_desktop;

DesktopDBusInterface::DesktopDBusInterface(QObject *parent) : QObject(parent)
{

}

void DesktopDBusInterface::Refresh(bool silent)
{
    QDBusInterface ifs(kDesktopServiceName,
                       "/org/deepin/dde/desktop/canvas",
                       "org.deepin.dde.desktop.canvas");
    ifs.asyncCall("Refresh", silent);
}

void DesktopDBusInterface::ShowWallpaperChooser(const QString &screen)
{
    QDBusInterface ifs(kDesktopServiceName,
                       "/org/deepin/dde/desktop/wallpapersettings",
                       "org.deepin.dde.desktop.wallpapersettings");
    ifs.asyncCall("ShowWallpaperChooser", screen);
}

void DesktopDBusInterface::ShowScreensaverChooser(const QString &screen)
{
    QDBusInterface ifs(kDesktopServiceName,
                       "/org/deepin/dde/desktop/wallpapersettings",
                       "org.deepin.dde.desktop.wallpapersettings");
    ifs.asyncCall("ShowScreensaverChooser", screen);
}
