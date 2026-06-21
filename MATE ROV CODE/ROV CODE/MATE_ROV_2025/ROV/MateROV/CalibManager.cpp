//
// Created by Adam G. on 5/18/25.
//

#include "CalibManager.h"
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include <imgui.h>

void CalibManager::registerControl(const std::string& name, float min, float max, bool hasInvert) {
    nameToIndex[name] = controls.size();
    controls.push_back({ name, { min, max, false }, min, max, hasInvert });
}

float CalibManager::apply(const std::string& name, float value) const {
    auto it = nameToIndex.find(name);

    if (it != nameToIndex.end()) {
        float invert = controls[it->second].calib.invert ? -1.0f : 1.0f;
        return invert * controls[it->second].calib.apply(value);
    }

    return value;
}

void CalibManager::renderUI() {
    for (auto& entry : controls) {
        ImGui::Text("%s", entry.name.c_str());
        if (entry.hasInvert) {
            ImGui::Checkbox((entry.name + " Invert").c_str(), &entry.calib.invert);
        }
        ImGui::SliderFloat((entry.name + " Min").c_str(), &entry.calib.min, entry.rangeMin, entry.rangeMax, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat((entry.name + " Max").c_str(), &entry.calib.max, entry.rangeMin, entry.rangeMax, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    }

    ImGui::SeparatorText("Profiles");

    if (!profileNames.empty()) {
        std::vector<const char*> items;
        for (const auto& name : profileNames) {
            items.push_back(name.c_str());
        }

        if (ImGui::Button("Refresh")) {
            refreshProfiles();
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Profile") && selectedProfileIndex >= 0) {
            loadProfile(profileNames[selectedProfileIndex]);
        }

        ImGui::SameLine();

        ImGui::Combo("Profiles", &selectedProfileIndex, items.data(), static_cast<int>(items.size()));
    }

    if (ImGui::Button("Save")) {
        saveProfile(profileInputBuf);
        refreshProfiles();
    }

    ImGui::SameLine();

    ImGui::InputText("New Profile Name", profileInputBuf, sizeof(profileInputBuf));
}

void CalibManager::loadLastUsedProfile() {
    std::ifstream in(profileFolder + "/last_profile.txt");

    if (!in) {
        return;
    }

    std::string last;
    std::getline(in, last);

    if (!last.empty()) {
        loadProfile(last);
        auto it = std::find(profileNames.begin(), profileNames.end(), last);

        if (it != profileNames.end()) {
            selectedProfileIndex = static_cast<int>(std::distance(profileNames.begin(), it));
        }
    }
}

void CalibManager::saveLastUsedProfile(const std::string& name) {
    std::ofstream(profileFolder + "/last_profile.txt") << name;
}

void CalibManager::saveProfile(const std::string& name) {
    if (name.empty()) {
        return;
    }

    std::filesystem::create_directories(profileFolder);
    nlohmann::json j;

    for (const auto& entry : controls) {
        j[entry.name] = { { "min", entry.calib.min }, { "max", entry.calib.max }, {"invert", entry.calib.invert} };
    }

    std::ofstream(profileFolder + "/calib_" + name + ".json") << j.dump(2);
    saveLastUsedProfile(name);
}

void CalibManager::loadProfile(const std::string& name) {
    std::ifstream file(profileFolder + "/calib_" + name + ".json");

    if (!file) {
        return;
    }

    nlohmann::json j;
    file >> j;

    for (auto& entry : controls) {
        if (j.contains(entry.name)) {
            entry.calib.min = j[entry.name].value("min", entry.calib.min);
            entry.calib.max = j[entry.name].value("max", entry.calib.max);
            entry.calib.invert = j[entry.name].value("invert", entry.calib.invert);
        }
    }

    saveLastUsedProfile(name);
}

void CalibManager::refreshProfiles() {
    profileNames.clear();
    std::filesystem::create_directories(profileFolder);

    for (const auto& file : std::filesystem::directory_iterator(profileFolder)) {
        if (file.path().extension() == ".json") {
            std::string fname = file.path().stem().string();
            if (fname.rfind("calib_", 0) == 0) {
                profileNames.push_back(fname.substr(6));
            }
        }
    }
}

const std::vector<std::string>& CalibManager::getProfiles() const {
    return profileNames;
}

