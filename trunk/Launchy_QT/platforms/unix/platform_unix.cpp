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

#include "precompiled.h"
#include <QtGui>
#include <QApplication>
#include <QX11Info>
#include "platform_unix.h"
#include <boost/pointer_cast.hpp>  
#include <X11/Xlib.h>
#include <QFileIconProvider>





PlatformUnix::PlatformUnix(int& argc, char** argv) :
        PlatformBase(argc, argv)
{
    /*
        instance = new LimitSingleInstance(TEXT("Local\\{ASDSAD0-DCC6-49b5-9C61-ASDSADIIIJJL}"));

        // Create local and global application mutexes so that installer knows when
        // Launchy is running
        localMutex = CreateMutex(NULL,0,_T("LaunchyMutex"));
        globalMutex = CreateMutex(NULL,0,_T("Global\\LaunchyMutex"));
        */
    icons = new UnixIconProvider();

}

PlatformUnix::~PlatformUnix()
{ 
    GlobalShortcutManager::clear();
}

QList<Directory> PlatformUnix::getDefaultCatalogDirectories() {
    QList<Directory> list;
    const char *dirs[] = { "/usr/share/applications/",
			   "/usr/local/share/applications/",
			   "/usr/share/gdm/applications/",
			   "/usr/share/applications/kde/",
			   "~/.local/share/applications/"};
    QStringList l;
    l << "*.desktop";
    
    for(int i = 0; i < 5; i++)
	list.append(Directory(dirs[i],l,false,false,100));

    list.append(Directory("~",QStringList(),true,false,0));
    
    return list;
}


QHash<QString, QList<QString> > PlatformUnix::getDirectories() {
    QHash<QString, QList<QString> > out;
    QDir d;

    QString xdg_config_home = qgetenv("XDG_CONFIG_HOME").constData();
    if (xdg_config_home.isEmpty()) {
        xdg_config_home = QDir::homePath() + "/.config/launchy";
    }

    d.mkdir(xdg_config_home);
    
    out["skins"] += qApp->applicationDirPath() + "/skins";
    out["skins"] += xdg_config_home + "/skins";
    out["skins"] += SKINS_PATH;

    out["plugins"] += qApp->applicationDirPath() + "/plugins";
    out["plugins"] += xdg_config_home + "/plugins";
    out["plugins"] += PLUGINS_PATH;

    out["config"] += xdg_config_home;
    out["portableConfig"] += qApp->applicationDirPath();
    
    if (QFile::exists(out["skins"].last() + "/Default"))
        out["defSkin"] += out["skins"].last() + "/Default";
    else
        out["defSkin"] += out["skins"].first() + "/Default";

    out["platforms"] += qApp->applicationDirPath();
    out["platforms"] += PLATFORMS_PATH;

    return out;
}


bool PlatformUnix::supportsAlphaBorder() const
{
    return QX11Info::isCompositingManagerRunning();
}


void PlatformUnix::alterItem(CatItem* item) {
    if (!item->fullPath.endsWith(".desktop", Qt::CaseInsensitive))
	return;

    QString locale = QLocale::system().name();
    
    

    QFile file(item->fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	return;

    QString name = "";
    QString icon = "";
    QString exe = "";
    while(!file.atEnd()) {
	QString line = QString::fromUtf8(file.readLine());
	
	if (name == "" && line.startsWith("Name[" + locale, Qt::CaseInsensitive))
	    name = line.split("=")[1].trimmed();
	else if (name == "" && line.startsWith("Name=", Qt::CaseInsensitive))
	    name = line.split("=")[1].trimmed();
	else if (icon == "" && line.startsWith("Icon", Qt::CaseInsensitive))
	    icon = line.split("=")[1].trimmed();
	else if (exe == "" && line.startsWith("Exec", Qt::CaseInsensitive))
	    exe = line.split("=")[1].trimmed();	
    }
    if (name.size() >= item->shortName.size() - 8) {
	item->shortName = name;
	item->lowName = item->shortName.toLower();
    }

    // Don't index desktop items wthout icons
    if (icon.trimmed() == "")
        return;


    /* fill in some specifiers while we have the info */
    exe.replace("%i", "--icon " + icon);
    exe.replace("%c", name);
    exe.replace("%k", item->fullPath);

    QStringList allExe = exe.trimmed().split(" ",QString::SkipEmptyParts);
    if (allExe.size() == 0 || allExe[0].size() == 0 )
            return;
    exe = allExe[0];
    allExe.removeFirst();

    
    /* if an absolute or relative path is supplied we can just skip this
       everything else should be checked to avoid picking up [unwanted]
       stuff from the working directory - if it doesnt exsist, use it anyway */
    if(!exe.contains(QRegExp("^.?.?/"))){
        foreach(QString line, QProcess::systemEnvironment()) {
            if (!line.startsWith("Path", Qt::CaseInsensitive))
                continue;

            QStringList spl = line.split("=");
            QStringList spl2 = spl[1].split(":");
            foreach(QString dir, spl2) {
                QString tmp = dir + "/" + exe;
                if (QFile::exists(tmp)) {
                    exe = tmp;
                    break;
                }
            }
            break;
        }
    }
    
    
    item->fullPath = exe + " " + allExe.join(" ");

    if (!QFile::exists(icon)) {
        icon = ((UnixIconProvider*)icons)->getDesktopIcon(file.fileName(), icon);
    }


    QFileInfo inf(icon);
    if (!inf.exists()) {
        qDebug() << "couldn't find icon for" << icon << item->fullPath;
        return;
    }

    item->icon = icon;

    file.close();
    return;
}


QString PlatformUnix::expandEnvironmentVars(QString txt)
{
	QStringList list = QProcess::systemEnvironment();
	txt.replace('~', "$HOME$");
	QString delim("$");
	QString out = "";
	int curPos = txt.indexOf(delim, 0);
	if (curPos == -1) return txt;

	while(curPos != -1)
	{
		int nextPos = txt.indexOf("$", curPos+1);
		if (nextPos == -1) 
		{
			out += txt.mid(curPos+1);
			break;
		}
		QString var = txt.mid(curPos+1, nextPos-curPos-1);
		bool found = false;
		foreach(QString s, list)
		{
			if (s.startsWith(var, Qt::CaseInsensitive))
			{
				found = true;
				out += s.mid(var.length()+1);
				break;
			}			
		}
		if (!found)
			out += "$" + var;
		curPos = nextPos;
	}
	return out;
}


// Create the application object
QApplication* createApplication(int& argc, char** argv)
{
        return new PlatformUnix(argc, argv);
}
