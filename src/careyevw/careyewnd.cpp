#include <qdebug.h>

#include <opencv2/imgproc/imgproc.hpp>
#if CV_MAJOR_VERSION >= 4
#define CV_BGR2RGB  cv::COLOR_BGR2RGB
#endif

#include <VX/vx.h>
#include <NVX/nvx_timer.hpp>

#ifdef WIN32
#include "NVXIO/Application.hpp"
#include "NVXIO/FrameSource.hpp"
#include "NVXIO/Render.hpp"
#include "NVXIO/SyncTimer.hpp"
#include "NVXIO/Utility.hpp"

#define vxio	nvxio

#else
#include "NVX/Application.hpp"
#include "OVX/FrameSourceOVX.hpp"
#include "OVX/RenderOVX.hpp"
#include "NVX/SyncTimer.hpp"
#include "OVX/UtilityOVX.hpp"

#define vxio	ovxio

#endif

#include "careyeapp.h"
#include "careyewnd.h"
#include "careyethread.h"

struct EventData
{
    EventData() : alive(true), pause(false) {}

    bool alive;
    bool pause;
};

struct NvxioData
{
    vxio::FrameSource::Parameters config;
    vxio::ContextGuard context;
    EventData eventData;
    vx_image frame;
    std::unique_ptr<nvxio::SyncTimer> syncTimer;
    std::unique_ptr<vxio::FrameSource> converter;
    std::unique_ptr<vxio::Render> render;
    vxio::Render::TextBoxStyle style;
};

class CarEyeWndPrivate {
public:
    CarEyeInferRes inferRes;
    NvxioData nvxio;
};

static void keyboardEventCallback(void* context, vx_char key, vx_uint32 /*x*/, vx_uint32 /*y*/)
{
    EventData* eventData = static_cast<EventData*>(context);
    if (key == 27) // escape
    {
	    eventData->alive = false;
    }
    else if (key == 32)
    {
	    eventData->pause = !eventData->pause;
    }
}

static void MouseEventCallback(void* context, ovxio::Render::MouseButtonEvent event, uint32_t x, uint32_t y)
{
	EventData* eventData = static_cast<EventData*>(context);
	if(event == ovxio::Render::LeftButtonDown)
	{
		eventData->alive = false;
	}

}

