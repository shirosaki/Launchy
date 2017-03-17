#include "precompiled.h"
#include "Package.h"

using namespace Windows::Management::Deployment;

//
// Workaround to fix build errors.
// https://forum.qt.io/topic/68670/how-can-i-use-winrt-c-apis-in-at-qt-winrt-app/9
//
HRESULT __stdcall GetActivationFactoryByPCWSTR(void*, ::Platform::Guid&, void**);
namespace __winRT
{
	HRESULT __stdcall __getActivationFactoryByPCWSTR(const void* str, ::Platform::Guid& pGuid, void** ppActivationFactory)
	{
		return GetActivationFactoryByPCWSTR(const_cast<void*>(str), pGuid, ppActivationFactory);
	}
}

Package::Package()
{
}

Package::~Package()
{
}

Package::Package(uint pluginId, QList<CatItem>* items, QString defaultIconPath)
{
	this->pluginId = pluginId;
	this->items = items;
	this->defaultIconPath = defaultIconPath;
}

void Package::setImagePath(QString path)
{
	imagePath = path;

	if (!QFile::exists(imagePath))
	{
		QDir().mkpath(imagePath);
	}
}

int Package::findPackages()
{
	// Specify the appropriate COM threading model
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	PackageManager^ packageManager = ref new PackageManager();

	Windows::Foundation::Collections::IIterable<Windows::ApplicationModel::Package^>^ packages = packageManager->FindPackagesForUser(nullptr);

	int packageCount = 0;
	std::for_each(Windows::Foundation::Collections::begin(packages), Windows::Foundation::Collections::end(packages),
		[&](Windows::ApplicationModel::Package^ package)
	{
		packageInfo(package);

		QString prefix = QString("ms-resource:");
		QVectorIterator<Application> i(apps);
		while (i.hasNext())
		{
			Application app = i.next();
			if (!app.displayName.isEmpty() && app.displayName.startsWith(prefix))
			{
				QString key = app.displayName.mid(prefix.size());
				QString parsed(prefix);
				if (key.startsWith("//"))
				{
					parsed += key;
				}
				else if (key.startsWith("/"))
				{
					parsed += "//";
					parsed += key;
				}
				else if (key.contains("/"))
				{
					parsed += key;
				}
				else
				{
					parsed += "///Resources/";
					parsed += key;
				}
				QString query("@{");
				query += fullName;
				query += "? ";
				query += parsed;
				query += "}";
				qDebug() << "Query: " << query;
				wchar_t outBuffer[128];
				HRESULT hr = SHLoadIndirectString(query.toStdWString().c_str(), outBuffer, _countof(outBuffer), NULL);
				if (SUCCEEDED(hr))
				{
					app.displayName = QString::fromWCharArray(outBuffer);
				}
				else {
					app.displayName = QString::fromWCharArray(package->Id->Name->Data());
				}
			}
			if (app.displayName.isEmpty())
			{
				app.displayName = QString::fromWCharArray(package->Id->Name->Data());
			}

			if (!app.logo.isEmpty())
			{
				QString query("@{");
				query += fullName;
				query += "? ";
				query += prefix + "//" + name + "/Files/" + app.logo;
				query += "}";
				qDebug() << "Query: " << query;
				wchar_t outBuffer[256];
				HRESULT hr = SHLoadIndirectString(query.toStdWString().c_str(), outBuffer, _countof(outBuffer), NULL);
				if (SUCCEEDED(hr)) {
					app.iconPath = QString::fromWCharArray(outBuffer);
					qDebug() << QString("Icon Path: ") << app.iconPath;
				}
			}

			QString path = QString(app.iconPath);
			if (!app.backgroundColor.isEmpty())
			{
				path += "\t";
				path += app.backgroundColor;
			}
			items->prepend(CatItem(app.userModelId, app.displayName, pluginId, path));
			packageCount += 1;
		}
	});

	if (packageCount < 1)
	{
		qDebug() << "No packages were found.";
	}
	else {
		qDebug() << "Packages count: " << packageCount;
	}

	return 0;
}

//
// Gets package informations.
//
void Package::packageInfo(Windows::ApplicationModel::Package^ package)
{
	if (package->IsFramework) {
		return;
	}
	name = QString::fromWCharArray(package->Id->Name->Data());
	fullName = QString::fromWCharArray(package->Id->FullName->Data());

	installedLocation = QString::fromWCharArray(package->InstalledLocation->Path->Data());
	std::wstring path = installedLocation.toStdWString();
	path += L"\\AppxManifest.xml";

	apps.clear();
	readManifest(path.c_str());
}

