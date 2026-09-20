#include <trx/gl/gpu_timer.h>

#include <trx/gl/utils.h>

#define M_RING 4

static struct {
    bool active;
    int32_t frame;
    int32_t open_slot;
    GLuint queries[TRX_GL_GPU_TIMER_SLOTS][M_RING];
    bool issued[TRX_GL_GPU_TIMER_SLOTS][M_RING];
    double result_ms[TRX_GL_GPU_TIMER_SLOTS];
} m_Priv = {};

void TRX_GL_GpuTimer_Init(const bool is_enabled)
{
    if (m_Priv.active || !is_enabled) {
        return;
    }
    for (int32_t i = 0; i < TRX_GL_GPU_TIMER_SLOTS; i++) {
        glGenQueries(M_RING, m_Priv.queries[i]);
    }
    m_Priv.open_slot = -1;
    m_Priv.active = true;
    TRX_GL_CheckError();
}

void TRX_GL_GpuTimer_Shutdown(void)
{
    if (!m_Priv.active) {
        return;
    }
    for (int32_t i = 0; i < TRX_GL_GPU_TIMER_SLOTS; i++) {
        glDeleteQueries(M_RING, m_Priv.queries[i]);
    }
    m_Priv = (typeof(m_Priv)) {};
}

void TRX_GL_GpuTimer_Begin(const int32_t slot)
{
    if (!m_Priv.active || slot < 0 || slot >= TRX_GL_GPU_TIMER_SLOTS
        || m_Priv.open_slot != -1) {
        return;
    }
    const int32_t idx = m_Priv.frame % M_RING;
    glBeginQuery(GL_TIME_ELAPSED, m_Priv.queries[slot][idx]);
    m_Priv.issued[slot][idx] = true;
    m_Priv.open_slot = slot;
}

void TRX_GL_GpuTimer_End(void)
{
    if (!m_Priv.active || m_Priv.open_slot == -1) {
        return;
    }
    glEndQuery(GL_TIME_ELAPSED);
    m_Priv.open_slot = -1;
}

void TRX_GL_GpuTimer_Collect(void)
{
    if (!m_Priv.active) {
        return;
    }
    const int32_t idx = (m_Priv.frame + 1) % M_RING;
    for (int32_t slot = 0; slot < TRX_GL_GPU_TIMER_SLOTS; slot++) {
        if (!m_Priv.issued[slot][idx]) {
            m_Priv.result_ms[slot] = 0.0;
            continue;
        }
        GLint ready = 0;
        glGetQueryObjectiv(
            m_Priv.queries[slot][idx], GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready) {
            continue;
        }
        GLuint64 elapsed_ns = 0;
        glGetQueryObjectui64v(
            m_Priv.queries[slot][idx], GL_QUERY_RESULT, &elapsed_ns);
        m_Priv.result_ms[slot] = (double)elapsed_ns / 1000000.0;
        m_Priv.issued[slot][idx] = false;
    }
    m_Priv.frame++;
}

double TRX_GL_GpuTimer_GetMs(const int32_t slot)
{
    if (!m_Priv.active || slot < 0 || slot >= TRX_GL_GPU_TIMER_SLOTS) {
        return 0.0;
    }
    return m_Priv.result_ms[slot];
}
