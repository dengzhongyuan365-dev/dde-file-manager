/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huanyu<huanyub@uniontech.com>
 *
 * Maintainer: zhengyouge<zhengyouge@uniontech.com>
 *             yanghao<yanghao@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILEVIEW_H
#define FILEVIEW_H

#include "dfm-base/interfaces/abstractbaseview.h"

#include <DListView>

DWIDGET_USE_NAMESPACE

class FileViewModel;
class FileViewPrivate;
class BaseItemDelegate;
class FileView : public DListView, public DFMBASE_NAMESPACE::AbstractBaseView
{
    Q_OBJECT
    friend class FileViewPrivate;
    QSharedPointer<FileViewPrivate> d;

public:
    explicit FileView(QWidget *parent = nullptr);

    QWidget *widget() const override;
    bool setRootUrl(const QUrl &url) override;
    QUrl rootUrl() const override;
    ViewState viewState() const override;
    QList<QAction *> toolBarActionList() const override;
    QList<QUrl> selectedUrlList() const override;
    void refresh() override;

    void setViewMode(QListView::ViewMode mode);
    void setDelegate(QListView::ViewMode mode, BaseItemDelegate *view);
    FileViewModel *model() const;
    void setModel(QAbstractItemModel *model) override;

    int getColumnWidth(const int &column) const;
    int getHeaderViewWidth() const;

public slots:
    void onHeaderViewMouseReleased();
    void onHeaderSectionResized(int logicalIndex, int oldSize, int newSize);
    void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);
    void onClicked(const QModelIndex &index);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QModelIndexList selectedIndexes() const override;

Q_SIGNALS:
    void urlClicked(const QUrl &url);
    void fileClicked(const QUrl &url);
    void dirClicked(const QUrl &url);

    void reqOpenNewWindow(const QList<QUrl> &urls);

private:
    void initializeModel();
    void initializeDelegate();
};

#endif   // FILEVIEW_H
