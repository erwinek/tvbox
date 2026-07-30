#pragma once

#include <cstdint>
#include <string>

namespace ui {

// Minimalny DRM atomic output z rotacja primary plane (i915).
// FB = rozmiar logiczny (portrait), CRTC mode = landscape EDID, rotation = 90/270.
class DrmAtomicOutput {
public:
    DrmAtomicOutput() = default;
    ~DrmAtomicOutput();

    DrmAtomicOutput(const DrmAtomicOutput&) = delete;
    DrmAtomicOutput& operator=(const DrmAtomicOutput&) = delete;

    // rotate_ccw: 90 lub 270 (DRM_MODE_ROTATE_*). prefer_* = preferowany tryb (np. 1920x1080).
    bool Init(int rotate_ccw, int prefer_mode_w = 1920, int prefer_mode_h = 1080);
    void Shutdown();

    bool ok() const { return fd_ >= 0 && fb_id_[0] != 0; }
    int logical_w() const { return fb_w_; }
    int logical_h() const { return fb_h_; }
    int mode_w() const { return mode_w_; }
    int mode_h() const { return mode_h_; }
    int rotate_ccw() const { return rotate_ccw_; }

    // pixels = XRGB8888 / ARGB8888, pitch w bajtach. Kopiuje do backbuffer i flip.
    bool Present(const void* pixels, int pitch);

private:
    struct Bo {
        uint32_t handle = 0;
        uint32_t pitch = 0;
        uint64_t size = 0;
        void* map = nullptr;
        uint32_t fb_id = 0;
    };

    bool CreateBo(Bo& bo, int w, int h);
    bool CreateBoGbm(int index, int w, int h);
    void DestroyBo(Bo& bo);
    bool AtomicModeset(uint32_t fb_id, bool allow_modeset);
    uint32_t GetPropId(uint32_t obj_id, uint32_t obj_type, const char* name);
    static std::string ErrnoStr();

    int fd_ = -1;
    uint32_t connector_id_ = 0;
    uint32_t crtc_id_ = 0;
    uint32_t plane_id_ = 0;
    uint32_t blob_id_ = 0;

    uint32_t prop_crtc_id_ = 0;
    uint32_t prop_fb_id_ = 0;
    uint32_t prop_src_x_ = 0;
    uint32_t prop_src_y_ = 0;
    uint32_t prop_src_w_ = 0;
    uint32_t prop_src_h_ = 0;
    uint32_t prop_crtc_x_ = 0;
    uint32_t prop_crtc_y_ = 0;
    uint32_t prop_crtc_w_ = 0;
    uint32_t prop_crtc_h_ = 0;
    uint32_t prop_rotation_ = 0;
    uint32_t prop_mode_id_ = 0;
    uint32_t prop_active_ = 0;
    uint32_t prop_conn_crtc_ = 0;

    int rotate_ccw_ = 0;
    int mode_w_ = 0;
    int mode_h_ = 0;
    int fb_w_ = 0;
    int fb_h_ = 0;
    uint64_t rotation_val_ = 0;

    Bo bo_[2]{};
    uint32_t fb_id_[2]{0, 0};
    int front_ = 0;
    bool mode_set_ = false;
};

}  // namespace ui
