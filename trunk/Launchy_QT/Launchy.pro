TEMPLATE = subdirs
SUBDIRS = plugins/calcy \
          plugins/gcalc \
          plugins/runner \
          plugins/weby \
          plugins/verby \
          src
          
win32 {
	SUBDIRS += plugins/controly
}

TRANSLATIONS = translations/launchy_fr.ts \
    translations/launchy_nl.ts \
    translations/launchy_zh.ts \
    translations/launchy_es.ts \
    translations/launchy_de.ts \
    translations/launchy_ja.ts \
    translations/launchy_zh_TW.ts \
    translations/launchy_rus.ts \
    translations/launchy_it_IT.ts \
