#pragma once
#include <QWidget>
#include <QMouseEvent>

class QT_Uhr: public QWidget {

	Q_OBJECT

	public:
		explicit QT_Uhr( QWidget* parent = nullptr );

	protected:
		void resizeEvent( QResizeEvent* ev ) override;
		void closeEvent( QCloseEvent* ev ) override;
		void mousePressEvent( QMouseEvent* e ) override;
		void mouseMoveEvent( QMouseEvent* e ) override;
		void mouseReleaseEvent( QMouseEvent* e ) override;

	private:
		void applyMaskPercent();
		bool m_dragging = false;
		QPoint m_dragOffset;
};