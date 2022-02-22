#ifndef DOCKABLEWIDGET_H
#define DOCKABLEWIDGET_H

#include <QWidget>
#include <QString>

class QLabel;
class QToolButton;
class QHBoxLayout;
class QVBoxLayout;
class QRubberBand;
class DockManager;
class QStatusBar;

class DockableWidget : public QWidget
{
	Q_OBJECT;
public:
	DockableWidget(DockManager& manager, QWidget* parent = nullptr);
	~DockableWidget() override;

	QWidget* widget() const;
	void setWidget(QWidget* widget);
	const QString& id() const;
	void setId(const QString& str);

	QString title() const;
	void setTitle(const QString& title);

	bool isFloating() const;
	void setFloating(bool enable, bool showNow = true);
	bool isMovable() const;
	void setMovable(bool enable);
	bool isClosable() const;
	void setClosable(bool enable);
	bool isDestroyable() const;
	void setDestroyable(bool enable);

private:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

	DockManager& dockManager;
	QString widgetId;

	bool floating;
	bool closable, movable, destroyable;
	bool dragging;
	QPoint dragStart, dragOffset;
	QRubberBand* rubberBand;

	QWidget* mainWidget;
	QHBoxLayout* headerLayout;
	QVBoxLayout* widgetLayout;
	QWidget* headerWidget;
	QLabel* titleLabel;
	QToolButton* closeButton;
	QStatusBar* statusBar;

signals:
	void visibilityChanged(DockableWidget* w);
};

#endif // DOCKABLEWIDGET_H
