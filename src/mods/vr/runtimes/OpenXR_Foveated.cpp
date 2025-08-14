#include "OpenXR_Foveated.hpp"
#include "Framework.hpp"
#include "../../VR.hpp"
#include <spdlog/spdlog.h>
#include <imgui.h>

namespace runtimes {

void FoveatedOpenXR::on_config_load(const utility::Config& cfg, bool set_defaults) {
    foveated_config.enabled = cfg.get_or_set_bool("OpenXR_Foveated_Enabled", false, set_defaults);
    foveated_config.center_resolution_scale = cfg.get_or_set_float("OpenXR_Foveated_CenterScale", 1.0f, set_defaults);
    foveated_config.peripheral_resolution_scale = cfg.get_or_set_float("OpenXR_Foveated_PeripheralScale", 0.25f, set_defaults);
    foveated_config.center_size_x = cfg.get_or_set_float("OpenXR_Foveated_CenterSizeX", 0.5f, set_defaults);
    foveated_config.center_size_y = cfg.get_or_set_float("OpenXR_Foveated_CenterSizeY", 0.5f, set_defaults);
    foveated_config.use_eye_tracking = cfg.get_or_set_bool("OpenXR_Foveated_EyeTracking", false, set_defaults);
    
    for (IModValue& option : this->options) {
        option.config_load(cfg, set_defaults);
    }
}

void FoveatedOpenXR::on_config_save(utility::Config& cfg) {
    cfg.set_bool("OpenXR_Foveated_Enabled", foveated_config.enabled);
    cfg.set_float("OpenXR_Foveated_CenterScale", foveated_config.center_resolution_scale);
    cfg.set_float("OpenXR_Foveated_PeripheralScale", foveated_config.peripheral_resolution_scale);
    cfg.set_float("OpenXR_Foveated_CenterSizeX", foveated_config.center_size_x);
    cfg.set_float("OpenXR_Foveated_CenterSizeY", foveated_config.center_size_y);
    cfg.set_bool("OpenXR_Foveated_EyeTracking", foveated_config.use_eye_tracking);
    
    for (IModValue& option : this->options) {
        option.config_save(cfg);
    }
}

void FoveatedOpenXR::on_draw_ui() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("OpenXR Foveated Rendering")) {
        foveated_config.on_draw_ui();
        
        if (ImGui::Button("Initialize Foveated Rendering")) {
            initialize_foveated_rendering();
        }
        
        if (foveated_ready) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Foveated Rendering: Ready");
        } else if (foveated_supported) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Foveated Rendering: Available but not initialized");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Foveated Rendering: Not Supported");
        }
        
        if (ImGui::TreeNode("Foveated Rendering Info")) {
            log_foveated_info();
            ImGui::TreePop();
        }
        
        ImGui::TreePop();
    }
    
    // Also show standard OpenXR options
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("OpenXR Standard")) {
        this->resolution_scale->draw("Resolution Scale");
        ImGui::Checkbox("Virtual Desktop Fix", &this->push_dummy_projection);
        this->ignore_vd_checks->draw("Ignore Virtual Desktop Checks");
        ImGui::TreePop();
    }
}

bool FoveatedOpenXR::check_extensions() {
    // Check for required foveated rendering extensions
    uint32_t extension_count = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extension_count, nullptr);
    
    if (extension_count > 0) {
        std::vector<XrExtensionProperties> extensions(extension_count);
        for (auto& ext : extensions) {
            ext.type = XR_TYPE_EXTENSION_PROPERTIES;
        }
        
        xrEnumerateInstanceExtensionProperties(nullptr, extension_count, &extension_count, extensions.data());
        
        for (const auto& ext : extensions) {
            std::string ext_name = ext.extensionName;
            
            if (ext_name == "XR_VARJO_foveated_rendering") {
                has_varjo_foveated = true;
                enabled_extensions.insert("XR_VARJO_foveated_rendering");
            }
            if (ext_name == "XR_FB_foveation") {
                has_fb_foveation = true;
                enabled_extensions.insert("XR_FB_foveation");
            }
            if (ext_name == "XR_META_foveation_eye_tracked") {
                has_meta_eye_tracked = true;
                enabled_extensions.insert("XR_META_foveation_eye_tracked");
            }
        }
    }
    
    return has_varjo_foveated || has_fb_foveation || has_meta_eye_tracked;
}

