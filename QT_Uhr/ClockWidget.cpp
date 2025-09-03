#include "ClockWidget.h"
#include <QPainter>
#include <QTime>
#include <QtMath>
#include <QVector2D>
#include <QPainterPath>
#include <QSettings>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QActionGroup>

static const QVector2D HOUR_HAND[] = {
	QVector2D( -0.05f,  0.12f ),
	QVector2D( 0.05f,  0.12f ),
	QVector2D( 0.05f, -0.55f ),
	QVector2D( -0.05f,  -0.55f )
};

static const int HOUR_HAND_COUNT = sizeof( HOUR_HAND ) / sizeof( HOUR_HAND[ 0 ] );

static const QVector2D MIN_HAND[] = {
	QVector2D( -0.03f,  0.25f ),
	QVector2D( 0.03f,  0.25f ),
	QVector2D( 0.03f, -0.85f ),
	QVector2D( -0.03f,  -0.85f )
};

static const int MIN_HAND_COUNT = sizeof( MIN_HAND ) / sizeof( MIN_HAND[ 0 ] );

static const QVector2D SEC_HAND[] = {
	QVector2D( -0.05f,  0.350f ),
	QVector2D( 0.05f,  0.350f ),
	QVector2D( 0.04f,  0.120f ),
	QVector2D( 0.020f, 0.100f ),
	QVector2D( 0.01f, -0.950f ), // Spitze
	QVector2D( -0.01f, -0.950f ), // Spitze
	QVector2D( -0.020f, 0.100f ),
	QVector2D( -0.04f,  0.120f )
};

static const int SEC_HAND_COUNT = sizeof( SEC_HAND ) / sizeof( SEC_HAND[ 0 ] );

static const QVector2D TICK_1MIN[] = {
	QVector2D( -0.015f, -1.00f ),
	QVector2D( 0.015f, -1.00f ),
	QVector2D( 0.015f, -0.92f ),
	QVector2D( -0.015f, -0.92f )
};

static const int TICK_1MIN_COUNT = sizeof( TICK_1MIN ) / sizeof( TICK_1MIN[ 0 ] );

static const QVector2D TICK_5MIN[] = {
	QVector2D( -0.020f, -1.00f ),
	QVector2D( 0.020f, -1.00f ),
	QVector2D( 0.020f, -0.8f ),
	QVector2D( -0.020f, -0.8f )
};

static const int TICK_5MIN_COUNT = sizeof( TICK_5MIN ) / sizeof( TICK_5MIN[ 0 ] );

static const QVector2D TICK_3H[] = {
	QVector2D( -0.035f, -1.00f ),
	QVector2D( 0.035f, -1.00f ),
	QVector2D( 0.035f, -0.70f ),
	QVector2D( -0.035f, -0.70f )
};

static const int TICK_3H_COUNT = sizeof( TICK_3H ) / sizeof( TICK_3H[ 0 ] );



static inline QPointF rotScaleTranslate( const QVector2D& v, float scale, float angleDeg, const QPointF& center ) {
	const float r = qDegreesToRadians( angleDeg );
	const float c = qCos( r );
	const float s = qSin( r );

	const float x = v.x() * scale;
	const float y = v.y() * scale;

	const float xr = c * x - s * y;
	const float yr = s * x + c * y;

	return QPointF( center.x() + xr, center.y() + yr );
}

static inline QPolygonF makePoly( const QVector2D* pts, int count, float scale, float angleDeg, const QPointF& center ) {
	QPolygonF poly;
	poly.reserve( count );

	for( int i = 0; i < count; ++i ) {
		poly << rotScaleTranslate( pts[ i ], scale, angleDeg, center );
	}

	return poly;
}

struct Mat2x3 {
	float m00, m01, m02;
	float m10, m11, m12;
};

static QVector2D mul( const Mat2x3& M, const QVector2D& v ) {

	return QVector2D( M.m00 * v.x() + M.m01 * v.y() + M.m02, M.m10 * v.x() + M.m11 * v.y() + M.m12 );
}

static Mat2x3 makeRT( float deg, float tx, float ty ) {

	float r = deg * M_PI / 180.0f;
	float c = cosf( r );
	float s = sinf( r );

	Mat2x3 M{ c, -s, tx, s,  c, ty };
	return M;
}



ClockWidget::ClockWidget( QWidget* parent ) : QWidget( parent ) {

	setAttribute( Qt::WA_TranslucentBackground );
	setAutoFillBackground( false );

	initUiAndTray();

	timer.setInterval( 100 );
	connect( &timer, &QTimer::timeout, this, QOverload<>::of( &ClockWidget::update ) );
	timer.start();
}

