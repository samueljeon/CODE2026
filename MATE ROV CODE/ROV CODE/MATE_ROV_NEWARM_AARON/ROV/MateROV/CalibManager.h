//
// Created by Adam G. on 5/18/25.
//

#ifndef CALIBMANAGER_H
#define CALIBMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>

struct PacketCalib {
    float min;
    float max;
    bool invert;
    float apply(float value) const {
        return std::clamp(value, min, max);
    }
};

class CalibManager {
public:
    void registerControl(const std::string& name, float min, float max, bool hasInvert = false);
    float apply(const std::string& name, float value) const;

    void renderUI();
    void loadLastUsedProfile();
    void saveLastUsedProfile(const std::string& name);
    void loadProfile(const std::string& name);
    void saveProfile(const std::string& name);
    void refreshProfiles();
    const std::vector<std::string>& getProfiles() const;

private:
    struct Entry {
        std::string name;
        PacketCalib calib;
        float rangeMin;
        float rangeMax;
        bool hasInvert;
        bool invert;
    };

    std::vector<Entry> controls;
    std::unordered_map<std::string, size_t> nameToIndex;
    std::vector<std::string> profileNames;
    std::string profileFolder = "calibration";

    char profileInputBuf[64] = "";
    int selectedProfileIndex = -1;
};

#endif //CALIBMANAGER_H
