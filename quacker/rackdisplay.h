/*
 *  Quackle -- Crossword game artificial intelligence and analysis tool
 *  Copyright (C) 2005-2019 Jason Katz-Brown, John O'Laughlin, and John Fultz.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUACKER_RACKDISPLAY_H
#define QUACKER_RACKDISPLAY_H

#include "view.h"
#include "alphabetparameters.h"

#include <QPointer>

class QFont;
class QLabel;
class QLineEdit;
class QHBoxLayout;
class GraphicalBoardFrame;
class GraphicalRack;
class RackTileWidget;

class QuickEntryRack : public View
{
	Q_OBJECT

public:
	QuickEntryRack(QWidget *parent = 0);
	virtual ~QuickEntryRack();
	GraphicalRack *tiles() const { return m_tiles; }

public slots:
	virtual void positionChanged(const Quackle::GamePosition *position);
	virtual void grabFocus();
	void returnTile(int slot);
	void removeTile(int slot);
	void restoreTiles();

private slots:
	void quickEditReturnPressed();
	void shuffle();

protected:
	virtual void processRack(const QString &rack);

private:
	QLabel *m_label;
	QLineEdit *m_lineEdit;
	GraphicalRack *m_tiles;
	Quackle::LetterString m_rackTiles;
};

class GraphicalRack : public QFrame
{
	Q_OBJECT

public:
	GraphicalRack(QWidget *parent = 0);
	bool dragTile(int slot, Quackle::Letter &letter) const;
	void setBoardFrame(GraphicalBoardFrame *board);

public slots:
	virtual void setText(const Quackle::LetterString &text);
	void returnTile(int slot);
	void removeTile(int slot);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	void updateTile(QLabel *label, Quackle::Letter letter);
	QHBoxLayout *m_layout;
	QPoint m_pressedPosition;
	QLabel *m_pressedLabel = nullptr;
	QPointer<GraphicalBoardFrame> m_boardFrame;
};

#endif
