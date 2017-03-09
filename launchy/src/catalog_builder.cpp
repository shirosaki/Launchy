/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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
#include "catalog_types.h"
#include "catalog_builder.h"
#include "globals.h"
#include "main.h"
#include "Directory.h"

#include <QDebug>

CatalogAdder::CatalogAdder(Catalog& cat)
    : m_cat(cat), m_active(false), m_stop_requested(false), m_abort_requested(false) {

}

void CatalogAdder::run() {

    qDebug() << "Catalog adder started";
    m_active = true;

    int log_block = 0;

    while( m_active ) {

        QMutexLocker lock(&m_mutex);

        if ( m_abort_requested ) {
            m_queue.clear();
            break;
        }

        if ( !m_queue.empty() ) {
            m_cat.addItem(m_queue.front());
            m_queue.pop_front();

            if ( (log_block++ % 100) == 0 )
                qDebug() << "catalog size: " << m_cat.count() <<  " items left: " << m_queue.count();
        } else if ( m_stop_requested ) {
            break;
        }

        msleep(1);
    }

    qDebug() << "Catalog adder ended";
}

void CatalogAdder::push(QList<CatItem> items) {

    QMutexLocker lock(&m_mutex);

    if ( m_stop_requested || m_abort_requested )
        return;

    foreach(const CatItem & itm, items) {
        m_queue.push_back(itm);
    }
}

void CatalogAdder::finish() {
    QMutexLocker lock(&m_mutex);
    m_stop_requested = true;
}

void CatalogAdder::abort() {
    QMutexLocker lock(&m_mutex);
    m_abort_requested = true;
}

CatalogBuilder::CatalogBuilder(PluginHandler* plugs) :
    plugins(plugs),
    progress(CATALOG_PROGRESS_MAX),
    stop_request(false)

{
    catalog = new SlowCatalog();
}

void CatalogBuilder::run()
{
    buildCatalog();
}

void CatalogBuilder::stop()
{
    stop_request = true;
}

void CatalogBuilder::buildCatalog()
{
    qDebug() << "BUILD CATALOG STARTED";

    stop_request = false;

    progress = CATALOG_PROGRESS_MIN;
    emit catalogIncrement(progress);
    catalog->incrementTimestamp();
    indexed.clear();

    QList<Directory> memDirs = SettingsManager::readCatalogDirectories();
    QHash<uint, PluginInfo> pluginsInfo = plugins->getPlugins();
    totalItems = memDirs.count() + pluginsInfo.count();
    currentItem = 0;

    adder = QSharedPointer<CatalogAdder>(new CatalogAdder(*catalog));
    adder->start(QThread::LowPriority);

    while (currentItem < memDirs.count())
    {
        if ( stop_request ) {
            qWarning() << "Aborting catalog builder";
            break;
        }

        QString cur = platform->expandEnvironmentVars(memDirs[currentItem].name);
        indexDirectory(cur, memDirs[currentItem].types, memDirs[currentItem].indexDirs, memDirs[currentItem].indexExe, memDirs[currentItem].depth);
        progressStep(currentItem);
    }

    if ( stop_request ) {
        adder->abort();
    } else {
        adder->finish();
    }

    qDebug() << "wait catalog adder";
    adder->wait();
    qDebug() << "catalog adder joined";

    if ( !stop_request ) {
        // Don't call the pluginhandler to request catalog because we need to track progress
        plugins->getCatalogs(catalog, this);
    }

    catalog->purgeOldItems();
    indexed.clear();
    progress = CATALOG_PROGRESS_MAX;
    emit catalogFinished();

    qDebug() << "BUILD CATALOG COMPLETED";
}