bool FoveatedOpenXR::check_foveated_support() {
    if (system == XR_NULL_SYSTEM_ID) {
        return false;
    }
    
    // Check if the system supports foveated rendering view configuration
    uint32_t view_config_count = 0;
    XrResult result = xrEnumerateViewConfigurations(instance, system, 0, &view_config_count, nullptr);
    
    if (XR_FAILED(result)) {
        spdlog::error("[Foveated] Failed to enumerate view configurations: {}", this->get_result_string(result));
        return false;
    }
    
    std::vector<XrViewConfigurationType> view_configs(view_config_count);
    result = xrEnumerateViewConfigurations(instance, system, view_config_count, &view_config_count, view_configs.data());
    
    if (XR_FAILED(result)) {
        spdlog::error("[Foveated] Failed to get view configurations: {}", this->get_result_string(result));
        return false;
    }
    
    for (const auto config : view_configs) {
        if (config == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_WITH_FOVEATED_INSET) {
            spdlog::info("[Foveated] Quad-view configuration supported");
            return true;
        }
    }
    
    spdlog::warn("[Foveated] Quad-view configuration not supported, falling back to standard stereo");
    return false;
}

bool FoveatedOpenXR::initialize_foveated_rendering() {
    if (!foveated_config.enabled) {
        spdlog::info("[Foveated] Foveated rendering disabled in config");
        return false;
    }
    
    // Check hardware support first
    if (!check_foveated_support()) {
        spdlog::warn("[Foveated] Hardware does not support foveated rendering");
        foveated_supported = false;
        return false;
    }
    
    foveated_supported = true;
    
    // Switch to foveated view configuration
    if (use_quad_views) {
        view_config = foveated_view_config;
        spdlog::info("[Foveated] Using quad-view configuration: {}", 
                     view_config == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_WITH_FOVEATED_INSET ? 
                     "PRIMARY_STEREO_WITH_FOVEATED_INSET" : "PRIMARY_QUAD_VARJO");
    }
    
    // Setup foveated views
    if (!setup_foveated_views()) {
        spdlog::error("[Foveated] Failed to setup foveated views");
        return false;
    }
    
    foveated_ready = true;
    spdlog::info("[Foveated] Foveated rendering initialized successfully");
    return true;
}

bool FoveatedOpenXR::setup_foveated_views() {
    if (system == XR_NULL_SYSTEM_ID) {
        return false;
    }
    
    uint32_t view_count = 0;
    XrResult result = xrEnumerateViewConfigurationViews(
        instance, system, view_config, 0, &view_count, nullptr);
    
    if (XR_FAILED(result)) {
        spdlog::error("[Foveated] Failed to get view count: {}", this->get_result_string(result));
        return false;
    }
    
    if (use_quad_views && view_count != 4) {
        spdlog::warn("[Foveated] Expected 4 views for foveated rendering, got {}", view_count);
        use_quad_views = false;
        return false;
    }
    
    // Resize vectors for all views
    view_configs.resize(view_count);
    views.resize(view_count);
    stage_views.resize(view_count);
    
    for (auto& config : view_configs) {
        config.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    }
    
    for (auto& view : views) {
        view.type = XR_TYPE_VIEW;
    }
    
    for (auto& view : stage_views) {
        view.type = XR_TYPE_VIEW;
    }
    
    result = xrEnumerateViewConfigurationViews(
        instance, system, view_config, view_count, &view_count, view_configs.data());
    
    if (XR_FAILED(result)) {
        spdlog::error("[Foveated] Failed to get view configurations: {}", this->get_result_string(result));
        return false;
    }
    
    if (use_quad_views) {
        // Initialize foveated view data
        foveated_views.high_res_views.resize(2);
        foveated_views.low_res_views.resize(2);
        foveated_views.high_res_configs.resize(2);
        foveated_views.low_res_configs.resize(2);
        
        // Calculate foveated viewport sizes
        calculate_foveated_viewports(
            view_configs[0].recommendedImageRectWidth,
            view_configs[0].recommendedImageRectHeight);
    }
    
    return true;
}

