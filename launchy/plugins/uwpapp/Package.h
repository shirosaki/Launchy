#pragma once

#include <iostream>
#include <collection.h>
#include <string>
#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <AppxPackaging.h>
#include <Shobjidl.h>
#include <Dwmapi.h>

#include "plugin_interface.h"
#include "Application.h"

class Package
{
public:
	Package();
	~Package();

	Package(uint pluginId, QList<CatItem>* items, QString defaultIconPath);

	QString name;
	QString fullName;
	QString installedLocation;
	QString defaultIconPath;
	QString imagePath;
	QString savedIconPath;
	QVector<Application> apps;

	void Package::setImagePath(QString path);
	int Package::findPackages();
	void Package::packageInfo(Windows::ApplicationModel::Package^ package);
	HRESULT Package::getManifestReader(_In_ LPCWSTR manifestFilePath, _Outptr_ IAppxManifestReader** reader);
	HRESULT Package::readManifest(LPCWSTR manifestFilePath);
	HRESULT Package::readManifestApplications(_In_ IAppxManifestReader* manifestReader);

private:
	uint pluginId;
	QList<CatItem>* items;
};

