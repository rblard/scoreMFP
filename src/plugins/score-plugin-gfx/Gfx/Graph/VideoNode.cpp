#include <Gfx/Graph/VideoNode.hpp>
#include <Gfx/Graph/decoders/GPUVideoDecoder.hpp>
#include <Gfx/Graph/decoders/HAP.hpp>
#include <Gfx/Graph/decoders/RGBA.hpp>
#include <Gfx/Graph/decoders/YUV420.hpp>
#include <Gfx/Graph/decoders/YUV422.hpp>
#include <Gfx/Graph/decoders/YUYV422.hpp>

#include <ossia/detail/flicks.hpp>

#include <QElapsedTimer>

namespace score::gfx
{
struct VideoNode::Rendered : GenericNodeRenderer
{
  using GenericNodeRenderer::GenericNodeRenderer;
  std::unique_ptr<GPUVideoDecoder> gpu;
  std::shared_ptr<Video::VideoInterface> decoder;
  std::vector<AVFrame*> framesToFree;
  AVPixelFormat current_format = AVPixelFormat(-1);
  int current_width{}, current_height{};
  QElapsedTimer t;

  Rendered(const VideoNode& node) noexcept
      : GenericNodeRenderer{node}
      , decoder{node.m_decoder} // TODO clone. But how to do for camera, etc. ?
      , current_format{decoder->pixel_format}
      , current_width{decoder->width}
      , current_height{decoder->height}
  {
  }

  ~Rendered()
  {
    auto& decoder = *static_cast<const VideoNode&>(node).m_decoder;
    for (auto frame : framesToFree)
      decoder.release_frame(frame);
  }

  void createGpuDecoder()
  {
    auto& model = (VideoNode&)(node);
    auto& filter = model.m_filter;
    switch (current_format)
    {
      case AV_PIX_FMT_YUV420P:
        gpu = std::make_unique<YUV420Decoder>(model, *decoder);
        break;
      case AV_PIX_FMT_YUVJ422P:
      case AV_PIX_FMT_YUV422P:
        gpu = std::make_unique<YUV422Decoder>(model, *decoder);
        break;
      case AV_PIX_FMT_UYVY422:
        gpu = std::make_unique<UYVY422Decoder>(model, *decoder);
        break;
      case AV_PIX_FMT_YUYV422:
        gpu = std::make_unique<YUYV422Decoder>(model, *decoder);
        break;
      case AV_PIX_FMT_RGB0:
      case AV_PIX_FMT_RGBA:
        gpu = std::make_unique<RGB0Decoder>(
            QRhiTexture::RGBA8, model, *decoder, filter);
        break;
      case AV_PIX_FMT_BGR0:
      case AV_PIX_FMT_BGRA:
        gpu = std::make_unique<RGB0Decoder>(
            QRhiTexture::BGRA8, model, *decoder, filter);
        break;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 19, 100)
      case AV_PIX_FMT_GRAYF32LE:
      case AV_PIX_FMT_GRAYF32BE:
        gpu = std::make_unique<RGB0Decoder>(
            QRhiTexture::R32F, model, *decoder, filter);
        break;
#endif
      case AV_PIX_FMT_GRAY8:
        gpu = std::make_unique<RGB0Decoder>(
            QRhiTexture::R8, model, *decoder, filter);
        break;
      default:
      {
        // try to read format as a 4cc
        std::string_view fourcc{(const char*)&current_format, 4};

        if (fourcc == "Hap1")
          gpu = std::make_unique<HAPDefaultDecoder>(
              QRhiTexture::BC1, model, *decoder, filter);
        else if (fourcc == "Hap5")
          gpu = std::make_unique<HAPDefaultDecoder>(
              QRhiTexture::BC3, model, *decoder, filter);
        else if (fourcc == "HapY")
          gpu = std::make_unique<HAPDefaultDecoder>(
              QRhiTexture::BC3,
              model,
              *decoder,
              HAPDefaultDecoder::ycocg_filter + filter);
        else if (fourcc == "HapM")
          gpu = std::make_unique<HAPMDecoder>(model, *decoder, filter);
        else if (fourcc == "HapA")
          gpu = std::make_unique<HAPDefaultDecoder>(
              QRhiTexture::BC4, model, *decoder, filter);
        else if (fourcc == "Hap7")
          gpu = std::make_unique<HAPDefaultDecoder>(
              QRhiTexture::BC7, model, *decoder, filter);

        if (!gpu)
        {
          qDebug() << "Unhandled pixel format: "
                   << av_get_pix_fmt_name(current_format);
          gpu = std::make_unique<EmptyDecoder>(model);
        }
        break;
      }
    }
  }

