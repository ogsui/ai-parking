#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

namespace fs = std::filesystem;

struct SystemConfig {
    std::map<std::string, double> tollRates;
    int cameraResolutionWidth;
    int cameraResolutionHeight;
    int cameraFPS;
};

class FileManager {
private:
    std::string baseDir;
    
public:
    FileManager(const std::string& basePath = "ai_toll_system") : baseDir(basePath) {
        createDirectoryStructure();
    }

    void createDirectoryStructure() {
        std::vector<std::string> dirs = {
            "config",
            "data",
            "data/tessdata",
            "logs",
            "output/captured_plates",
            "output/processed_images",
            "output/daily_summaries"
        };

        for (const auto& dir : dirs) {
            fs::create_directories(baseDir + "/" + dir);
        }
    }

    std::string getConfigPath(const std::string& filename) {
        return baseDir + "/config/" + filename;
    }

    std::string getLogPath(const std::string& filename) {
        return baseDir + "/logs/" + filename;
    }

    std::string getOutputPath(const std::string& subdir, const std::string& filename) {
        return baseDir + "/output/" + subdir + "/" + filename;
    }
};

class Logger {
private:
    FileManager& fileManager;
    std::ofstream transactionLog;
    std::ofstream errorLog;

public:
    Logger(FileManager& fm) : fileManager(fm) {
        transactionLog.open(fileManager.getLogPath("transaction_log.csv"), std::ios::app);
        errorLog.open(fileManager.getLogPath("error_log.txt"), std::ios::app);
        
        // Write headers if files are new
        if (transactionLog.tellp() == 0) {
            transactionLog << "timestamp,vehicle_id,payment_method,amount,balance_remaining\n";
        }
    }

    void logTransaction(const std::string& vehicleId, 
                       const std::string& paymentMethod, 
                       double amount, 
                       double balanceRemaining) {
        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        
        transactionLog << std::ctime(&nowTime) << ","
                      << vehicleId << ","
                      << paymentMethod << ","
                      << amount << ","
                      << balanceRemaining << "\n";
        transactionLog.flush();
    }

    void logError(const std::string& error) {
        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        
        errorLog << std::ctime(&nowTime) << ": " << error << "\n";
        errorLog.flush();
    }
};

class ConfigManager {
private:
    FileManager& fileManager;
    SystemConfig config;

public:
    ConfigManager(FileManager& fm) : fileManager(fm) {
        loadConfig();
    }

    void loadConfig() {
        std::ifstream configFile(fileManager.getConfigPath("config.txt"));
        if (!configFile.is_open()) {
            createDefaultConfig();
            return;
        }

        std::string line;
        while (std::getline(configFile, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                if (key == "toll_rate_car") config.tollRates["car"] = std::stod(value);
                else if (key == "toll_rate_truck") config.tollRates["truck"] = std::stod(value);
                else if (key == "toll_rate_bus") config.tollRates["bus"] = std::stod(value);
                else if (key == "camera_resolution_width") config.cameraResolutionWidth = std::stoi(value);
                else if (key == "camera_resolution_height") config.cameraResolutionHeight = std::stoi(value);
                else if (key == "camera_fps") config.cameraFPS = std::stoi(value);
            }
        }
    }

    void createDefaultConfig() {
        std::ofstream configFile(fileManager.getConfigPath("config.txt"));
        configFile << "# Toll Rates\n"
                  << "toll_rate_car=50.0\n"
                  << "toll_rate_truck=100.0\n"
                  << "toll_rate_bus=75.0\n\n"
                  << "# Camera Settings\n"
                  << "camera_resolution_width=1920\n"
                  << "camera_resolution_height=1080\n"
                  << "camera_fps=30\n";
        
        // Set default values
        config.tollRates["car"] = 50.0;
        config.tollRates["truck"] = 100.0;
        config.tollRates["bus"] = 75.0;
        config.cameraResolutionWidth = 1920;
        config.cameraResolutionHeight = 1080;
        config.cameraFPS = 30;
    }

    const SystemConfig& getConfig() const { return config; }
};

// ... (Previous Vehicle and VehicleImage classes remain the same)

class TollSystem {
private:
    FileManager fileManager;
    Logger logger;
    ConfigManager configManager;
    std::map<std::string, Vehicle> registeredVehicles;
    ComputerVision cvSystem;

public:
    TollSystem() : 
        fileManager("ai_toll_system"),
        logger(fileManager),
        configManager(fileManager) {
        loadRegisteredVehicles();
    }

    void loadRegisteredVehicles() {
        std::ifstream vehicleFile(fileManager.getConfigPath("registered_vehicles.csv"));
        if (!vehicleFile.is_open()) {
            logger.logError("Could not open registered vehicles file");
            return;
        }

        std::string line;
        // Skip header
        std::getline(vehicleFile, line);
        
        while (std::getline(vehicleFile, line)) {
            std::istringstream iss(line);
            std::string plate, rfid, type;
            double balance;
            
            if (std::getline(iss, plate, ',') &&
                std::getline(iss, rfid, ',') &&
                (iss >> balance) &&
                std::getline(iss, type, ',')) {
                
                Vehicle vehicle(plate, rfid, balance, type);
                registeredVehicles[rfid] = vehicle;
            }
        }
    }

    bool processVehicleByCamera(const cv::Mat& vehicleImage) {
        // Save original image
        std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        cv::imwrite(fileManager.getOutputPath("processed_images", "vehicle_" + timestamp + ".jpg"), vehicleImage);

        VehicleImage vImage;
        vImage.preprocessImage();
        
        cv::Rect plateRegion = vImage.detectLicensePlateRegion(cvSystem.plateCascade);
        if (plateRegion.empty()) {
            logger.logError("No license plate detected in image " + timestamp);
            return false;
        }

        // Save cropped plate image
        cv::Mat plateImage = vehicleImage(plateRegion);
        cv::imwrite(fileManager.getOutputPath("captured_plates", "plate_" + timestamp + ".jpg"), plateImage);
        
        std::string recognizedPlate = cvSystem.recognizeLicensePlate(plateImage);
        return processVehicleByANPR(recognizedPlate);
    }

    // ... (Rest of the TollSystem methods remain similar, but add logging)
};

int main() {
    try {
        TollSystem tollSystem;
        
        // Test with a sample image
        cv::Mat testImage = cv::imread("test_vehicle.jpg");
        if (!testImage.empty()) {
            tollSystem.processVehicleByCamera(testImage);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}