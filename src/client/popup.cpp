/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtQml>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDebug>

#include "wayland-desktop-shell-client-protocol.h"
#include "popup.h"
#include "client.h"
#include "grab.h"
#include "utils.h"
#include "element.h"

static const int a = qmlRegisterType<Popup>("Orbital", 1, 0, "Popup");

Popup::Popup(QObject *p)
     : QObject(p)
     , m_window(nullptr)
     , m_parent(nullptr)
     , m_content(nullptr)
     , m_shsurf(nullptr)
{
}

Popup::~Popup()
{
    if (m_window) {
        hide();
    }
}

bool Popup::visible() const
{
    return m_window ? m_window->isVisible() : false;
}

void Popup::setVisible(bool v)
{
    v ? show() : hide();
}

void Popup::setContent(QQuickItem *i)
{
    if (m_content == i) {
        return;
    }

    if (m_content) {
        disconnect(m_content, &QQuickItem::widthChanged, this, &Popup::contentWidthChanged);
        disconnect(m_content, &QQuickItem::heightChanged, this, &Popup::contentHeightChanged);
    }
    m_content = i;
    if (m_content) {
        connect(m_content, &QQuickItem::widthChanged, this, &Popup::contentWidthChanged);
        connect(m_content, &QQuickItem::heightChanged, this, &Popup::contentHeightChanged);
    }
}

void Popup::show()
{
    if (!m_parent || !m_content || m_shsurf) {
        return;
    }

    Element *e = Element::fromItem(m_parent);
    if (!e) {
        qWarning() << "Popup::show(): Something is wrong, the popup does not have a parent Element!";
        return;
    }

    Element::Location loc = e->location();

    QQuickWindow *w = m_parent->window();
    QPointF pos = m_parent->mapToScene(QPointF(0, 0));
    if (!m_window) {
        m_window = new QQuickWindow();
        m_window->setFlags(Qt::BypassWindowManagerHint);
        m_window->create();
    }
    m_window->setScreen(w->screen());

    // TODO: Better placement, maybe by adding some protocol
    switch (loc) {
        case Element::Location::TopEdge:
        case Element::Location::Floating:
            m_window->setX(pos.x());
            m_window->setY(pos.y() + m_parent->height());
            break;
        case Element::Location::LeftEdge:
            m_window->setX(pos.x() + m_parent->width());
            m_window->setY(pos.y());
            break;
        case Element::Location::RightEdge:
            m_window->setX(pos.x() - m_content->width());
            m_window->setY(pos.y());
            break;
        case Element::Location::BottomEdge:
            m_window->setX(pos.x());
            m_window->setY(pos.y() - m_content->height());
            break;
    }

    m_window->setWidth(m_content->width());
    m_window->setHeight(m_content->height());

    m_window->setColor(Qt::transparent);
    m_content->setParentItem(m_window->contentItem());

    m_window->show();

    m_shsurf = Client::client()->setPopup(m_window, e->window());
    desktop_shell_surface_add_listener(m_shsurf, &m_shsurf_listener, this);

    emit visibleChanged();
}

void Popup::hide()
{
    hideEvent();
}

void Popup::hideEvent()
{
    m_window->hide();
    emit visibleChanged();
    if (m_shsurf) {
        desktop_shell_surface_destroy(m_shsurf);
        m_shsurf = nullptr;
    }
}

void Popup::close(desktop_shell_surface *s)
{
    QMetaObject::invokeMethod(this, "hideEvent");
}

void Popup::contentWidthChanged()
{
    if (m_window) {
        m_window->setWidth(m_content->width());
    }
}

void Popup::contentHeightChanged()
{
    if (m_window) {
        m_window->setHeight(m_content->height());
    }
}

const desktop_shell_surface_listener Popup::m_shsurf_listener = {
    wrapInterface(&Popup::close)
};
