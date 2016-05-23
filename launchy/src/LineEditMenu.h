#ifndef LINEEDITMENU_H
#define LINEEDITMENU_H


#if QT_VERSION >= 0x050000
#   include <QtWidgets/QLabel>
#else
#   include <QtGui/QLabel>
#endif

class LineEditMenu : public QLabel
{
	Q_OBJECT

public:
	LineEditMenu(QWidget* parent = 0);
	void contextMenuEvent(QContextMenuEvent *event);

signals:
	void menuEvent(QContextMenuEvent*);
};


#endif
