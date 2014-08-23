
#ifndef ORBITAL_COMPOSITOR_H
#define ORBITAL_COMPOSITOR_H

#include <QObject>
#include <QTimer>

struct wl_display;
struct wl_event_loop;
struct weston_compositor;
struct weston_surface;

namespace Orbital {

class Backend;
class Shell;
class Layer;
class Output;
class DummySurface;
class View;
struct Listener;

class Compositor : public QObject
{
    Q_OBJECT
public:
    explicit Compositor(Backend *backend);
    ~Compositor();

    bool init(const QString &socket);

    Layer *rootLayer() const;
    QList<Output *> outputs() const;

    DummySurface *createDummySurface(int width, int height);
    View *pickView(double x, double y, double *vx = nullptr, double *vy = nullptr) const;

    static Compositor *fromCompositor(weston_compositor *c);

private:
    void processEvents();

    wl_display *m_display;
    wl_event_loop *m_loop;
    weston_compositor *m_compositor;
    Listener *m_listener;
    Backend *m_backend;
    Shell *m_shell;
    Layer *m_rootLayer;
    QList<Output *> m_outputs;
    QTimer m_timer;

    friend class Global;
};

}

#endif