void FoveatedOpenXR::calculate_foveated_viewports(uint32_t base_width, uint32_t base_height) {
    if (!use_quad_views) {
        return;
    }
    
    // Calculate high-resolution center viewport sizes
    foveated_views.high_res_width = static_cast<uint32_t>(
        base_width * foveated_config.center_size_x * foveated_config.center_resolution_scale);
    foveated_views.high_res_height = static_cast<uint32_t>(
        base_height * foveated_config.center_size_y * foveated_config.center_resolution_scale);
    
    // Calculate low-resolution peripheral viewport sizes
    foveated_views.low_res_width = static_cast<uint32_t>(
        base_width * foveated_config.peripheral_resolution_scale);
    foveated_views.low_res_height = static_cast<uint32_t>(
        base_height * foveated_config.peripheral_resolution_scale);
    
    spdlog::info("[Foveated] Viewport sizes calculated:");
    spdlog::info("  High-res: {}x{}", foveated_views.high_res_width, foveated_views.high_res_height);
    spdlog::info("  Low-res: {}x{}", foveated_views.low_res_width, foveated_views.low_res_height);
}

VRRuntime::Error FoveatedOpenXR::update_matrices(float nearz, float farz) {
    if (!foveated_ready || !use_quad_views) {
        return OpenXR::update_matrices(nearz, farz);
    }
    
    // Handle foveated rendering matrix updates
    std::scoped_lock _{this->sync_assignment_mtx};
    
    if (!this->session_ready) {
        return VRRuntime::Error::SUCCESS;
    }
    
    // Update matrices for all 4 views (2 high-res + 2 low-res)
    // This would need to be implemented based on the specific foveated rendering approach
    
    return VRRuntime::Error::SUCCESS;
}

VRRuntime::Error FoveatedOpenXR::update_render_target_size() {
    if (!foveated_ready || !use_quad_views) {
        return OpenXR::update_render_target_size();
    }
    
    // Update render target sizes for foveated rendering
    std::scoped_lock _{this->sync_assignment_mtx};
    
    if (!this->session_ready || this->view_configs.empty()) {
        return VRRuntime::Error::SUCCESS;
    }
    
    // Calculate sizes based on foveated configuration
    calculate_foveated_viewports(
        this->view_configs[0].recommendedImageRectWidth,
        this->view_configs[0].recommendedImageRectHeight);
    
    return VRRuntime::Error::SUCCESS;
}

void FoveatedOpenXR::log_foveated_info() {
    ImGui::Text("Foveated Support: %s", foveated_supported ? "Yes" : "No");
    ImGui::Text("Quad Views: %s", use_quad_views ? "Enabled" : "Disabled");
    ImGui::Text("View Configuration: %d", static_cast<int>(view_config));
    ImGui::Text("Extensions:");
    ImGui::Text("  Varjo Foveated: %s", has_varjo_foveated ? "Available" : "Not Available");
    ImGui::Text("  FB Foveation: %s", has_fb_foveation ? "Available" : "Not Available");
    ImGui::Text("  Meta Eye Tracked: %s", has_meta_eye_tracked ? "Available" : "Not Available");
    
    if (foveated_ready) {
        ImGui::Text("High-Res Size: %dx%d", foveated_views.high_res_width, foveated_views.high_res_height);
        ImGui::Text("Low-Res Size: %dx%d", foveated_views.low_res_width, foveated_views.low_res_height);
    }
}

void FoveatedOpenXR::destroy() {
    foveated_ready = false;
    foveated_supported = false;
    use_quad_views = false;
    
    foveated_views.high_res_views.clear();
    foveated_views.low_res_views.clear();
    foveated_views.high_res_configs.clear();
    foveated_views.low_res_configs.clear();
    
    foveated_swapchains.clear();
    
    OpenXR::destroy();
}

// Initialize options
void FoveatedOpenXR::setup_foveated_options() {
    this->options = {
        *this->resolution_scale,
        *this->ignore_vd_checks,
    };
}

} // namespace runtimes