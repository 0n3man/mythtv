#ifndef MYTHRENDER_OPENGL_H_
#define MYTHRENDER_OPENGL_H_

// Std
#include <cstdint>

// Qt
#include <QObject>
#include <QtGlobal>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QHash>
#include <QMutex>
#include <QMatrix4x4>
#include <QStack>

// MythTV
#include "mythuiexp.h"
#include "mythlogging.h"
#include "mythrender_base.h"
#include "mythrender_opengl_defs.h"
#include "mythuianimation.h"

typedef enum
{
    kGLFeatNone    = 0x0000,
    kGLExtFBDiscard= 0x0001,
    kGLNVFence     = 0x0002,
    kGLAppleFence  = 0x0004,
    kGLBufferMap   = 0x0008,
    kGLExtRGBA16   = 0x0010
} GLFeatures;

#define TEX_OFFSET 8

class MUI_PUBLIC MythGLTexture
{
  public:
    explicit MythGLTexture(QOpenGLTexture *Texture);
    explicit MythGLTexture(GLuint Texture);
   ~MythGLTexture() = default;

    unsigned char  *m_data;
    int             m_bufferSize;
    GLuint          m_textureId;
    QOpenGLTexture *m_texture;
    QOpenGLTexture::PixelFormat m_pixelFormat;
    QOpenGLTexture::PixelType   m_pixelType;
    QOpenGLBuffer  *m_vbo;
    QSize           m_size;
    QSize           m_totalSize;
    bool            m_flip;
    bool            m_crop;
    QRect           m_source;
    QRect           m_destination;
    GLfloat         m_vertexData[16];

  private:
    Q_DISABLE_COPY(MythGLTexture)
};

typedef enum
{
    kShaderSimple  = 0,
    kShaderDefault,
    kShaderCircle,
    kShaderCircleEdge,
    kShaderVertLine,
    kShaderHorizLine,
    kShaderCount,
} DefaultShaders;

class QWindow;
class QPaintDevice;

class MUI_PUBLIC MythRenderOpenGL : public QOpenGLContext, public QOpenGLFunctions, public MythRender
{
    Q_OBJECT

  public:
    static MythRenderOpenGL* Create(const QString &Painter, QPaintDevice* Device = nullptr);
    static bool IsEGL(void);
    MythRenderOpenGL(const QSurfaceFormat &Format, QPaintDevice* Device, RenderType Type = kRenderOpenGL);

    // MythRender
    void Release(void) override;

    // These functions are not virtual in the base QOpenGLContext
    // class, so these are not overrides but new functions.
    virtual void makeCurrent();
    virtual void doneCurrent();
    virtual void swapBuffers();

    void  setWidget(QWidget *Widget);
    bool  Init(void);
    int   GetMaxTextureSize(void) const;
    int   GetMaxTextureUnits(void) const;
    int   GetExtraFeatures(void) const;
    QOpenGLFunctions::OpenGLFeatures GetFeatures(void) const;
    bool  IsRecommendedRenderer(void);
    void  MoveResizeWindow(const QRect &rect);
    void  SetViewPort(const QRect &rect, bool viewportonly = false);
    QRect GetViewPort(void) { return m_viewport; }
    void  PushTransformation(const UIEffects &fx, QPointF &center);
    void  PopTransformation(void);
    void  Flush(bool use_fence);
    void  SetBlend(bool enable);
    void  SetBackground(int r, int g, int b, int a);
    void  SetFence(void);
    void  DeleteFence(void);

    static const GLuint kVertexSize;
    QOpenGLBuffer* CreateVBO(int Size, bool Release = true);

    MythGLTexture* CreateHelperTexture(void);
    MythGLTexture* CreateTexture(QSize Size,
                        QOpenGLTexture::PixelType     PixelType   = QOpenGLTexture::UInt8,
                        QOpenGLTexture::PixelFormat   PixelFormat = QOpenGLTexture::RGBA,
                        QOpenGLTexture::Filter        Filter      = QOpenGLTexture::Linear,
                        QOpenGLTexture::WrapMode      Wrap        = QOpenGLTexture::ClampToEdge,
                        QOpenGLTexture::TextureFormat Format      = QOpenGLTexture::NoFormat);
    MythGLTexture* CreateTextureFromQImage(QImage *Image);
    MythGLTexture* CreateExternalTexture(QSize Size);
    QSize GetTextureSize(const QSize &size);
    int   GetTextureDataSize(MythGLTexture *Texture);
    void  SetTextureFilters(MythGLTexture *Texture, QOpenGLTexture::Filter Filter, QOpenGLTexture::WrapMode Wrap);
    void  ActiveTexture(GLuint ActiveTex);
    void  EnableTextures(void);
    void  DisableTextures(void);
    void  DeleteTexture(MythGLTexture *Texture);

    QOpenGLFramebufferObject* CreateFramebuffer(QSize &Size);
    MythGLTexture* CreateFramebufferTexture(QOpenGLFramebufferObject *Framebuffer);
    void  DeleteFramebuffer(QOpenGLFramebufferObject *Framebuffer);
    void  BindFramebuffer(QOpenGLFramebufferObject *Framebuffer);
    void  ClearFramebuffer(void);
    void  DiscardFramebuffer(QOpenGLFramebufferObject *Framebuffer);

