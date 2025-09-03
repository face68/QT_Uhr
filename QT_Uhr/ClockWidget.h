#pragma once
#include <QWidget>
#include <QTimer>
#include <QMenu>
#include <QSystemTrayIcon>

class ClockWidget: public QWidget {

	Q_OBJECT

	public:

		explicit ClockWidget( QWidget* parent = nullptr );

	protected:

		void paintEvent( QPaintEvent* e ) override;
		void resizeEvent( QResizeEvent* ev ) override;
		void closeEvent( QCloseEvent* ev ) override;
		void mousePressEvent( QMouseEvent* e ) override;
		void mouseMoveEvent( QMouseEvent* e ) override;
		void mouseReleaseEvent( QMouseEvent* e ) override;
		void contextMenuEvent( QContextMenuEvent* e ) override;

	private:

		QMenu* buildSharedMenu();
		void initUiAndTray();
		QMenu* _sharedMenu = nullptr;
		QAction* _actOnTop = nullptr;
		QAction* _actNoTaskbar = nullptr;
		QSystemTrayIcon* m_trayIcon = nullptr;
		bool m_inverse = false;
		int m_opacity = 75;
		bool m_onTop = false;
		bool m_noTaskbar = false;
		QSize m_size;

		QTimer timer;

		QPoint m_dragOffset;
		bool m_dragging = false;
};
