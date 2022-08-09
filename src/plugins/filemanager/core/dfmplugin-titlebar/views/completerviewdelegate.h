#ifndef COMPLETERVIEWDELEGATE_H
#define COMPLETERVIEWDELEGATE_H

#include "dfmplugin_titlebar_global.h"

#include <QStyledItemDelegate>

namespace dfmplugin_titlebar {

class CompleterViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit CompleterViewDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    void paintItemIcon(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    QPixmap createCustomOpacityPixmap(const QPixmap &px, float opacity) const;
};

}

#endif   // COMPLETERVIEWDELEGATE_H