    QOpenGLShaderProgram* CreateShaderProgram(const QString &Vertex, const QString &Fragment);
    void  DeleteShaderProgram(QOpenGLShaderProgram* Program);
    bool  EnableShaderProgram(QOpenGLShaderProgram* Program);
    void  SetShaderProgramParams(QOpenGLShaderProgram* Program, const QMatrix4x4 &Value, const char* Uniform);

    void  DrawBitmap(MythGLTexture *Texture, QOpenGLFramebufferObject *Target,
                     const QRect &Source, const QRect &Destination,
                     QOpenGLShaderProgram *Program, int Alpha = 255,
                     int Red = 255, int Green = 255, int Blue = 255);
    void  DrawBitmap(MythGLTexture **Textures, uint TextureCount,
                     QOpenGLFramebufferObject *Target,
                     const QRect &Source, const QRect &Destination,
                     QOpenGLShaderProgram *Program);
    void  DrawRect(QOpenGLFramebufferObject *Target,
                   const QRect &Area, const QBrush &FillBrush,
                   const QPen &LinePen, int Alpha);
    void  DrawRoundRect(QOpenGLFramebufferObject *Target,
                        const QRect &Area, int CornerRadius,
                        const QBrush &FillBrush, const QPen &LinePen, int Alpha);
    bool  RectanglesAreAccelerated(void) { return true; }

  public slots:
    void  messageLogged  (const QOpenGLDebugMessage &Message);
    void  logDebugMarker (const QString &Message);

  protected:
    ~MythRenderOpenGL() override;
    void  Init2DState(void);
    void  InitProcs(void);
    QFunctionPointer GetProcAddress(const QString &Proc) const;
    void  SetMatrixView(void);

    void  DeleteOpenGLResources(void);
    void  DeleteFramebuffers(void);

    bool  UpdateTextureVertices(MythGLTexture *Texture, const QRect &Source, const QRect &Destination);
    GLfloat* GetCachedVertices(GLuint Type, const QRect &Area);
    void  ExpireVertices(int Max = 0);
    void  GetCachedVBO(GLuint Type, const QRect &Area);
    void  ExpireVBOS(int Max = 0);
    int   GetBufferSize(QSize Size, QOpenGLTexture::PixelFormat Format, QOpenGLTexture::PixelType Type);

    bool  CreateDefaultShaders(void);
    void  DeleteDefaultShaders(void);

    static void StoreBicubicWeights(float X, float *Dst);

  protected:
    // Prevent compiler complaints about using 0 as a null pointer.
    inline void glVertexAttribPointerI(GLuint Index, GLint Size, GLenum Type,
                                       GLboolean Normalize, GLsizei Stride,
                                       const GLuint Value);
    // Framebuffers
    QOpenGLFramebufferObject    *m_activeFramebuffer;

    // Synchronisation
    GLuint                       m_fence;

    // Shaders
    QOpenGLShaderProgram*        m_defaultPrograms[kShaderCount];
    QOpenGLShaderProgram*        m_activeProgram;

    // Vertices
    QMap<uint64_t,GLfloat*>      m_cachedVertices;
    QList<uint64_t>              m_vertexExpiry;
    QMap<uint64_t,QOpenGLBuffer*>m_cachedVBOS;
    QList<uint64_t>              m_vboExpiry;

    // Locking
    QMutex     m_lock;
    int        m_lockLevel;

    // profile
    QOpenGLFunctions::OpenGLFeatures m_features;
    int        m_extraFeatures;
    int        m_extraFeaturesUsed;
    int        m_maxTextureSize;
    int        m_maxTextureUnits;

    // State
    QRect      m_viewport;
    GLuint     m_activeTexture;
    GLenum     m_activeTextureTarget;
    bool       m_blend;
    int32_t    m_background;
    QMatrix4x4 m_projection;
    QStack<QMatrix4x4> m_transforms;
    QMatrix4x4 m_parameters;
    QHash<QString,QMatrix4x4> m_cachedMatrixUniforms;

    // For Performance improvement set false to disable glFlush.
    // Needed for Raspberry pi
    bool       m_flushEnabled;

    // NV_fence
    MYTH_GLGENFENCESNVPROC               m_glGenFencesNV;
    MYTH_GLDELETEFENCESNVPROC            m_glDeleteFencesNV;
    MYTH_GLSETFENCENVPROC                m_glSetFenceNV;
    MYTH_GLFINISHFENCENVPROC             m_glFinishFenceNV;
    // APPLE_fence
    MYTH_GLGENFENCESAPPLEPROC            m_glGenFencesAPPLE;
    MYTH_GLDELETEFENCESAPPLEPROC         m_glDeleteFencesAPPLE;
    MYTH_GLSETFENCEAPPLEPROC             m_glSetFenceAPPLE;
    MYTH_GLFINISHFENCEAPPLEPROC          m_glFinishFenceAPPLE;
    // Framebuffer discard
    MYTH_GLDISCARDFRAMEBUFFER            m_glDiscardFramebuffer;

  private:
    void DebugFeatures (void);
    QOpenGLDebugLogger *m_openglDebugger;
    QWindow *m_window;
};

class MUI_PUBLIC OpenGLLocker
{
  public:
    explicit OpenGLLocker(MythRenderOpenGL *Render);
   ~OpenGLLocker();
  private:
    MythRenderOpenGL *m_render;
};

#endif
