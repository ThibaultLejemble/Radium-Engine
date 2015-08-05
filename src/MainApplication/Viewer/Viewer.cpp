#include <MainApplication/Viewer/Viewer.hpp>

#include <iostream>

#include <QTimer>
#include <QMouseEvent>
#include <QPainter>

#include <Core/String/StringUtils.hpp>
#include <Core/Log/Log.hpp>

#include <Engine/Renderer/OpenGL/OpenGL.hpp>
#include <Engine/Entity/Component.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <MainApplication/Viewer/TrackballCamera.hpp>
#include <MainApplication/Gui/MainWindow.hpp>

/// Helper functions
namespace
{
    /// Allows us to access the main window
    Ra::Gui::MainWindow * getMainWin(const QWidget* w)
    {
        // Assumption : main window is our grand parent. This is checked in MainApplication
        return static_cast<Ra::Gui::MainWindow*>(w->parent()->parent());
    }

class RenderThread : public QThread, protected QOpenGLFunctions
{
public:
    RA_CORE_ALIGNED_NEW
    RenderThread(Ra::Gui::Viewer* viewer, Ra::Engine::Renderer* renderer)
    : QThread(viewer), m_viewer(viewer), m_renderer(renderer), isInit(false)
    {
        CORE_ASSERT(m_renderer != nullptr && m_viewer != nullptr,
        "Uninitialized renderer");
    }

    virtual ~RenderThread() {}

    // This is the function that gets called in the render thread
    virtual void run() override
    {
        // check that the context has correctly been moved from the main thread.
        CORE_ASSERT(m_viewer->context()->thread() == QThread::currentThread(),
                    "Context is in the wrong thread");

        // Grab the context
        m_viewer->makeCurrent();

        if(!isInit)
        {
            initializeOpenGLFunctions();
            isInit = true;
        }

        CORE_ASSERT(glGetString(GL_VERSION)!= 0, "GL context unavailable");

        // render will lock the renderer itself.
        m_renderer->render(m_renderData);

        // Give back viewer context to main thread.
        m_viewer->doneCurrent();
        m_viewer->context()->moveToThread( qApp->thread() );
    }

    /// Keep a local copy of the render data.
    Ra::Engine::RenderData m_renderData;
    Ra::Gui::Viewer* m_viewer;
    Ra::Engine::Renderer* m_renderer;
    bool isInit;
};

}

namespace Ra
{

Gui::Viewer::Viewer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_interactionState(NONE)
    , m_renderThread(nullptr)
{
    // Allow Viewer to receive events
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(QSize(800, 600));

    m_camera.reset(new Gui::TrackballCamera(width(), height()));

    /// Intercept events to properly lock the renderer when it is compositing.
#if !defined(FORCE_RENDERING_ON_MAIN_THREAD)
    connect(this, &QOpenGLWidget::aboutToCompose, this, &Viewer::onAboutToCompose);
    connect(this, &QOpenGLWidget::frameSwapped,   this, &Viewer::onFrameSwapped);
    connect(this, &QOpenGLWidget::aboutToResize,  this, &Viewer::onAboutToResize);
    connect(this, &QOpenGLWidget::resized,        this, &Viewer::onResized);
#endif

}

Gui::Viewer::~Viewer()
{
#if !defined(FORCE_RENDERING_ON_MAIN_THREAD)
    CORE_ASSERT(m_renderThread->isFinished(), "Render thread is still running");
    delete m_renderThread;
#endif
}

void Gui::Viewer::initializeGL()
{
    initializeOpenGLFunctions();

    LOG(logINFO) << "***Radium Engine Viewer***";
    LOG(logINFO) <<"Renderer : " << glGetString(GL_RENDERER);
    LOG(logINFO) <<"Vendor   : " << glGetString(GL_VENDOR);
    LOG(logINFO) <<"OpenGL   : " << glGetString(GL_VERSION);
    LOG(logINFO) <<"GLSL     : " << glGetString(GL_SHADING_LANGUAGE_VERSION);

#if defined (OS_WINDOWS)
    glewExperimental = GL_TRUE;

    GLuint result = glewInit();
    if (result != GLEW_OK)
    {
        std::string errorStr;
        Ra::Core::StringUtils::stringPrintf(errorStr, " GLEW init failed : %s", glewGetErrorString(result));
        CORE_ERROR(errorStr.c_str());
    }
    else
    {
        LOG(logINFO) << "GLEW     : " << glewGetString(GLEW_VERSION);
        glFlushError();
    }

#endif

#if defined(FORCE_RENDERING_ON_MAIN_THREAD)
    LOG(logDEBUG) << "Rendering on main thread";
#else
    LOG(logDEBUG) << "Rendering on dedicated thread";
#endif
    m_renderer.reset(new Engine::Renderer(width(), height()));
    m_renderer->initialize();

#if !defined (FORCE_RENDERING_ON_MAIN_THREAD)
    m_renderThread = new RenderThread(this, m_renderer.get());
#endif
}

void Gui::Viewer::initRenderer(Engine::RadiumEngine* engine)
{
    m_renderer->setEngine(engine);
}

void Gui::Viewer::onAboutToCompose()
{
    // This slot function is called from the main thread as part of the event loop
    // when the GUI is about to update. We have to wait for the rendering to finish.
    m_renderer->lockRendering();
}

void Gui::Viewer::onFrameSwapped()
{
    // This slot is called from the main thread as part of the event loop when the
    // GUI has finished displaying the rendered image, so we unlock the renderer.
    m_renderer->unlockRendering();
}

void Gui::Viewer::onAboutToResize()
{
    // Like swap buffers, resizing is a blocking operation and we have to wait for the rendering
    // to finish before resizing.
    m_renderer->lockRendering();
}

void Gui::Viewer::onResized()
{
    m_renderer->unlockRendering();
}

void Gui::Viewer::resizeGL(int width, int height)
{
    // Renderer should have been locked by previous events.
    m_camera->resizeViewport(width, height);
    m_renderer->resize(width, height);
}

void Gui::Viewer::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
        case Qt::LeftButton:
        {
            if (m_interactionState != NONE)
            {
                // TODO(Charly): Handle interaction mode.
                break;
            }

            if (m_camera->handleMousePressEvent(event))
            {
                 m_interactionState = CAMERA;
            }

        } break;