void CatalogBuilder::indexDirectory(const QString& directory, const QStringList& filters, bool fdirs, bool fbin, int depth)
{
    if ( stop_request )
        return;

    QList<CatItem> partial_catalog;

    qDebug() << "Indexing directory \"" << directory << "\" with filters: " << filters.join(",");
    qDebug() << "Index folders: " << fdirs << " Index binaries: " << fbin << " Indexing depth: " << depth;
    qDebug() << "Item indexed: " << indexed.count();

    QString dir = QDir::toNativeSeparators(directory);
    QDir qd(dir);
    dir = qd.absolutePath();
    QStringList dirs = qd.entryList(QDir::AllDirs);

    if (depth > 0)
    {
        qDebug() << "Indexing " << dirs.count() << " subdirectories";
        for (int i = 0; i < dirs.count(); ++i)
        {
            if ( stop_request )
                return;

            if (!dirs[i].startsWith("."))
            {
                QString cur = dirs[i];
                if (!cur.contains(".lnk"))
                {
                    indexDirectory(dir + "/" + dirs[i], filters, fdirs, fbin, depth-1);
                }
            }
        }
    }

    if (fdirs)
    {
        qDebug() << "Processing " << dirs.count() << " subdirectories";
        for (int i = 0; i < dirs.count(); ++i)
        {
            if ( stop_request )
                return;

            if (!dirs[i].startsWith(".") && !indexed.contains(dir + "/" + dirs[i]))
            {
                bool isShortcut = dirs[i].endsWith(".lnk", Qt::CaseInsensitive);

                CatItem item(dir + "/" + dirs[i], !isShortcut);
                //catalog->addItem(item);
                partial_catalog.append(item);
                indexed.insert(dir + "/" + dirs[i]);
            }
        }
    }
    else
    {
        qDebug() << "Processing " << dirs.count() << " subdirectories ( shortcuts only )";

        // Grab any shortcut directories
        // This is to work around a QT weirdness that treats shortcuts to directories as actual directories
        for (int i = 0; i < dirs.count(); ++i)
        {
            if ( stop_request )
                return;

            if (!dirs[i].startsWith(".") && dirs[i].endsWith(".lnk", Qt::CaseInsensitive))
            {
                if (!indexed.contains(dir + "/" + dirs[i]))
                {
                    CatItem item(dir + "/" + dirs[i], true);
                    //catalog->addItem(item);
                    partial_catalog.append(item);
                    indexed.insert(dir + "/" + dirs[i]);
                }
            }
        }
    }

    if (fbin)
    {
        QStringList bins = qd.entryList(QDir::Files | QDir::Executable);

        qDebug() << "Indexing " << bins.count() << " executables";

        for (int i = 0; i < bins.count(); ++i)
        {
            if ( stop_request )
                return;

            if (!indexed.contains(dir + "/" + bins[i]))
            {
                CatItem item(dir + "/" + bins[i]);
                //catalog->addItem(item);
                partial_catalog.append(item);
                indexed.insert(dir + "/" + bins[i]);
            }
        }
    }

    // Don't want a null file filter, that matches everything..
    if (filters.count() > 0) {

        QStringList files = qd.entryList(filters, QDir::Files | QDir::System, QDir::Unsorted );

        qDebug() << "Indexing " << files.count() << " files";
        for (int i = 0; i < files.count(); ++i)
        {
            if ( stop_request )
                return;

            if (!indexed.contains(dir + "/" + files[i]))
            {
                CatItem item(dir + "/" + files[i]);
                platform->alterItem(&item);
                //catalog->addItem(item);
                partial_catalog.append(item);
                indexed.insert(dir + "/" + files[i]);
            }
        }
    }

    // add to the catalog
    if ( partial_catalog.count() > 0 ) {
        adder->push(partial_catalog);
    }
}


bool CatalogBuilder::progressStep(int newStep)
{
    newStep = newStep;

    ++currentItem;
    int newProgress = (int)(CATALOG_PROGRESS_MAX * (float)currentItem / totalItems);
    if (newProgress != progress)
    {
        progress = newProgress;
        emit catalogIncrement(progress);
    }
    return true;
}