void ClockWidget::paintEvent( QPaintEvent* e ) {

	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing, true );
	int w = width(), h = height(), d = qMin( w, h );
	int x = ( w - d ) / 2, y = ( h - d ) / 2;

	QPainterPath clip;
	clip.addEllipse( QRectF( x, y, d, d ) );
	painter.setClipPath( clip );

	painter.setPen( Qt::NoPen );
	if( m_inverse ) {
		painter.setBrush( Qt::white );
	}
	else {
		painter.setBrush( Qt::black );
	}

	painter.drawEllipse( QRectF( x, y, d, d ) );

	const float radius = 0.5f * qMin( width(), height() );
	const QPointF C( width() * 0.5, height() * 0.5 );

	if( m_inverse ) {
		painter.setBrush( Qt::black );
		painter.setPen( Qt::black );
	}
	else {
		painter.setBrush( Qt::white );
		painter.setPen( Qt::white );
	}

	// ticks
	for( int i = 0; i < 60; ++i ) {
		const float a = i * 6.0f;
		const bool is5 = ( i % 5 ) == 0;
		const bool is3h = ( i % 15 ) == 0;
		const QPolygonF poly = is3h
			? makePoly( TICK_3H, TICK_3H_COUNT, radius, a, C )
			: is5
			? makePoly( TICK_5MIN, TICK_5MIN_COUNT, radius, a, C )
			: makePoly( TICK_1MIN, TICK_1MIN_COUNT, radius, a, C );
		painter.drawPolygon( poly );
	}

	// hands
	const QTime t = QTime::currentTime();
	const double s = t.second() + t.msec() / 1000.0;
	const double angS = 6.0 * s;
	const float angM = 6.0f * t.minute() + 0.1f * t.second();
	const float angH = 30.0f * ( t.hour() % 12 ) + 0.5f * t.minute();

	painter.setPen( Qt::NoPen );
	painter.drawPolygon( makePoly( HOUR_HAND, HOUR_HAND_COUNT, radius, angH, C ) );
	painter.drawPolygon( makePoly( MIN_HAND, MIN_HAND_COUNT, radius, angM, C ) );

	painter.setBrush( QColor( 206, 63, 48 ) );

	painter.drawPolygon( makePoly( SEC_HAND, SEC_HAND_COUNT, radius, angS, C ) );
	painter.drawEllipse( QPointF( width() * 0.5, height() * 0.5 ), width() * 0.02, height() * 0.02 );

	painter.setBrush( QColor( 50, 50, 50 ) );
	painter.drawEllipse( QPointF( width() * 0.5, height() * 0.5 ), width() * 0.0075, height() * 0.0075 );
}

void ClockWidget::resizeEvent( QResizeEvent* ev ) {

	QWidget::resizeEvent( ev );
}

void ClockWidget::closeEvent( QCloseEvent* ev ) {

	QSettings s;
	s.setValue( "pos", pos() );
	ev->accept();
}

void ClockWidget::mousePressEvent( QMouseEvent* e ) {

	if( e->button() == Qt::MouseButton::LeftButton ) {
		m_dragging = true;
		m_dragOffset = e->globalPos() - frameGeometry().topLeft();
	}
	if( e->button() == Qt::MouseButton::MiddleButton ) {
		qApp->quit();
	}
}

void ClockWidget::mouseMoveEvent( QMouseEvent* e ) {

	if( m_dragging && ( e->buttons() & Qt::LeftButton ) ) {
		move( e->globalPos() - m_dragOffset );
		resize( m_size );
	}
}

void ClockWidget::mouseReleaseEvent( QMouseEvent* e ) {

	if( e->button() == Qt::LeftButton ) {
		m_dragging = false;
	}
}

void ClockWidget::contextMenuEvent( QContextMenuEvent* e ) {

	buildSharedMenu()->exec( e->globalPos() );
}