  void setupGpuDecoder(RenderList& r)
  {
    if (gpu)
    {
      gpu->release(r, *this);
      for (auto sampler : m_samplers)
      {
        delete sampler.sampler;
      }
      m_samplers.clear();

      delete m_p.pipeline;
      m_p.pipeline = nullptr;
    }
    createGpuDecoder();

    if (gpu)
    {
      gpu->init(r, *this);
      m_p = score::gfx::buildPipeline(
          r,
          node.mesh(),
          node.m_vertexS,
          node.m_fragmentS,
          m_rt,
          m_processUBO,
          m_materialUBO,
          m_samplers);
    }
  }

  void checkFormat(RenderList& r, AVPixelFormat fmt, int w, int h)
  {
    // TODO won't work if VK is threaded and there are multiple windows
    if (!gpu || fmt != current_format || w != current_width
        || h != current_height)
    {
      current_format = fmt;
      current_width = w;
      current_height = h;
      setupGpuDecoder(r);
    }
  }

  void customInit(RenderList& renderer) override
  {
    defaultShaderMaterialInit(renderer);

    if (!gpu)
    {
      createGpuDecoder();
    }
    if (gpu)
    {
      gpu->init(renderer, *this);
    }
  }

  // TODO if we have multiple renderers for the same video, we must always keep
  // a frame because rendered may have different rates, so we cannot know "when"
  // all renderers have rendered, thue the pattern in the following function
  // is not enough
  double lastFrameTime{};
  double lastPlaybackTime{-1.};
  void
  customUpdate(RenderList& renderer, QRhiResourceUpdateBatch& res) override
  {
    auto& nodem = const_cast<VideoNode&>(static_cast<const VideoNode&>(node));
    auto& decoder = *nodem.m_decoder;
    for (auto frame : framesToFree)
      decoder.release_frame(frame);
    framesToFree.clear();

    // TODO
    auto mustReadFrame = [this, &decoder, &nodem] {
      double tempoRatio = 1.;
      if (nodem.m_nativeTempo)
        tempoRatio = (*nodem.m_nativeTempo) / 120.;

      auto current_time = nodem.standardUBO.time * tempoRatio; // In seconds
      auto next_frame_time = lastFrameTime;

      // pause
      if (nodem.standardUBO.time == lastPlaybackTime)
      {
        return false;
      }
      lastPlaybackTime = nodem.standardUBO.time;

      // what more can we do ?
      const double inv_fps
          = decoder.fps > 0 ? 1. / (tempoRatio * decoder.fps) : 1. / 24.;
      next_frame_time += inv_fps;

      const bool we_are_late = current_time > next_frame_time;
      const bool timer = t.elapsed() > (1000. * inv_fps);

      //const bool we_are_in_advance = std::abs(current_time - next_frame_time) > (2. * inv_fps);
      //const bool seeked = nodem.seeked.exchange(false);
      //const bool seeked_forward =
      return we_are_late || timer;
    };

    if (decoder.realTime || mustReadFrame())
    {
      if (auto frame = decoder.dequeue_frame())
      {
        checkFormat(
            renderer,
            static_cast<AVPixelFormat>(frame->format),
            frame->width,
            frame->height);
        if (gpu)
        {
          gpu->exec(renderer, *this, res, *frame);
        }

        int64_t ts = frame->best_effort_timestamp;
        lastFrameTime
            = (decoder.flicks_per_dts * ts) / ossia::flicks_per_second<double>;

        //qDebug() << lastFrameTime << node.standardUBO.time;
        //qDebug() << (lastFrameTime - nodem.standardUBO.time);

        framesToFree.push_back(frame);
      }
      t.restart();
    }
  }

  void customRelease(RenderList& r) override
  {
    if (gpu)
      gpu->release(r, *this);
  }
};

VideoNode::VideoNode(
    std::shared_ptr<Video::VideoInterface> dec,
    std::optional<double> nativeTempo,
    QString f)
    : m_decoder{std::move(dec)}
    , m_nativeTempo{nativeTempo}
    , m_filter{f}
{
  output.push_back(new Port{this, {}, Types::Image, {}});
}

const Mesh& VideoNode::mesh() const noexcept
{
  return this->m_mesh;
}

VideoNode::~VideoNode() { }

score::gfx::NodeRenderer*
VideoNode::createRenderer(RenderList& r) const noexcept
{
  return new Rendered{*this};
}

void VideoNode::seeked()
{
  SCORE_TODO;
}
}
#include <hap/source/hap.c>
