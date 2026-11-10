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

#include <iostream>

#include <QtWidgets>

#include <game.h>
#include <quackleio/util.h>

#include "geometry.h"
#include "graphicalboard.h"
#include "rackdisplay.h"

QuickEntryRack::QuickEntryRack(QWidget *parent)
	: View(parent)
{
	QHBoxLayout *textLayout = new QHBoxLayout();
	Geometry::setupInnerLayout(textLayout);

	m_lineEdit = new QLineEdit;
	m_lineEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(quickEditReturnPressed()));

	QPushButton *setButton = new QPushButton(tr("Set Rack"));
	connect(setButton, SIGNAL(clicked()), this, SLOT(quickEditReturnPressed()));

	QPushButton *shuffleButton = new QPushButton(tr("Shu&ffle"));
	connect(shuffleButton, SIGNAL(clicked()), this, SLOT(shuffle()));

	m_label = new QLabel(tr("&Rack:"));
	m_label->setBuddy(m_lineEdit);
	textLayout->addWidget(m_label, 1);
	textLayout->addWidget(m_lineEdit, 4);
	textLayout->addWidget(setButton, 1);
	textLayout->addWidget(shuffleButton, 1);

	m_tiles = new GraphicalRack;

	QVBoxLayout *layout = new QVBoxLayout;
	Geometry::setupInnerLayout(layout);
	layout->addWidget(m_tiles);
	layout->addLayout(textLayout);
	setLayout(layout);
	connect(m_tiles, SIGNAL(rackTileSelected(int)), this, SIGNAL(rackTileSelected(int)));
}

QuickEntryRack::~QuickEntryRack() {}

void QuickEntryRack::positionChanged(const Quackle::GamePosition *position)
{
	if (m_rackTiles == position->currentPlayer().rack().tiles())
		return;

	m_rackTiles = position->currentPlayer().rack().tiles();
	QString tiles = QuackleIO::Util::letterStringToQString(m_rackTiles);
	m_lineEdit->setText(tiles);
	m_tiles->setText(m_rackTiles);
}

void QuickEntryRack::grabFocus()
{
	m_lineEdit->setFocus();
	m_lineEdit->selectAll();
}

void QuickEntryRack::returnTile(int letter)
{
	m_tiles->returnTile(letter);
}

void QuickEntryRack::removeTile(int letter)
{
	m_tiles->removeTile(letter);
}

void QuickEntryRack::quickEditReturnPressed()
{
	QString text(m_lineEdit->text());
	m_lineEdit->clear();
	processRack(text);
}

void QuickEntryRack::processRack(const QString &rack)
{
	if (rack.isEmpty())
	{
		emit statusMessage(tr("Useless rack."));
		return;
	}

	emit setRack(QuackleIO::Util::makeRack(rack));
}

void QuickEntryRack::shuffle()
{
	Quackle::Rack rack(QuackleIO::Util::makeRack(m_lineEdit->text()));
	rack.shuffle();

	emit setRack(rack);
}

GraphicalRack::GraphicalRack(QWidget *parent)
	: QFrame(parent)
{
	m_layout = new QHBoxLayout(this);
	Geometry::setupInnerLayout(m_layout);
	m_layout->addStretch();

	setMinimumSize(50, 50);
}

void GraphicalRack::setText(const Quackle::LetterString &text)
{
	// clear old labels
	m_selectedLabel = nullptr;
	while (m_layout->count())
	{
		QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(0)->widget());
		if (!label)
		{
			break;
		}
		m_layout->removeWidget(label);
		label->close();
	}

	PixmapCacher::self()->invalidate();
	for (int i = text.size() - 1; i >= 0; --i)
	{
		QLabel *label = new QLabel;
		label->setAttribute(Qt::WA_DeleteOnClose);

		TileWidget tile;
		Quackle::Board::TileInformation info;
		info.isOnRack = true;
		info.letter = text[i];
		info.tileType = Quackle::Board::LetterTile;
		tile.setDevicePixelRatio(devicePixelRatio());
		tile.setInformation(info);
		tile.setSideLength(50);
		tile.prepare();

		label->setPixmap(tile.tilePixmap());
		label->setProperty("quackleLetter", static_cast<int>(text[i]));

		m_layout->insertWidget(0, label);
	}
}

void GraphicalRack::returnTile(int letterValue)
{
	const Quackle::Letter letter = static_cast<Quackle::Letter>(letterValue);
	Quackle::Board::TileInformation info;
	info.isOnRack = true;
	info.tileType = Quackle::Board::LetterTile;
	info.letter = QUACKLE_ALPHABET_PARAMETERS->isBlankLetter(letter) ? QUACKLE_BLANK_MARK : letter;

	TileWidget tile;
	tile.setDevicePixelRatio(devicePixelRatio());
	tile.setInformation(info);
	tile.setSideLength(50);
	tile.prepare();

	QLabel *label = new QLabel;
	label->setPixmap(tile.tilePixmap());
	label->setProperty("quackleLetter", letterValue);
	label->setAttribute(Qt::WA_DeleteOnClose);
	m_layout->insertWidget(m_layout->count() - 1, label);
}

void GraphicalRack::removeTile(int letterValue)
{
	for (int index = 0; index < m_layout->count(); ++index)
	{
		QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(index)->widget());
		if (label && label->property("quackleLetter").toInt() == letterValue)
		{
			if (m_selectedLabel == label)
				m_selectedLabel = nullptr;
			m_layout->removeWidget(label);
			label->close();
			return;
		}
	}
}

void GraphicalRack::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		event->ignore();
		return;
	}

	QLabel *label = qobject_cast<QLabel *>(childAt(event->pos()));
	if (!label)
	{
		event->ignore();
		return;
	}

	if (m_selectedLabel)
		m_selectedLabel->setFrameStyle(QFrame::NoFrame);
	m_selectedLabel = label;
	m_selectedLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
	m_selectedLabel->setLineWidth(2);
	emit rackTileSelected(label->property("quackleLetter").toInt());
	event->accept();
}
