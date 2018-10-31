/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 * Copyright (C) 2018  Nathan Osman <nathan@quickmediasolutions.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <QMutableListIterator>

#include "application.h"
#include "mrumenu.h"
#include "settings.h"

const int MaxItems = 5;

using namespace LxImage;

MruMenu::MruMenu(QWidget *parent)
    : QMenu(parent),
      mFilenames(static_cast<Application*>(qApp)->settings().recentlyOpenedFiles())
{
    for (QStringList::const_iterator i = mFilenames.constBegin(); i != mFilenames.constEnd(); ++i) {
        addAction(createAction(*i));
    }

    // Add a separator and hide it if there are no items in the list
    mSeparator = addSeparator();
    mSeparator->setVisible(!mFilenames.empty());

    // Add the clear action and disable it if there are no items in the list
    mClearAction = new QAction(tr("&Clear"));
    mClearAction->setEnabled(!mFilenames.empty());
    connect(mClearAction, &QAction::triggered, this, &MruMenu::onClearTriggered);
    addAction(mClearAction);
}

void MruMenu::addItem(const QString &filename)
{
    if (mFilenames.isEmpty()) {
        mSeparator->setVisible(true);
        mClearAction->setEnabled(true);
    }

    // If the filename is already in the list, remove it
    int index = mFilenames.indexOf(filename);
    if (index != -1) {
        mFilenames.removeAt(index);
        destroyAction(index);
    }

    // Insert the action at the beginning of the list
    mFilenames.push_front(filename);
    insertAction(actions().first(), createAction(filename));

    // If the list contains more than MaxItems, remove the last one
    if (mFilenames.count() > MaxItems) {
        mFilenames.removeLast();
        destroyAction(MaxItems - 1);
    }

    updateSettings();
}

void MruMenu::onItemTriggered()
{
    Q_EMIT itemClicked(qobject_cast<QAction*>(sender())->text());
}

void MruMenu::onClearTriggered()
{
    while (!mFilenames.empty()) {
        mFilenames.removeFirst();
        destroyAction(0);
    }

    // Hide the separator and disable the clear action
    mSeparator->setVisible(false);
    mClearAction->setEnabled(false);

    updateSettings();
}

QAction *MruMenu::createAction(const QString &filename)
{
    QAction *action = new QAction(filename, this);
    connect(action, &QAction::triggered, this, &MruMenu::onItemTriggered);
    return action;
}

void MruMenu::destroyAction(int index)
{
    QAction *action = actions().at(index);
    removeAction(action);
    delete action;
}

void MruMenu::updateSettings()
{
    static_cast<Application*>(qApp)->settings().setRecentlyOpenedFiles(mFilenames);
}
