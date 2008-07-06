#pragma once
#include <QtGui>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>


#include <QFileIconProvider>

#include <QWidget>

class AlphaBorder : public QWidget
{
    Q_OBJECT    
private:
    public:
QString alphaFile;

 AlphaBorder(QWidget * parent, QString);
 ~AlphaBorder();
 QWidget* p;
    void paintEvent(QPaintEvent *);
    void SetAlphaOpacity(double trans);
    //    QPaintEngine * paintEngine() { return 0; }
    //    void mousePressEvent(QMouseEvent *event);
    GC			gc;	// Target Painter
    GC                  gcback;
    XImage 		*xmask;	// Image informations
    XImage              *buffer;
    XGCValues		values;	// Setup GC
    uint 		width;
    uint 		height;
};

