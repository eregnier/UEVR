#pragma once

#include <unordered_set>
#include <deque>

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <common/xr_linear.h>

#include <sdk/Math.hpp>

#include "Mod.hpp"
#include "VRRuntime.hpp"

namespace runtimes {

// Foveated rendering configuration structure
struct FoveatedConfig {
    bool enabled = false;
    float center_resolution_scale = 1.0f;
    float peripheral_resolution_scale = 0.25f;
    float center_size_x = 0.5f;
    float center_size_y = 0.5f;
    bool use_eye_tracking = false;
    
    void on_draw_ui() {
        ImGui::Checkbox("Enable Foveated Rendering", &enabled);
        if (enabled) {
            ImGui::SliderFloat("Center Resolution Scale", &center_resolution_scale, 0.1f, 2.0f);
            ImGui::SliderFloat("Peripheral Resolution Scale", &peripheral_resolution_scale, 0.1f, 1.0f);
            ImGui::SliderFloat("Center Size X", &center_size_x, 0.1f, 1.0f);
            ImGui::SliderFloat("Center Size Y", &center_size_y, 0.1f, 1.0f);
            ImGui::Checkbox("Use Eye Tracking", &use_eye_tracking);
        }
    }
};

// Foveated view data for quad-view rendering
struct FoveatedViewData {
    std::vector<XrView> high_res_views;      // 2 high-resolution central views
    std::vector<XrView> low_res_views;       // 2 low-resolution peripheral views
    std::vector<XrViewConfigurationView> high_res_configs;
    std::vector<XrViewConfigurationView> low_res_configs;
    
    uint32_t high_res_width = 0;
    uint32_t high_res_height = 0;
    uint32_t low_res_width = 0;
    uint32_t low_res_height = 0;
};

// Foveated rendering extension for OpenXR
class FoveatedOpenXR : public VRRuntime {
public:
    FoveatedOpenXR() {
        this->custom_stage = SynchronizeStage::VERY_LATE;
    }
    
    virtual ~FoveatedOpenXR() {
        this->destroy();
    }

    VRRuntime::Type type() const override { 
        return VRRuntime::Type::OPENXR;
    }

    std::string_view name() const override {
        return "OpenXR-Foveated";
    }

    bool ready() const override {
        return VRRuntime::ready() && this->session_ready && this->foveated_ready;
    }

    void on_config_load(const utility::Config& cfg, bool set_defaults) override;
    void on_config_save(utility::Config& cfg) override;
    void on_draw_ui() override;

    void destroy() override;
    
    // Foveated rendering specific methods
    bool initialize_foveated_rendering();
    bool check_foveated_support();
    void calculate_foveated_viewports(uint32_t base_width, uint32_t base_height);
    
    // Override view configuration for foveated rendering
    VRRuntime::Error update_matrices(float nearz, float farz) override;
    VRRuntime::Error update_render_target_size() override;
    
    // Foveated view management
    bool setup_foveated_views();
    bool create_foveated_swapchains();
    
protected:
    // Foveated rendering state
    bool foveated_ready = false;
    bool foveated_supported = false;
    bool use_quad_views = false;
    
    FoveatedConfig foveated_config;
    FoveatedViewData foveated_views;
    
    // Extended view configuration for foveated rendering
    XrViewConfigurationType foveated_view_config{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_WITH_FOVEATED_INSET};
    
    // Additional swapchains for foveated rendering
    std::unordered_map<uint32_t, Swapchain> foveated_swapchains{};
    
    // Eye tracking support
    bool eye_tracking_supported = false;
    bool eye_tracking_active = false;
    
    // Foveated rendering specific extensions
    bool has_varjo_foveated = false;
    bool has_fb_foveation = false;
    bool has_meta_eye_tracked = false;
    
    // Utility methods
    void setup_foveated_options();
    bool check_extensions();
    void log_foveated_info();
};

} // namespace runtimes