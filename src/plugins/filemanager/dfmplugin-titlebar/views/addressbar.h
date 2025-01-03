// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AddressBar_H
#define AddressBar_H

#include "dfmplugin_titlebar_global.h"

#include <QLineEdit>

namespace dfmplugin_titlebar {

class AddressBarPrivate;
class AddressBar : public QLineEdit
{
    Q_OBJECT
    friend class AddressBarPrivate;
    AddressBarPrivate *const d;

public:
    explicit AddressBar(QWidget *parent = nullptr);
    bool completerViewVisible();
    void setCurrentUrl(const QUrl &url);
    QUrl currentUrl();
    void showOnFocusLostOnce();

protected:
    bool event(QEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *e) override;

Q_SIGNALS:
    void lostFocus();
    void escKeyPressed();
    void clearButtonPressed();
    void pauseButtonClicked();
    void urlChanged();
};

}

#endif   //AddressBar_H