int init_nvxio(nvxio::Application &app, NvxioData &nvxio)
{
    try
    {
        nvxio.config.frameWidth = g_conf->value(CONF_CAMERA_WIDTH).toInt();
        nvxio.config.frameHeight = g_conf->value(CONF_CAMERA_HEIGHT).toInt();
        nvxio.config.format = VX_DF_IMAGE_RGB;
        nvxio.config.fps = 25;
        nvxio.style = { { 255, 255, 255, 255 }, { 0, 0, 0, 64 }, { 0, 0 } };

        vxRegisterLogCallback(nvxio.context, &vxio::stdoutLogCallback, vx_false_e);

        std::string input = "device:///v4l2?index=-1";
        std::unique_ptr<vxio::FrameSource> converter(vxio::createDefaultFrameSource(nvxio.context, input));
        nvxio.converter = std::move(converter);
        nvxio.converter->setConfiguration(nvxio.config);

        bool full_screen = g_conf->value(CONF_FULL_SCREEN).toBool();

        if (!full_screen) {
            std::unique_ptr<vxio::Render> render(vxio::createDefaultRender(
                nvxio.context, "CarEye", nvxio.config.frameWidth, nvxio.config.frameHeight));
            nvxio.render = std::move(render);
        } else {
            //parameter: --nvxio_fullscreen
            std::unique_ptr<vxio::Render> render(vxio::createDefaultRender(
                nvxio.context, "CarEye", nvxio.config.frameWidth, nvxio.config.frameHeight,
                VX_DF_IMAGE_RGBX, true, true));
            nvxio.render = std::move(render);
        }
        if (!nvxio.render)
        {
            std::cout << "Error: Cannot open default render!" << std::endl;
            return nvxio::Application::APP_EXIT_CODE_NO_RENDER;
        }
        nvxio.render->setOnKeyboardEventCallback(keyboardEventCallback, &nvxio.eventData);
        nvxio.render->setOnMouseEventCallback(MouseEventCallback, &nvxio.eventData);
        nvxio.frame = vxCreateImage(nvxio.context, nvxio.config.frameWidth,
            nvxio.config.frameHeight, nvxio.config.format);
        NVXIO_CHECK_REFERENCE(nvxio.frame);

        nvxio.syncTimer = nvxio::createSyncTimer();
        nvxio.syncTimer->arm(1. / app.getFPSLimit());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

static void putFps(std::unique_ptr<vxio::Render>& render, vxio::Render::TextBoxStyle &style, double total_ms)
{
    std::ostringstream txt;
    txt << std::fixed << std::setprecision(1);
    txt << 1000.0 / total_ms << " FPS" << std::endl;
    render->putTextViewport(txt.str(), style);
}

static void putInferRes(std::unique_ptr<vxio::Render>& render, const std::vector<ce::InferInfo> &infos)
{
    char str[128];
    vx_rectangle_t location;
    vxio::Render::DetectedObjectStyle style1 = { "", { 36, 255, 36, 255 }, 1, 0, false };
    vxio::Render::DetectedObjectStyle style2 = { "", { 255, 36, 36, 255 }, 1, 0, false };
    char itoabuff[16];
    float collisionThreshold = g_conf->value(CONF_COLLISION_THRESHOLD).toFloat();

    for (int i = 0; i < infos.size(); ++i) {
        const ce::InferInfo &info = infos[i];
        const ce::DetInfo &det = info.left;

#if 1
        if (info.distance>0.001)
            sprintf(str, "%.1lfm", info.distance);
        else
            str[0] = 0;
#endif
        location.start_x = det.x;
        location.start_y = det.y;
        location.end_x = det.x + det.width;
        location.end_y = det.y + det.height;
        //style.label = info.name + "(" + itoa((int)info.id, itoabuff, 10) + ")";
        vxio::Render::DetectedObjectStyle &style = (info.collisionTime<0.01 || info.collisionTime > collisionThreshold) ? style1 : style2;
        style.label = info.name + " ";
        style.label += str;	
        render->putObjectLocation(location, style);
#if 0
        if (g_conf->value(CONF_VELOCITY_SHOW).toInt()>0 && info.velocity > 0.1) {
            sprintf(str, "%.1lfkm/h", info.velocity);
            vxio::Render::TextBoxStyle ts = { { 255, 255, 255, 255 }, { 0, 0, 0, 64 }, { det.x, det.y } };
            render->putTextViewport(str, ts);
        }
#endif
#if 0
        if (info.collisionTime > 0.00001) {
            sprintf(str, "%.4lfs", info.collisionTime);
            vxio::Render::TextBoxStyle ts = { { 255, 255, 255, 255 }, { 0, 0, 0, 64 }, { det.x, det.y+20 } };
            render->putTextViewport(str, ts);
        }
#endif

#if CE_DEBUG
        if (!info.left.image.empty()) {
            cv::Mat keyimg;
            info.left.image.copyTo(keyimg);
            cv::drawKeypoints(keyimg, info.left.keyPoints, keyimg, cv::Scalar(255, 0, 0));
            cv::imshow("key_image", keyimg);
            cv::waitKey(10);
        }
#endif
    }
}

CarEyeWnd::CarEyeWnd(QObject *parent)
    : QThread(parent)
{
    m_d = new CarEyeWndPrivate();
}

CarEyeWnd::~CarEyeWnd()
{
    if (m_d) {
        delete m_d;
        m_d = NULL;
    }
}

void CarEyeWnd::run()
{
#if 1
    int camWidth = g_conf->value(CONF_CAMERA_WIDTH).toInt();
    int camHeight = g_conf->value(CONF_CAMERA_HEIGHT).toInt();
    while (1) {
        msleep(10);
        if (!g_app->engineThread())
            continue;
        if (!g_app->engineThread()->getInferRes(m_d->inferRes, camWidth, camHeight))
            continue;
        break;
    }
#endif

    nvxio::Application &app = nvxio::Application::get();
    if (init_nvxio(app, m_d->nvxio)) {
        //return nvxio::Application::APP_EXIT_CODE_ERROR;
        return;
    }
    try
    {
        BinocsFrame frame;
        int ret = 0;

        nvx::Timer totalTimer;
        totalTimer.tic();

        while (m_d->nvxio.eventData.alive)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            vxio::FrameSource::FrameStatus status = vxio::FrameSource::OK;

            if (!m_d->nvxio.eventData.pause) {

                if (!g_app->engineThread())
                    continue;
                if (!g_app->engineThread()->getInferRes(m_d->inferRes, m_d->nvxio.config.frameWidth, m_d->nvxio.config.frameHeight))
                    continue;

                cv::Mat imageconv;
                cv::cvtColor(m_d->inferRes.frame, imageconv, CV_BGR2RGB);
                m_d->nvxio.converter->convert(imageconv.cols, imageconv.rows, imageconv.channels(),
                    (char*)imageconv.data, (int)imageconv.step[0], m_d->nvxio.frame);
            }
            double total_ms = totalTimer.toc();
            totalTimer.tic();

            m_d->nvxio.render->putImage(m_d->nvxio.frame);

            putInferRes(m_d->nvxio.render, m_d->inferRes.infers);
            putFps(m_d->nvxio.render, m_d->nvxio.style, total_ms);

            if (!m_d->nvxio.render->flush())
                m_d->nvxio.eventData.alive = false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        //return nvxio::Application::APP_EXIT_CODE_ERROR;
        return;
    }

    vxReleaseImage(&m_d->nvxio.frame);
    m_d->nvxio.render->close();

    //return nvxio::Application::APP_EXIT_CODE_SUCCESS;
}
