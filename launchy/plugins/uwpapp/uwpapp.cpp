/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <QtGui>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>

#ifdef Q_WS_WIN
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

#endif

#include <VersionHelpers.h>
#include "uwpapp.h"

uwpappPlugin* guwpappInstance = NULL;

void uwpappPlugin::getID(uint* id)
{
	*id = HASH_uwpapp;
}

void uwpappPlugin::getName(QString* str)
{
	*str = PLUGIN_NAME;
}

void uwpappPlugin::init()
{
}


void uwpappPlugin::getLabels(QList<InputData>* id)
{
}

void uwpappPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
}


QString uwpappPlugin::getIcon()
{
	return QString();
}

void uwpappPlugin::getCatalog(QList<CatItem>* items)
{
	if (!IsWindows8OrGreater())
	{
		return;
	}

	Package package(HASH_uwpapp, items, getIcon());
	package.findPackages();
}

void uwpappPlugin::launchItem(QList<InputData>* id, CatItem* item)
{
	// Specify the appropriate COM threading model
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	IApplicationActivationManager* paam = NULL;
	HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&paam));
	if (FAILED(hr))
	{
		qDebug() << "Error creating CoCreateINstance & HR is" << hr;
		return;
	}

	DWORD pid = 0;
	hr = paam->ActivateApplication(item->fullPath.toStdWString().c_str(), L"", AO_NONE, &pid);
	if (FAILED(hr))
	{
		qDebug() << "Error in ActivateApplication call & HR is " << hr;
		return;
	}

	if (hr == 0)
		qDebug() << "Activated  " << item->fullPath << " with pid " << pid;

	CoUninitialize();
}

void uwpappPlugin::extractIcon(CatItem* item, QIcon* icon)
{
	QColor background;
	QStringList iconPaths = item->icon.split('\t');
	QString backgroundColor;

	if (iconPaths.size() > 1)
	{
		backgroundColor = iconPaths[1];
		qDebug() << "background color: " << backgroundColor;
	}

	if (backgroundColor.isEmpty() || backgroundColor == "transparent")
	{
		// Get theme color for background.
		DWORD colorizationColor;
		DWORD size = sizeof(DWORD);

		RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", L"ColorizationColor", RRF_RT_REG_DWORD, 0, &colorizationColor, &size);

		BYTE r = (colorizationColor >> 16) & 0xFF;
		BYTE g = (colorizationColor >> 8) & 0xFF;
		BYTE b = colorizationColor & 0xFF;
		background.setRgb(r, g, b);
	}
	else if (backgroundColor.startsWith("#"))
	{
		background.setNamedColor(backgroundColor);
	}

	// Create image with background color
	QString path = iconPaths[0];
	QPixmap iconPixmap = QPixmap(path);
	// Compose proper image with icon alpha channel
	QImage iconImage = QImage(iconPixmap.size(), QImage::Format_ARGB32);
	iconImage.fill(background);
	QPainter painter(&iconImage);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.drawPixmap(0, 0, iconPixmap);
	icon->addPixmap(QPixmap::fromImage(iconImage));
}

void uwpappPlugin::doDialog(QWidget* parent, QWidget** newDlg)
{
}

void uwpappPlugin::endDialog(bool accept)
{
}

int uwpappPlugin::msg(int msgId, void* wParam, void* lParam)
{
	int handled = 0;
	switch (msgId)
	{
		case MSG_INIT:
			init();
			handled = 1;
			break;
		case MSG_GET_LABELS:
			getLabels((QList<InputData>*) wParam);
			handled = 1;
			break;
		case MSG_GET_ID:
			getID((uint*) wParam);
			handled = 1;
			break;
		case MSG_GET_NAME:
			getName((QString*) wParam);
			handled = 1;
			break;
		case MSG_GET_RESULTS:
			getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
			handled = 1;
			break;
		case MSG_GET_CATALOG:
			getCatalog((QList<CatItem>*) wParam);
			handled = 1;
			break;
		case MSG_LAUNCH_ITEM:
			launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
			handled = 1;
			break;
		case MSG_EXTRACT_ICON:
			extractIcon((CatItem*)wParam, (QIcon*)lParam);
			handled = 1;
			break;
		case MSG_HAS_DIALOG:
			// Set to true if you provide a gui
			handled = 1;
			break;
		case MSG_DO_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			doDialog((QWidget*) wParam, (QWidget**) lParam);
			break;
		case MSG_END_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			endDialog((bool) wParam);
			break;

		default:
			break;
	}

	return handled;
}
