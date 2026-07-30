#include "ui/DrmAtomicOutput.h"

#include "util/Logger.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <drm_mode.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#ifndef I915_FORMAT_MOD_Y_TILED
#define I915_FORMAT_MOD_Y_TILED fourcc_mod_code(INTEL, 1)
#endif
#ifndef I915_FORMAT_MOD_Yf_TILED
#define I915_FORMAT_MOD_Yf_TILED fourcc_mod_code(INTEL, 2)
#endif

namespace ui {

namespace {

constexpr uint64_t kRotate0 = DRM_MODE_ROTATE_0;
constexpr uint64_t kRotate90 = DRM_MODE_ROTATE_90;
constexpr uint64_t kRotate180 = DRM_MODE_ROTATE_180;
constexpr uint64_t kRotate270 = DRM_MODE_ROTATE_270;

}  // namespace

// Globalny stan GBM — jeden output kiosk.
struct DrmAtomicGbm {
    gbm_device* dev = nullptr;
    gbm_bo* bo[2]{nullptr, nullptr};
    void* map_data[2]{nullptr, nullptr};
};
static DrmAtomicGbm g_gbm;

DrmAtomicOutput::~DrmAtomicOutput() {
    Shutdown();
}

std::string DrmAtomicOutput::ErrnoStr() {
    return std::string(std::strerror(errno));
}

uint32_t DrmAtomicOutput::GetPropId(uint32_t obj_id, uint32_t obj_type, const char* name) {
    drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(fd_, obj_id, obj_type);
    if (!props) {
        return 0;
    }
    uint32_t found = 0;
    for (uint32_t i = 0; i < props->count_props; ++i) {
        drmModePropertyPtr p = drmModeGetProperty(fd_, props->props[i]);
        if (!p) {
            continue;
        }
        if (std::strcmp(p->name, name) == 0) {
            found = p->prop_id;
        }
        drmModeFreeProperty(p);
        if (found) {
            break;
        }
    }
    drmModeFreeObjectProperties(props);
    return found;
}

bool DrmAtomicOutput::CreateBo(Bo& bo, int w, int h) {
    // Legacy dumb (linear) — tylko rotacja 0.
    drm_mode_create_dumb cre{};
    cre.width = static_cast<uint32_t>(w);
    cre.height = static_cast<uint32_t>(h);
    cre.bpp = 32;
    if (drmIoctl(fd_, DRM_IOCTL_MODE_CREATE_DUMB, &cre) != 0) {
        util::Log(util::LogLevel::Error, "DRM CREATE_DUMB: " + ErrnoStr());
        return false;
    }
    bo.handle = cre.handle;
    bo.pitch = cre.pitch;
    bo.size = cre.size;

    uint32_t handles[4] = {bo.handle, 0, 0, 0};
    uint32_t pitches[4] = {bo.pitch, 0, 0, 0};
    uint32_t offsets[4] = {0, 0, 0, 0};
    if (drmModeAddFB2(fd_, w, h, DRM_FORMAT_XRGB8888, handles, pitches, offsets, &bo.fb_id, 0) !=
        0) {
        util::Log(util::LogLevel::Error, "DRM AddFB2: " + ErrnoStr());
        DestroyBo(bo);
        return false;
    }

    drm_mode_map_dumb map{};
    map.handle = bo.handle;
    if (drmIoctl(fd_, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
        util::Log(util::LogLevel::Error, "DRM MAP_DUMB: " + ErrnoStr());
        DestroyBo(bo);
        return false;
    }
    bo.map = mmap(nullptr, static_cast<size_t>(bo.size), PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                  static_cast<off_t>(map.offset));
    if (bo.map == MAP_FAILED) {
        bo.map = nullptr;
        util::Log(util::LogLevel::Error, "DRM mmap: " + ErrnoStr());
        DestroyBo(bo);
        return false;
    }
    std::memset(bo.map, 0, static_cast<size_t>(bo.size));
    return true;
}

bool DrmAtomicOutput::CreateBoGbm(int index, int w, int h) {
    if (!g_gbm.dev) {
        g_gbm.dev = gbm_create_device(fd_);
        if (!g_gbm.dev) {
            util::Log(util::LogLevel::Error, "gbm_create_device failed");
            return false;
        }
    }

    const uint64_t mods[] = {I915_FORMAT_MOD_Y_TILED, I915_FORMAT_MOD_Yf_TILED};
    gbm_bo* bo = gbm_bo_create_with_modifiers(g_gbm.dev, static_cast<uint32_t>(w),
                                              static_cast<uint32_t>(h), GBM_FORMAT_XRGB8888, mods,
                                              2);
    if (!bo) {
        util::Log(util::LogLevel::Warn, "gbm_bo_create_with_modifiers failed, trying flags");
        bo = gbm_bo_create(g_gbm.dev, static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                           GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }
    if (!bo) {
        util::Log(util::LogLevel::Error, "gbm_bo_create failed");
        return false;
    }

    const int planes = gbm_bo_get_plane_count(bo);
    uint32_t handles[4] = {};
    uint32_t strides[4] = {};
    uint32_t offsets[4] = {};
    uint64_t modifiers[4] = {};
    for (int p = 0; p < planes && p < 4; ++p) {
        handles[p] = gbm_bo_get_handle_for_plane(bo, p).u32;
        strides[p] = gbm_bo_get_stride_for_plane(bo, p);
        offsets[p] = gbm_bo_get_offset(bo, p);
        modifiers[p] = gbm_bo_get_modifier(bo);
    }

    uint32_t fb_id = 0;
    const int add = drmModeAddFB2WithModifiers(
        fd_, w, h, DRM_FORMAT_XRGB8888, handles, strides, offsets, modifiers, &fb_id,
        DRM_MODE_FB_MODIFIERS);
    if (add != 0) {
        util::Log(util::LogLevel::Error,
                  "AddFB2WithModifiers failed (" + ErrnoStr() + "), mod=0x" +
                      std::to_string(modifiers[0]));
        gbm_bo_destroy(bo);
        return false;
    }

    g_gbm.bo[index] = bo;
    bo_[index].fb_id = fb_id;
    bo_[index].pitch = strides[0];
    bo_[index].handle = handles[0];
    bo_[index].map = nullptr;  // map on demand via gbm_bo_map
    bo_[index].size = 0;

    util::Log(util::LogLevel::Info,
              "GBM BO[" + std::to_string(index) + "] " + std::to_string(w) + "x" +
                  std::to_string(h) + " modifier=0x" + std::to_string(modifiers[0]));
    return true;
}

void DrmAtomicOutput::DestroyBo(Bo& bo) {
    if (bo.map && bo.size) {
        munmap(bo.map, static_cast<size_t>(bo.size));
        bo.map = nullptr;
    }
    if (bo.fb_id) {
        drmModeRmFB(fd_, bo.fb_id);
        bo.fb_id = 0;
    }
    if (bo.handle && bo.size) {
        // dumb buffer
        drm_mode_destroy_dumb des{};
        des.handle = bo.handle;
        drmIoctl(fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &des);
    }
    bo.handle = 0;
    bo.pitch = 0;
    bo.size = 0;
}

void DrmAtomicOutput::Shutdown() {
    if (fd_ < 0) {
        return;
    }
    for (int i = 0; i < 2; ++i) {
        if (g_gbm.map_data[i]) {
            if (g_gbm.bo[i]) {
                gbm_bo_unmap(g_gbm.bo[i], g_gbm.map_data[i]);
            }
            g_gbm.map_data[i] = nullptr;
        }
        if (g_gbm.bo[i]) {
            if (bo_[i].fb_id) {
                drmModeRmFB(fd_, bo_[i].fb_id);
                bo_[i].fb_id = 0;
            }
            gbm_bo_destroy(g_gbm.bo[i]);
            g_gbm.bo[i] = nullptr;
        } else {
            DestroyBo(bo_[i]);
        }
    }
    fb_id_[0] = fb_id_[1] = 0;
    if (g_gbm.dev) {
        gbm_device_destroy(g_gbm.dev);
        g_gbm.dev = nullptr;
    }
    if (blob_id_) {
        drmModeDestroyPropertyBlob(fd_, blob_id_);
        blob_id_ = 0;
    }
    drmDropMaster(fd_);
    close(fd_);
    fd_ = -1;
    mode_set_ = false;
}

bool DrmAtomicOutput::AtomicModeset(uint32_t fb_id, bool allow_modeset) {
    drmModeAtomicReqPtr req = drmModeAtomicAlloc();
    if (!req) {
        return false;
    }

    auto add = [&](uint32_t obj, uint32_t prop, uint64_t val) -> bool {
        if (!prop) {
            return false;
        }
        return drmModeAtomicAddProperty(req, obj, prop, val) >= 0;
    };

    bool ok = true;
    ok = ok && add(connector_id_, prop_conn_crtc_, crtc_id_);
    ok = ok && add(crtc_id_, prop_mode_id_, blob_id_);
    ok = ok && add(crtc_id_, prop_active_, 1);

    ok = ok && add(plane_id_, prop_crtc_id_, crtc_id_);
    ok = ok && add(plane_id_, prop_fb_id_, fb_id);
    ok = ok && add(plane_id_, prop_src_x_, 0);
    ok = ok && add(plane_id_, prop_src_y_, 0);
    ok = ok && add(plane_id_, prop_src_w_, static_cast<uint64_t>(fb_w_) << 16);
    ok = ok && add(plane_id_, prop_src_h_, static_cast<uint64_t>(fb_h_) << 16);
    ok = ok && add(plane_id_, prop_crtc_x_, 0);
    ok = ok && add(plane_id_, prop_crtc_y_, 0);
    ok = ok && add(plane_id_, prop_crtc_w_, static_cast<uint64_t>(mode_w_));
    ok = ok && add(plane_id_, prop_crtc_h_, static_cast<uint64_t>(mode_h_));
    ok = ok && add(plane_id_, prop_rotation_, rotation_val_);

    if (!ok) {
        drmModeAtomicFree(req);
        util::Log(util::LogLevel::Error, "DRM atomic: missing properties");
        return false;
    }

    // Modeset: ALLOW_MODESET + test. Flip: blocking (vsync), bez NONBLOCK.
    uint32_t flags = 0;
    if (allow_modeset) {
        flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
        const int test =
            drmModeAtomicCommit(fd_, req, flags | DRM_MODE_ATOMIC_TEST_ONLY, nullptr);
        if (test != 0) {
            util::Log(util::LogLevel::Error,
                      "DRM atomic TEST failed (rot=" + std::to_string(rotate_ccw_) +
                          "): " + ErrnoStr());
            drmModeAtomicFree(req);
            return false;
        }
    }

    const int ret = drmModeAtomicCommit(fd_, req, flags, nullptr);
    drmModeAtomicFree(req);
    if (ret != 0) {
        util::Log(util::LogLevel::Error, "DRM atomic COMMIT failed: " + ErrnoStr());
        return false;
    }
    return true;
}

bool DrmAtomicOutput::Init(int rotate_ccw, int prefer_mode_w, int prefer_mode_h) {
    Shutdown();
    rotate_ccw_ = rotate_ccw;
    if (rotate_ccw_ == 90) {
        rotation_val_ = kRotate90;
    } else if (rotate_ccw_ == 270) {
        rotation_val_ = kRotate270;
    } else if (rotate_ccw_ == 180) {
        rotation_val_ = kRotate180;
    } else {
        rotation_val_ = kRotate0;
        rotate_ccw_ = 0;
    }

    const char* cards[] = {"/dev/dri/card1", "/dev/dri/card0", nullptr};
    for (int i = 0; cards[i]; ++i) {
        fd_ = open(cards[i], O_RDWR | O_CLOEXEC);
        if (fd_ >= 0) {
            util::Log(util::LogLevel::Info, std::string("DRM open ") + cards[i]);
            break;
        }
    }
    if (fd_ < 0) {
        util::Log(util::LogLevel::Error, "DRM open failed: " + ErrnoStr());
        return false;
    }

    if (drmSetClientCap(fd_, DRM_CLIENT_CAP_ATOMIC, 1) != 0) {
        util::Log(util::LogLevel::Error, "DRM ATOMIC cap failed: " + ErrnoStr());
        Shutdown();
        return false;
    }
    drmSetClientCap(fd_, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    drmSetClientCap(fd_, DRM_CLIENT_CAP_ASPECT_RATIO, 1);

    drmModeResPtr res = drmModeGetResources(fd_);
    if (!res) {
        util::Log(util::LogLevel::Error, "DRM GetResources failed");
        Shutdown();
        return false;
    }

    drmModeConnectorPtr conn = nullptr;
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnectorPtr c = drmModeGetConnector(fd_, res->connectors[i]);
        if (!c) {
            continue;
        }
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            conn = c;
            connector_id_ = c->connector_id;
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!conn) {
        util::Log(util::LogLevel::Error, "DRM: no connected connector");
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }

    drmModeModeInfo mode = conn->modes[0];
    bool found = false;
    for (int i = 0; i < conn->count_modes; ++i) {
        const auto& m = conn->modes[i];
        if (static_cast<int>(m.hdisplay) == prefer_mode_w &&
            static_cast<int>(m.vdisplay) == prefer_mode_h) {
            mode = m;
            found = true;
            break;
        }
    }
    if (!found) {
        for (int i = 0; i < conn->count_modes; ++i) {
            if (conn->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
                mode = conn->modes[i];
                break;
            }
        }
    }
    mode_w_ = mode.hdisplay;
    mode_h_ = mode.vdisplay;

    if (rotate_ccw_ == 90 || rotate_ccw_ == 270) {
        fb_w_ = mode_h_;
        fb_h_ = mode_w_;
    } else {
        fb_w_ = mode_w_;
        fb_h_ = mode_h_;
    }

    crtc_id_ = 0;
    if (conn->encoder_id) {
        drmModeEncoderPtr enc = drmModeGetEncoder(fd_, conn->encoder_id);
        if (enc) {
            crtc_id_ = enc->crtc_id;
            drmModeFreeEncoder(enc);
        }
    }
    if (!crtc_id_ && res->count_crtcs > 0) {
        crtc_id_ = res->crtcs[0];
    }

    drmModePlaneResPtr planes = drmModeGetPlaneResources(fd_);
    if (!planes) {
        util::Log(util::LogLevel::Error, "DRM GetPlaneResources failed");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }
    plane_id_ = 0;
    for (uint32_t i = 0; i < planes->count_planes; ++i) {
        drmModePlanePtr p = drmModeGetPlane(fd_, planes->planes[i]);
        if (!p) {
            continue;
        }
        int crtc_idx = -1;
        for (int c = 0; c < res->count_crtcs; ++c) {
            if (res->crtcs[c] == crtc_id_) {
                crtc_idx = c;
                break;
            }
        }
        const bool possible =
            crtc_idx >= 0 && (p->possible_crtcs & (1u << static_cast<uint32_t>(crtc_idx)));
        drmModeObjectPropertiesPtr props =
            drmModeObjectGetProperties(fd_, p->plane_id, DRM_MODE_OBJECT_PLANE);
        bool primary = false;
        if (props) {
            for (uint32_t k = 0; k < props->count_props; ++k) {
                drmModePropertyPtr pr = drmModeGetProperty(fd_, props->props[k]);
                if (pr && std::strcmp(pr->name, "type") == 0) {
                    if (props->prop_values[k] == DRM_PLANE_TYPE_PRIMARY) {
                        primary = true;
                    }
                }
                if (pr) {
                    drmModeFreeProperty(pr);
                }
            }
            drmModeFreeObjectProperties(props);
        }
        if (possible && primary) {
            plane_id_ = p->plane_id;
            drmModeFreePlane(p);
            break;
        }
        drmModeFreePlane(p);
    }
    drmModeFreePlaneResources(planes);

    if (!plane_id_ || !crtc_id_) {
        util::Log(util::LogLevel::Error, "DRM: no primary plane/CRTC");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }

    prop_conn_crtc_ = GetPropId(connector_id_, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID");
    prop_mode_id_ = GetPropId(crtc_id_, DRM_MODE_OBJECT_CRTC, "MODE_ID");
    prop_active_ = GetPropId(crtc_id_, DRM_MODE_OBJECT_CRTC, "ACTIVE");
    prop_crtc_id_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "CRTC_ID");
    prop_fb_id_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "FB_ID");
    prop_src_x_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "SRC_X");
    prop_src_y_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "SRC_Y");
    prop_src_w_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "SRC_W");
    prop_src_h_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "SRC_H");
    prop_crtc_x_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "CRTC_X");
    prop_crtc_y_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "CRTC_Y");
    prop_crtc_w_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "CRTC_W");
    prop_crtc_h_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "CRTC_H");
    prop_rotation_ = GetPropId(plane_id_, DRM_MODE_OBJECT_PLANE, "rotation");

    if (!prop_rotation_) {
        util::Log(util::LogLevel::Error, "DRM: plane has no rotation property");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }

    if (drmModeCreatePropertyBlob(fd_, &mode, sizeof(mode), &blob_id_) != 0) {
        util::Log(util::LogLevel::Error, "DRM CreatePropertyBlob: " + ErrnoStr());
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }

    const bool need_y_tile = (rotate_ccw_ == 90 || rotate_ccw_ == 270);
    if (need_y_tile) {
        if (!CreateBoGbm(0, fb_w_, fb_h_) || !CreateBoGbm(1, fb_w_, fb_h_)) {
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            Shutdown();
            return false;
        }
    } else {
        if (!CreateBo(bo_[0], fb_w_, fb_h_) || !CreateBo(bo_[1], fb_w_, fb_h_)) {
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            Shutdown();
            return false;
        }
    }
    fb_id_[0] = bo_[0].fb_id;
    fb_id_[1] = bo_[1].fb_id;
    front_ = 0;

    // Test pattern na BO[0]
    if (need_y_tile && g_gbm.bo[0]) {
        uint32_t stride = 0;
        void* map_data = nullptr;
        void* addr = gbm_bo_map(g_gbm.bo[0], 0, 0, static_cast<uint32_t>(fb_w_),
                                static_cast<uint32_t>(fb_h_), GBM_BO_TRANSFER_WRITE, &stride,
                                &map_data);
        if (addr) {
            auto* px = static_cast<uint32_t*>(addr);
            const int s = static_cast<int>(stride / 4);
            for (int y = 0; y < fb_h_; ++y) {
                for (int x = 0; x < fb_w_; ++x) {
                    uint32_t c = 0xFF202030;
                    if (y < fb_h_ / 10) c = 0xFF00FF40;
                    else if (x < fb_w_ / 10) c = 0xFFFF4020;
                    px[y * s + x] = c;
                }
            }
            gbm_bo_unmap(g_gbm.bo[0], map_data);
        }
    } else if (bo_[0].map) {
        auto* px = static_cast<uint32_t*>(bo_[0].map);
        const int stride = static_cast<int>(bo_[0].pitch / 4);
        for (int y = 0; y < fb_h_; ++y) {
            for (int x = 0; x < fb_w_; ++x) {
                uint32_t c = 0xFF202030;
                if (y < fb_h_ / 10) c = 0xFF00FF40;
                else if (x < fb_w_ / 10) c = 0xFFFF4020;
                px[y * stride + x] = c;
            }
        }
    }

    util::Log(util::LogLevel::Info,
              "DRM atomic: mode " + std::to_string(mode_w_) + "x" + std::to_string(mode_h_) +
                  " fb " + std::to_string(fb_w_) + "x" + std::to_string(fb_h_) + " rot_ccw=" +
                  std::to_string(rotate_ccw_) + " plane=" + std::to_string(plane_id_) +
                  (need_y_tile ? " (GBM Y-tile)" : " (dumb linear)"));

    if (!AtomicModeset(fb_id_[0], true)) {
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        Shutdown();
        return false;
    }
    mode_set_ = true;

    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    util::Log(util::LogLevel::Info, "DRM atomic modeset OK");
    return true;
}

