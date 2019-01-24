#include "mythlogging.h"

#include "mythmainwindow.h"

#ifdef USING_OPENGL
#include "mythrender_opengl.h"
#endif

#ifdef USING_VDPAU
#include "mythrender_vdpau.h"
#endif

extern "C" {
#include "goom/goom_tools.h"
#include "goom/goom_core.h"
}

#include "videovisualgoom.h"

VideoVisualGoom::VideoVisualGoom(AudioPlayer *audio, MythRender *render, bool hd)
  : VideoVisual(audio, render), m_buffer(nullptr), m_vdpauSurface(0), m_glSurface(nullptr), m_hd(hd)
{
    int max_width  = m_hd ? 1200 : 600;
    int max_height = m_hd ? 800  : 400;
    MythMainWindow *mw = GetMythMainWindow();
    QSize sz = mw ? mw->GetUIScreenRect().size() : QSize(600, 400);
    int width  = (sz.width() > max_width)   ? max_width  : sz.width();
    int height = (sz.height() > max_height) ? max_height : sz.height();
    m_area = QRect(0, 0, width, height);
    goom_init(width, height, 0);
    LOG(VB_GENERAL, LOG_INFO, QString("Initialised Goom (%1x%2)")
            .arg(width).arg(height));
}

VideoVisualGoom::~VideoVisualGoom()
{
#ifdef USING_OPENGL
    if (m_glSurface && m_render && (m_render->Type() == kRenderOpenGL))
    {
        MythRenderOpenGL *glrender = static_cast<MythRenderOpenGL*>(m_render);
        if (glrender)
            glrender->DeleteTexture(m_glSurface);
        m_glSurface = nullptr;
    }
#endif

#ifdef USING_VDPAU
    if (m_vdpauSurface && m_render &&
       (m_render->Type() == kRenderVDPAU))
    {
        MythRenderVDPAU *render = static_cast<MythRenderVDPAU*>(m_render);
        if (render)
            render->DestroyBitmapSurface(m_vdpauSurface);
        m_vdpauSurface = 0;
    }
#endif

    goom_close();
}

void VideoVisualGoom::Draw(const QRect &area, MythPainter */*painter*/,
                           QPaintDevice */*device*/)
{
    if (m_disabled || !m_render || area.isEmpty())
        return;

    QMutexLocker lock(mutex());
    unsigned int* last = m_buffer;
    VisualNode *node = GetNode();
    if (node)
    {
        int numSamps = 512;
        if (node->length < 512)
            numSamps = node->length;

        signed short int data[2][512];
        int i= 0;
        for (; i < numSamps; i++)
        {
            data[0][i] = node->left[i];
            data[1][i] = node->right ? node->right[i] : data[0][i];
        }

        for (; i < 512; i++)
        {
            data[0][i] = 0;
            data[1][i] = 0;
        }

        m_buffer = goom_update(data, 0);
    }

#ifdef USING_OPENGL
    if ((m_render->Type() == kRenderOpenGL))
    {
        MythRenderOpenGL *glrender = static_cast<MythRenderOpenGL*>(m_render);
        if (!m_glSurface && glrender && m_buffer)
        {
            m_glSurface = glrender->CreateTexture(m_area.size(), true);
        }

        if (m_glSurface && glrender && m_buffer)
        {
            if (m_buffer != last)
            {
                void* buf = glrender->GetTextureBuffer(m_glSurface, false);
                if (buf)
                    memcpy(buf, m_buffer, m_area.width() * m_area.height() * 4);
                glrender->UpdateTexture(m_glSurface, (void*)m_buffer);
            }
            glrender->DrawBitmap(&m_glSurface, 1, nullptr, m_area, area, nullptr);
        }
        return;
    }
#endif

#ifdef USING_VDPAU
    if (m_render->Type() == kRenderVDPAU)
    {
        MythRenderVDPAU *render = static_cast<MythRenderVDPAU*>(m_render);

        if (!m_vdpauSurface && render)
            m_vdpauSurface = render->CreateBitmapSurface(m_area.size());

        if (m_vdpauSurface && render && m_buffer)
        {
            if (m_buffer != last)
            {
                void    *plane[1] = { m_buffer };
                uint32_t pitch[1] = { static_cast<uint32_t>(m_area.width() * 4) };
                render->UploadBitmap(m_vdpauSurface, plane, pitch);
            }
            render->DrawBitmap(m_vdpauSurface, 0, nullptr, nullptr, kVDPBlendNull, 255, 255, 255, 255);
        }
        return;
    }
#endif
}

static class VideoVisualGoomFactory : public VideoVisualFactory
{
  public:
    const QString &name(void) const override // VideoVisualFactory
    {
        static QString name("Goom");
        return name;
    }

    VideoVisual *Create(AudioPlayer *audio,
                        MythRender  *render) const override // VideoVisualFactory
    {
        return new VideoVisualGoom(audio, render, false);
    }

    bool SupportedRenderer(RenderType type) override // VideoVisualFactory
    {
        return (type == kRenderVDPAU ||
                type == kRenderOpenGL);
    }
} VideoVisualGoomFactory;

static class VideoVisualGoomHDFactory : public VideoVisualFactory
{
  public:
    const QString &name(void) const override // VideoVisualFactory
    {
        static QString name("Goom HD");
        return name;
    }

    VideoVisual *Create(AudioPlayer *audio,
                        MythRender  *render) const override // VideoVisualFactory
    {
        return new VideoVisualGoom(audio, render, true);
    }

    bool SupportedRenderer(RenderType type) override // VideoVisualFactory
    {
        return (type == kRenderVDPAU ||
                type == kRenderOpenGL);
    }
} VideoVisualGoomHDFactory;