//
// Reads an app package manifest.
//
// Parameters:
//   manifestFilePath
//     Manifest file path of a application to be read.
//
HRESULT Package::readManifest(LPCWSTR manifestFilePath)
{
	HRESULT hr = S_OK;

	IAppxManifestReader* manifestReader = NULL;
	hr = getManifestReader(manifestFilePath, &manifestReader);

	if (SUCCEEDED(hr))
	{
		hr = readManifestApplications(manifestReader);
	}
	if (manifestReader != NULL)
	{
		manifestReader->Release();
		manifestReader = NULL;
	}
	return hr;
}


//
// Creates an app package reader.
//
// Parameters:
//   manifestFilePath
//     Manifest file path of a application to be read.
//   reader
//     On success, receives the created instance of IAppxPackageReader.
//
HRESULT Package::getManifestReader(
	_In_ LPCWSTR manifestFilePath,
	_Outptr_ IAppxManifestReader** reader)
{
	HRESULT hr = S_OK;
	IAppxFactory* appxFactory = NULL;
	IStream* inputStream = NULL;

	// Create a new factory
	hr = CoCreateInstance(
		__uuidof(AppxFactory),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IAppxFactory),
		(LPVOID*)(&appxFactory));

	// Create a stream over the input app package
	if (SUCCEEDED(hr))
	{
		hr = SHCreateStreamOnFileEx(
			manifestFilePath,
			STGM_READ | STGM_SHARE_EXCLUSIVE,
			0, // default file attributes
			FALSE, // do not create new file
			NULL, // no template
			&inputStream);
	}

	// Create a new package reader using the factory.  For
	// simplicity, we don't verify the digital signature of the package.
	if (SUCCEEDED(hr))
	{
		hr = appxFactory->CreateManifestReader(
			inputStream,
			reader);
	}

	// Clean up allocated resources
	if (inputStream != NULL)
	{
		inputStream->Release();
		inputStream = NULL;
	}
	if (appxFactory != NULL)
	{
		appxFactory->Release();
		appxFactory = NULL;
	}
	return hr;
}


//
// Reads a subset of the manifest Applications element.
//
//
HRESULT Package::readManifestApplications(
	_In_ IAppxManifestReader* manifestReader)
{
	HRESULT hr = S_OK;
	BOOL hasCurrent = FALSE;
	UINT32 applicationsCount = 0;

	IAppxManifestApplicationsEnumerator* applications = NULL;

	// Get elements and attributes from the manifest reader
	hr = manifestReader->GetApplications(&applications);
	if (SUCCEEDED(hr))
	{
		hr = applications->GetHasCurrent(&hasCurrent);

		while (SUCCEEDED(hr) && hasCurrent)
		{
			IAppxManifestApplication* application = NULL;
			Application app;
			LPWSTR displayName = NULL;
			LPWSTR logo = NULL;
			LPWSTR backgroundColor = NULL;

			hr = applications->GetCurrent(&application);
			LPWSTR userModelId = NULL;
			hr = application->GetAppUserModelId(&userModelId);
			if (SUCCEEDED(hr))
			{
				qDebug() << "UserModelId: " << userModelId;
				if (userModelId)
					app.userModelId = QString::fromWCharArray(userModelId);
			}

			hr = application->GetStringValue(L"DisplayName", &displayName);

			if (SUCCEEDED(hr))
			{
				qDebug() << "Package display name: " << displayName;
				if (displayName)
					app.displayName = QString::fromWCharArray(displayName);
			}

			hr = application->GetStringValue(L"Square44x44Logo", &logo);

			if (SUCCEEDED(hr))
			{
				qDebug() << "Logo file name: " << logo;
				if (logo)
					app.logo = QString::fromWCharArray(logo);
			}

			hr = application->GetStringValue(L"BackgroundColor", &backgroundColor);

			if (SUCCEEDED(hr))
			{
				qDebug() << "Background Color: " << backgroundColor;
				if (backgroundColor)
					app.backgroundColor = QString::fromWCharArray(backgroundColor);
			}

			if (!app.userModelId.isEmpty())
			{
				apps.append(app);
				applicationsCount++;
			}

			hr = applications->MoveNext(&hasCurrent);

			if (application != NULL)
			{
				application->Release();
				application = NULL;
			}
			// Free all string buffers returned from the manifest API
			CoTaskMemFree(userModelId);
			CoTaskMemFree(displayName);
			CoTaskMemFree(logo);
			CoTaskMemFree(backgroundColor);
		}

		qDebug() << "Count of apps in the package: " << applicationsCount;
	}

	// Clean up allocated resources
	if (applications != NULL)
	{
		applications->Release();
		applications = NULL;
	}

	return hr;
}
