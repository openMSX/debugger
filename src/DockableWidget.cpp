#include "DockableWidget.h"
#include "DockManager.h"
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionDockWidget>
#include <QRubberBand>
#include <QMouseEvent>
#include <QStatusBar>
#include <algorithm>


DockableWidget::DockableWidget(DockManager& manager, QWidget* parent)
	: QWidget(parent), dockManager(manager)
{
	mainWidget = nullptr;
	floating = false;
	movable = true;
	closable = true;
	destroyable = true;
	dragging = false;
	setAttribute(Qt::WA_DeleteOnClose, true);
#ifndef Q_OS_MAC
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
#else
	// on Mac the FramelessWindowHint hides the SizeGrip
	setWindowFlags(Qt::Tool);
	setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif

	titleLabel = new QLabel();
	closeButton = new QToolButton();
	closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	closeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	int sz = style()->pixelMetric(QStyle::PM_SmallIconSize);
	closeButton->setIconSize(style()->standardIcon(
		QStyle::SP_TitleBarCloseButton).actualSize(QSize(sz, sz)));
	closeButton->setAutoRaise(true);
	connect(closeButton, &QToolButton::clicked, this, &DockableWidget::close);

	headerLayout = new QHBoxLayout();
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->addWidget(titleLabel, 1);
	headerLayout->addWidget(closeButton, 0);

	headerWidget = new QWidget();
	headerWidget->setLayout(headerLayout);

	widgetLayout = new QVBoxLayout();
	widgetLayout->setContentsMargins(3, 3, 3, 3);
	widgetLayout->setSpacing(1);
	widgetLayout->addWidget(headerWidget);
	setLayout(widgetLayout);

	dockManager.attachWidget(this);

	rubberBand = new QRubberBand(QRubberBand::Rectangle);
}

DockableWidget::~DockableWidget()
{
	delete mainWidget;
	delete rubberBand;
	dockManager.detachWidget(this);
}

QWidget* DockableWidget::widget() const
{
	return mainWidget;
}

void DockableWidget::setWidget(QWidget* widget)
{
	if (mainWidget) {
		widgetLayout->removeWidget(mainWidget);
		delete statusBar;
	}
	mainWidget = widget;

	if (widget) {
		widgetLayout->addWidget(widget, 1);
		int minW = sizeHint().width()  - widget->sizeHint().width()
		         + widget->minimumWidth();
		int minH = sizeHint().height() - widget->sizeHint().height()
		         + widget->minimumHeight();
		int maxW = sizeHint().width()  - widget->sizeHint().width()
		         + widget->maximumWidth();
		int maxH = sizeHint().height() - widget->sizeHint().height()
		         + widget->maximumHeight();
		maxW = std::min(maxW, QWIDGETSIZE_MAX);
		maxH = std::min(maxH, QWIDGETSIZE_MAX);
		setMinimumSize(minW, minH);
		setMaximumSize(maxW, maxH);
		setSizePolicy(widget->sizePolicy());

		statusBar = new QStatusBar(nullptr);
		statusBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		statusBar->setVisible(false);
		statusBar->setSizeGripEnabled(false);
		widgetLayout->addWidget(statusBar, 2);
	}
}

const QString& DockableWidget::id() const
{
	return widgetId;
}

void DockableWidget::setId(const QString& str)
{
	widgetId = str;
}

QString DockableWidget::title() const
{
	return windowTitle();
}

void DockableWidget::setTitle(const QString& title)
{
	setWindowTitle(title);
	titleLabel->setText(title + ':');
}

bool DockableWidget::isFloating() const
{
	return floating;
}

void DockableWidget::setFloating(bool enable, bool showNow)
{
	if (floating == enable) return;

	floating = enable;

	if (mainWidget->sizePolicy().horizontalPolicy() != QSizePolicy::Fixed &&
			mainWidget->sizePolicy().verticalPolicy() != QSizePolicy::Fixed) {
		statusBar->setVisible(floating);
		statusBar->setSizeGripEnabled(floating);
	}

	if (floating && showNow) {
		// force widget to never get behind main window
#ifndef Q_OS_MAC
		setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
#else
		// on Mac the FramelessWindowHint hides the SizeGrip
		setWindowFlags(Qt::Tool);
		setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
		show();
	}
}

bool DockableWidget::isMovable() const
{
	return movable;
}

void DockableWidget::setMovable(bool enable)
{
	movable = enable;
}

bool DockableWidget::isClosable() const
{
	return closable;
}

void DockableWidget::setClosable(bool enable)
{
	if (closable == enable) return;

	closable = enable;
	if (enable) {
		closeButton->show();
	} else {
		closeButton->hide();
	}
	dragging = false;
	rubberBand->hide();
}

bool DockableWidget::isDestroyable() const
{
	return destroyable;
}

void DockableWidget::setDestroyable(bool enable)
{
	destroyable = enable;
	setAttribute(Qt::WA_DeleteOnClose, enable);
}

void DockableWidget::closeEvent(QCloseEvent* event)
{
	if (closable || destroyable) {
		if (destroyable && floating) {
			dockManager.undockWidget(this);
			event->accept();
		} else {
			hide();
			event->ignore();
			emit visibilityChanged(this);
		}
	} else {
		event->ignore();
	}
}

void DockableWidget::mousePressEvent(QMouseEvent* event)
{
	if (movable && event->button() == Qt::LeftButton) {
		dragging = true;
		dragStart = event->globalPosition();
		dragOffset = event->pos();
	}
}

void DockableWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!dragging) return;

	if (event->buttons() & Qt::LeftButton) {
		// dragging of widget in progress, update rubberband
		if (!rubberBand->isVisible()) {
			if (abs(event->globalPosition().x() - dragStart.x()) > 20 ||
			    abs(event->globalPosition().y() - dragStart.y()) > 20) {
				rubberBand->resize(width(), height());
				rubberBand->move(event->globalPosition().x()-dragOffset.x(),
				                 event->globalPosition().y()-dragOffset.y());
				rubberBand->show();
			}
		} else {
			QRect r(event->globalPosition().x()-dragOffset.x(),
			        event->globalPosition().y()-dragOffset.y(),
			        width(), height());
			if (floating && dockManager.insertLocation(r, sizePolicy())) {
				rubberBand->move(r.x(), r.y());
				rubberBand->resize(r.width(), r.height());
			} else {
				rubberBand->move(event->globalPosition().x()-dragOffset.x(),
				                 event->globalPosition().y()-dragOffset.y());
				rubberBand->resize(width(), height());
			}
		}
	} else {
		dragging = false;
		rubberBand->hide();
	}
}

void DockableWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (!dragging) return;

	dragging = false;
	rubberBand->hide();

	// only do anything if this was a meaningful drag
	if (!movable ||
	    (abs(event->globalPosition().x() - dragStart.x()) <= 20 &&
	     abs(event->globalPosition().y() - dragStart.y()) <= 20)) {
		return;
	}

	if (floating) {
		QRect mouseRect(event->globalPosition().x() - dragOffset.x(),
		                event->globalPosition().y() - dragOffset.y(),
		                width(), height());
		QRect r(mouseRect);
		if (dockManager.insertLocation(r, sizePolicy())) {
			setFloating(false);
			dockManager.dockWidget(this, QPoint(), mouseRect);
		} else {
			auto m = event->globalPosition() - dragOffset;
			move((int)m.x(), (int)m.y());
		}
	} else {
		dockManager.undockWidget(this);
		setFloating(true);

		auto m = event->globalPosition() - dragOffset;
		move((int)m.x(), (int)m.y());
	}
}