bool DrmAtomicOutput::Present(const void* pixels, int pitch) {
    if (!ok() || !pixels) {
        return false;
    }
    const int back = 1 - front_;
    if (g_gbm.bo[back]) {
        uint32_t stride = 0;
        void* map_data = nullptr;
        void* addr =
            gbm_bo_map(g_gbm.bo[back], 0, 0, static_cast<uint32_t>(fb_w_),
                       static_cast<uint32_t>(fb_h_), GBM_BO_TRANSFER_WRITE, &stride, &map_data);
        if (!addr) {
            util::Log(util::LogLevel::Error, "gbm_bo_map failed");
            return false;
        }
        auto* dst = static_cast<uint8_t*>(addr);
        const auto* src = static_cast<const uint8_t*>(pixels);
        const int copy_pitch = std::min(pitch, static_cast<int>(stride));
        for (int y = 0; y < fb_h_; ++y) {
            std::memcpy(dst + y * stride, src + y * pitch, static_cast<size_t>(copy_pitch));
        }
        gbm_bo_unmap(g_gbm.bo[back], map_data);
    } else {
        Bo& bo = bo_[back];
        auto* dst = static_cast<uint8_t*>(bo.map);
        const auto* src = static_cast<const uint8_t*>(pixels);
        const int copy_pitch = std::min(pitch, static_cast<int>(bo.pitch));
        for (int y = 0; y < fb_h_; ++y) {
            std::memcpy(dst + y * bo.pitch, src + y * pitch, static_cast<size_t>(copy_pitch));
        }
    }
    if (!AtomicModeset(bo_[back].fb_id, !mode_set_)) {
        return false;
    }
    mode_set_ = true;
    front_ = back;
    return true;
}

}  // namespace ui
