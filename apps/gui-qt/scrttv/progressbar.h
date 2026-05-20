/***************************************************************************
 * Copyright (C) GFZ Potsdam                                               *
 * All rights reserved.                                                    *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 ***************************************************************************/




#ifndef SEISCOMP_GUI_PROGRESSBAR_H__
#define SEISCOMP_GUI_PROGRESSBAR_H__

#include <QPainter>
#include <QPaintEvent>
#include <QFrame>

class QWidget;


namespace Seiscomp {
namespace Gui {


class ProgressBar : public QFrame
{
  Q_OBJECT

  public:
	ProgressBar(QWidget *parent=0)
	: QFrame(parent)
	{
		setMinimumSize(50,10);
		_value = 0;
		// Set accessibility attributes
		setAccessibleName("Progress bar");
		setAccessibleDescription("Shows loading progress for seismic data");
	}

  public slots:
	void reset()
	{
		_value = 0;
		update();
		// Emit accessibility event
		 QAccessibleEvent event(this, QAccessible::ValueChanged);
		 QAccessible::updateAccessibility(&event);
	}

	void setValue(int val)
	{
		_value = val;
		if (_value>100)
			_value = 100;
		repaint();
		// Emit accessibility event
		QAccessibleEvent event(this, QAccessible::ValueChanged);
		QAccessible::updateAccessibility(&event);
	}

  protected:
	void paintEvent(QPaintEvent *)
	{
		int w=width(), h=height();
		QPainter paint(this);
		paint.fillRect(0, 0, int(w*_value / 100), h, QColor(0,0,128));
	}

	void focusInEvent(QFocusEvent *) override
	{
		update();
	}

	void focusOutEvent(QFocusEvent *) override
	{
		update();
	}

	virtual QAccessible::State state() const override
	{
		QAccessible::State state = QFrame::state();
		state.disabled = false;
		state.focused = false;
		return state;
	}

	virtual QString text(QAccessible::Text t) const override
	{
		if (t == QAccessible::Description) {
			return QString("Data loading progress: %1%").arg(_value);
		}
		return QFrame::text(t);
	}

  private:
	int _value;
};


}
}


#endif
