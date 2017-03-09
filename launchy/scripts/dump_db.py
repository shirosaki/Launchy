#!/usr/bin/python

import os
import sys
from PyQt5 import QtCore

"""
class CatItem {
public:
    
	/** The full path of the indexed item */
	QString fullPath;
	/** The abbreviated name of the indexed item */
	QString shortName;
	/** The lowercase name of the indexed item */
	QString lowName;
	/** A path to an icon for the item */
	QString icon;
	/** How many times this item has been called by the user */
	int usage;
	/** This is unused, and meant for plugin writers and future extensions */
	void* data;
	/** The plugin id of the creator of this CatItem */
	int id;
    
	out << item.fullPath;
	out << item.shortName;
	out << item.lowName;
	out << item.icon;
	out << item.usage;
	out << item.id;
        
"""

class CatalogItem(object):
    def __init__(self):
        self.full_path = b""
        self.short_name = b""
        self.low_name = b""
        self.icon = b""
        self.usage = 0
        self.cid = 0
        
    def read(self, ds):
    
        self.full_path = ds.readQString()
        self.short_name = ds.readQString()
        self.low_name = ds.readQString()
        self.icon = ds.readQString()
        self.usage = ds.readInt32()
        self.cid = ds.readInt32()


class Catalog(object):
    def __init__(self, dbpath=""):
        self.db_name = "launchy.db"
        self.db_path = dbpath
        self.items = []
        self.paths = {}
        self.dups = []
        
    def process(self):
        full_path = os.path.join(self.db_path, self.db_name)
        with open(full_path, 'rb') as fp:
            data_ = fp.read()
            uncompressed_ = QtCore.qUncompress(data_)
            with open(full_path + ".uncompressed", "wb") as ufp:
                ufp.write(uncompressed_)
    
            self._process(full_path + ".uncompressed")
        
    def _process(self, fn):
        with open(fn, "rb") as fp:
            data_ = QtCore.QByteArray(fp.read())
            ds = QtCore.QDataStream(data_)
            ds.setVersion(QtCore.QDataStream.Qt_4_2)
            
            while not ds.atEnd():
                item = CatalogItem()
                item.read(ds)
                
                self.items.append(item)
                if item.full_path not in self.paths:
                    self.paths[item.full_path] = 0
                else:
                    self.paths[item.full_path] += 1
                    self.dups.append(item)
            
    def dump(self):
        for i in self.items:
            print(i.full_path.encode('utf-8'))
            
    def print_dups(self):
        for i in self.dups:
            print(i.full_path.encode('utf-8'))
    
if __name__ == '__main__':
    if len(sys.argv) > 1:
        where = sys.argv[1]
        catalog = Catalog(where)
        catalog.process()
        catalog.dump()