QMenu* ClockWidget::buildSharedMenu() {

	if( _sharedMenu ) {
		return _sharedMenu;
	}

	_sharedMenu = new QMenu( this );

	QAction* aBigger = _sharedMenu->addAction( "Size +20" );
	QAction* aSmaller = _sharedMenu->addAction( "Size -20" );

	QActionGroup* opacityGroup = new QActionGroup( this );
	opacityGroup->setExclusive( true );

	QMenu* op = _sharedMenu->addMenu( "Opacity" );
	QAction* aOp100 = op->addAction( "100%" );
	aOp100->setCheckable( true );
	aOp100->setActionGroup( opacityGroup );

	QAction* aOp075 = op->addAction( "75%" );
	aOp075->setCheckable( true );
	aOp075->setActionGroup( opacityGroup );

	QAction* aOp050 = op->addAction( "50%" );
	aOp050->setCheckable( true );
	aOp050->setActionGroup( opacityGroup );

	switch( m_opacity ) {
		case 100: aOp100->setChecked( true ); break;

		case 75: aOp075->setChecked( true ); break;

		case 50: aOp050->setChecked( true ); break;

		default: break;
	}

	QAction* aColorToggle = _sharedMenu->addAction( m_inverse ? "Dark" : "Light" );

	_actOnTop = _sharedMenu->addAction( "Always on top" );
	_actOnTop->setCheckable( true );

	_actNoTaskbar = _sharedMenu->addAction( "No Taskbar" );
	_actNoTaskbar->setCheckable( true );

	_sharedMenu->addSeparator();
	QAction* aExit = _sharedMenu->addAction( "Exit" );

	connect( _sharedMenu, &QMenu::aboutToShow, this, [this, aColorToggle] {

		_actOnTop->setChecked( windowFlags().testFlag( Qt::WindowStaysOnTopHint ) );
		_actNoTaskbar->setChecked( windowFlags().testFlag( Qt::Tool ) );
		aColorToggle->setText( m_inverse ? "Dark Theme" : "Light Theme" );
			 } );

	connect( aBigger, &QAction::triggered, this, [this] {

		QSettings s;
		int w = width() + 20;
		int h = height() + 20;
		m_size = QSize( w, h );
		resize( w, h );
		s.setValue( "size", m_size );
			 } );

	connect( aSmaller, &QAction::triggered, this, [this] {

		QSettings s;
		int w = qMax( 100, width() - 20 );
		int h = qMax( 80, height() - 20 );
		m_size = QSize( w, h );
		resize( w, h );
		s.setValue( "size", m_size );
			 } );

	connect( aOp100, &QAction::triggered, this, [this] {

		QSettings s;
		setWindowOpacity( 1.0 );
		s.setValue( "opacity", 100 );
			 } );

	connect( aOp075, &QAction::triggered, this, [this] {

		QSettings s;
		setWindowOpacity( 0.75 );
		s.setValue( "opacity", 75 );
			 } );

	connect( aOp050, &QAction::triggered, this, [this] {

		QSettings s;
		setWindowOpacity( 0.5 );
		s.setValue( "opacity", 50 );
			 } );

	connect( aColorToggle, &QAction::triggered, this, [this, aColorToggle] {

		QSettings s;
		m_inverse = !m_inverse;
		s.setValue( "theme", m_inverse );
		aColorToggle->setText( m_inverse ? "Dark" : "Light" );
			 } );

	connect( _actOnTop, &QAction::toggled, this, [this] ( bool want ) {

		QSettings s;
		s.setValue( "onTop", want );

		Qt::WindowFlags f = windowFlags();
		if( want ) {
			f |= Qt::WindowStaysOnTopHint;
		}
		else {
			f &= ~Qt::WindowStaysOnTopHint;
		}
		setWindowFlags( f );
		show();
			 } );

	connect( _actNoTaskbar, &QAction::toggled, this, [this] ( bool want ) {

		QSettings s;
		s.setValue( "aNoTaskbar", want );

		Qt::WindowFlags f = windowFlags();
		if( want ) {
			f |= Qt::Tool;
		}
		else {
			f &= ~Qt::Tool;
		}
		setWindowFlags( f );
		show();

		if( m_trayIcon ) {
			m_trayIcon->setVisible( want );
		}
			 } );

	connect( aExit, &QAction::triggered, qApp, &QCoreApplication::quit );

	return _sharedMenu;
}

void ClockWidget::initUiAndTray() {

	QSettings s;

	m_onTop = s.value( "onTop", false ).toBool();
	m_noTaskbar = s.value( "aNoTaskbar", false ).toBool();
	m_inverse = s.value( "theme", false ).toBool();
	m_size = s.value( "size", QSize( 200, 200 ) ).toSize();
	m_opacity = s.value( "opacity", 75 ).toInt();

	Qt::WindowFlags f = Qt::FramelessWindowHint | Qt::Window;
	if( m_onTop ) {
		f |= Qt::WindowStaysOnTopHint;
	}
	if( m_noTaskbar ) {
		f |= Qt::Tool;
	}
	setWindowFlags( f );
	show();

	setWindowOpacity( m_opacity * 0.01 );

	QPoint p = s.value( "pos", QPoint( 100, 100 ) ).toPoint();
	move( p );
	resize( m_size );

	m_trayIcon = new QSystemTrayIcon( this );
	m_trayIcon->setIcon( QIcon( ":/icon.ico" ) );
	m_trayIcon->setContextMenu( buildSharedMenu() );
	m_trayIcon->setVisible( m_noTaskbar );
}