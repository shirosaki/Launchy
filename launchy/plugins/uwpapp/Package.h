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
	QString namespaces;
	QVector<Application> apps;

	int Package::findPackages();
	void Package::packageInfo(Windows::ApplicationModel::Package^ package);
	HRESULT Package::getManifestReader(_In_ LPCWSTR manifestFilePath, _Outptr_ IAppxManifestReader** reader);
	HRESULT Package::readManifest(LPCWSTR manifestFilePath);
	void getXmlNamespaces(LPCWSTR path);
	HRESULT Package::readManifestApplications(_In_ IAppxManifestReader* manifestReader);
	std::wstring getLogoKey();

private:
	uint pluginId;
	QList<CatItem>* items;
};