        case Qt::RightButton:
        {
            // Check picking
            // FIXME : check thread-saefty of this.
            m_renderer->lockRendering();
            makeCurrent();
            int clicked = m_renderer->checkPicking(event->x(), height() - event->y());
            LOG(logDEBUG) << "Clicked object " << clicked;
            doneCurrent();
            m_renderer->unlockRendering();
        } break;

        default:
        {
        } break;
    }
}

void Gui::Viewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_interactionState == CAMERA)
    {
        m_camera->handleMouseReleaseEvent(event);

        m_interactionState = NONE;
    }
}

void Gui::Viewer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_interactionState == CAMERA)
    {
        m_camera->handleMouseMoveEvent(event);
    }
}

void Gui::Viewer::wheelEvent(QWheelEvent* event)
{
    QOpenGLWidget::wheelEvent(event);
    getMainWin(this)->viewerWheelEvent(event);
}

void Gui::Viewer::reloadShaders()
{
    // FIXME : check thread-saefty of this.
    m_renderer->lockRendering();
    makeCurrent();
    m_renderer->reloadShaders();
    doneCurrent();
    m_renderer->unlockRendering();
}

// Asynchronous rendering implementation

void Gui::Viewer::startRendering( const Scalar dt )
{
#if defined(FORCE_RENDERING_ON_MAIN_THREAD)
    makeCurrent();
    Engine::RenderData data;
    data.projMatrix = m_camera->getProjMatrix();
    data.viewMatrix = m_camera->getViewMatrix();
    data.dt = dt;
    m_renderer->render(data);
#else
    CORE_ASSERT(m_renderThread != nullptr,
                "Render thread is not initialized (should have been done in initGL)");

    // First release the context and give it to the rendering thread.
    doneCurrent();
    context()->moveToThread(m_renderThread);

    // Copy camera data from the main thread as some later events may overwrite it.
    Engine::RenderData& data = static_cast<RenderThread*>(m_renderThread)->m_renderData;
    data.projMatrix = m_camera->getProjMatrix();
    data.viewMatrix = m_camera->getViewMatrix();
    data.dt = dt;

    // Launch the thread, calling the run() method.
    m_renderThread->start();
#endif
}

void Gui::Viewer::waitForRendering()
{
#if !defined(FORCE_RENDERING_ON_MAIN_THREAD)
    // Join with render thread.
    m_renderThread->wait();
    CORE_ASSERT( context()->thread() == QThread::currentThread(),
                 "Context has not been properly given back to main thread.");
    makeCurrent();
#endif
}

void Gui::Viewer::handleFileLoading(const std::string &file)
{
    m_renderer->handleFileLoading(file);
}

} // namespace Ra
