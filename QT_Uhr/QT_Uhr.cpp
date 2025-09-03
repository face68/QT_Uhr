#include "QT_Uhr.h"
#include "ClockWidget.h"

#include <QWebEngineView>
#include <QVBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QCloseEvent>

QT_Uhr::QT_Uhr( QWidget* parent ) : QWidget( parent ) {

	setWindowFlags( Qt::FramelessWindowHint | Qt::Window );
	setAttribute( Qt::WA_TranslucentBackground );
	setAutoFillBackground( false );
	setWindowOpacity( 0.75 );

	auto* layout = new QVBoxLayout( this );
	layout->setContentsMargins( 0, 0, 0, 0 );
	layout->setSpacing( 0 );

	auto* clock = new ClockWidget( this );
	layout->addWidget( clock );

	resize( 300, 300 );
	applyMaskPercent();
}

void QT_Uhr::resizeEvent( QResizeEvent* ev ) {

	QWidget::resizeEvent( ev );
	applyMaskPercent();
}

void QT_Uhr::closeEvent( QCloseEvent* ev ) {
	QSettings s;
	s.setValue( "geom", saveGeometry() );
	ev->accept();
}

void QT_Uhr::applyMaskPercent() {

	int w = width();
	int h = height();

	int rw = w;      // volle Breite
	int rh = h;      // volle Höhe
	int x = 0;
	int y = 0;

	// Kreis aus der Mitte: nimm kleineren Radius
	if( w != h ) {
		int d = qMin( w, h );
		rw = d;
		rh = d;
		x = ( w - d ) / 2;
		y = ( h - d ) / 2;
	}

	setMask( QRegion( x, y, rw, rh, QRegion::Ellipse ) );
}


void QT_Uhr::mousePressEvent( QMouseEvent* e ) {
	if( e->button() == Qt::LeftButton ) {
		m_dragging = true;
		m_dragOffset = e->globalPos() - frameGeometry().topLeft();
	}
}

void QT_Uhr::mouseMoveEvent( QMouseEvent* e ) {
	if( m_dragging && ( e->buttons() & Qt::LeftButton ) ) {
		move( e->globalPos() - m_dragOffset );
	}
}

void QT_Uhr::mouseReleaseEvent( QMouseEvent* e ) {
	if( e->button() == Qt::LeftButton ) {
		m_dragging = false;
	}
}