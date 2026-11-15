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

void QuickEntryRack::restoreTiles()
{
	m_tiles->setText(m_rackTiles);
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

void GraphicalRack::setBoardFrame(GraphicalBoardFrame *board)
{
	m_boardFrame = board;
}

void GraphicalRack::setText(const Quackle::LetterString &text)
{
	m_pressedLabel = nullptr;
	while (m_layout->count())
	{
		QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(0)->widget());
		if (!label)
			break;
		m_layout->removeWidget(label);
		label->close();
	}

	PixmapCacher::self()->invalidate();
	for (int i = 0; i < static_cast<int>(text.size()); ++i)
	{
		QLabel *label = new QLabel;
		label->setAttribute(Qt::WA_DeleteOnClose);
		label->setFixedSize(50, 50);
		label->setProperty("quackleSlot", i);
		label->setProperty("quackleOccupied", true);
		updateTile(label, text[i]);
		m_layout->insertWidget(i, label);
	}
}

void GraphicalRack::updateTile(QLabel *label, Quackle::Letter letter)
{
	Quackle::Board::TileInformation info;
	info.isOnRack = true;
	info.tileType = Quackle::Board::LetterTile;
	info.isBlank = letter != QUACKLE_BLANK_MARK && QUACKLE_ALPHABET_PARAMETERS->isBlankLetter(letter);
	info.letter = letter;
	TileWidget tile;
	tile.setDevicePixelRatio(devicePixelRatio());
	tile.setInformation(info);
	tile.setSideLength(50);
	tile.prepare();
	label->setPixmap(tile.tilePixmap());
	label->setProperty("quackleLetter", static_cast<int>(letter));
}

bool GraphicalRack::dragTile(int slot, Quackle::Letter &letter) const
{
	if (slot < 0 || slot >= m_layout->count())
		return false;
	QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(slot)->widget());
	if (!label || !label->property("quackleOccupied").toBool())
		return false;
	letter = static_cast<Quackle::Letter>(label->property("quackleLetter").toInt());
	return letter != QUACKLE_BLANK_MARK;
}

void GraphicalRack::returnTile(int slot)
{
	if (slot < 0 || slot >= m_layout->count())
		return;
	QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(slot)->widget());
	if (label)
	{
		label->setProperty("quackleOccupied", true);
		updateTile(label, static_cast<Quackle::Letter>(label->property("quackleLetter").toInt()));
	}
}

void GraphicalRack::removeTile(int slot)
{
	if (slot < 0 || slot >= m_layout->count())
		return;
	QLabel *label = qobject_cast<QLabel *>(m_layout->itemAt(slot)->widget());
	if (label)
	{
		label->setProperty("quackleOccupied", false);
		label->clear();
	}
}

void GraphicalRack::mousePressEvent(QMouseEvent *event)
{
	m_pressedLabel = nullptr;
	if (event->button() == Qt::LeftButton)
	{
		QLabel *label = qobject_cast<QLabel *>(childAt(event->pos()));
		if (label && label->property("quackleOccupied").toBool())
		{
			m_pressedLabel = label;
			m_pressedPosition = event->pos();
			event->accept();
			return;
		}
	}
	event->ignore();
}

void GraphicalRack::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_pressedLabel || !(event->buttons() & Qt::LeftButton)
		|| (event->pos() - m_pressedPosition).manhattanLength() < QApplication::startDragDistance())
		return;

	QLabel *label = m_pressedLabel;
	m_pressedLabel = nullptr;
	Quackle::Letter letter;
	const int slot = label->property("quackleSlot").toInt();
	if (!dragTile(slot, letter))
	{
		QMessageBox::information(this, tr("Blank tile"), tr("Click the blank and enter a lowercase letter before dragging it."));
		return;
	}
	if (!m_boardFrame || m_boardFrame->tileSideLength() <= 0)
	{
		QMessageBox::information(this, tr("Board not ready"), tr("The board is not ready for tile placement."));
		return;
	}
	const int side = m_boardFrame->tileSideLength();
	const QPixmap source = label->grab();
	const qreal ratio = source.devicePixelRatio();
	QPixmap preview = source.scaled(qRound(side * ratio), qRound(side * ratio), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	preview.setDevicePixelRatio(ratio);
	QMimeData *mime = new QMimeData;
	mime->setData("application/x-quackle-rack-slot", QByteArray::number(slot));
	QDrag *drag = new QDrag(this);
	drag->setMimeData(mime);
	drag->setPixmap(preview);
	drag->setHotSpot(QPoint(side / 2, side / 2));
	drag->exec(Qt::MoveAction);
}

void GraphicalRack::mouseReleaseEvent(QMouseEvent *event)
{
	QLabel *label = m_pressedLabel;
	m_pressedLabel = nullptr;
	if (event->button() != Qt::LeftButton || !label || childAt(event->pos()) != label)
		return;
	const Quackle::Letter current = static_cast<Quackle::Letter>(label->property("quackleLetter").toInt());
	if (current != QUACKLE_BLANK_MARK && !QUACKLE_ALPHABET_PARAMETERS->isBlankLetter(current))
		return;

	bool accepted = false;
	const QString text
		= QInputDialog::getText(this, tr("Blank tile"), tr("Enter a lowercase letter:"), QLineEdit::Normal, QString(), &accepted)
			  .normalized(QString::NormalizationForm_C);
	if (!accepted)
		return;
	const Quackle::LetterString letters = QuackleIO::Util::encode(text);
	if (text.isEmpty() || text != text.toLower() || letters.length() != 1 || letters[0] < QUACKLE_FIRST_LETTER
		|| QuackleIO::Util::letterToQString(QUACKLE_ALPHABET_PARAMETERS->clearBlankness(letters[0])).toLower() != text)
	{
		QMessageBox::warning(this, tr("Blank tile"), tr("Enter one lowercase letter from the selected alphabet."));
		return;
	}
	updateTile(label, QUACKLE_ALPHABET_PARAMETERS->setBlankness(QUACKLE_ALPHABET_PARAMETERS->clearBlankness(letters[0])));
}
