#include "sim_logger.h"
#include <simLib/scriptFunctionData.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <simLib/simLib.h>

static LIBRARY simLib;
static std::ofstream csvFile;
static std::chrono::high_resolution_clock::time_point lastTime, initialTime;
static int frameCount = 0;
static float fps = 0.0f;
static auto sep = ';';

// Get current system date and time for filename creation
static std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    std::tm* localTime = std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S");
    
    return oss.str();
}

SIM_DLLEXPORT int simInit(SSimInit* info)
{
    simLib = loadSimLibrary(info->coppeliaSimLibPath);
    if (simLib == NULL)
    {
        simAddLog(info->pluginName, sim_verbosity_errors, "Could not find all required functions in the CoppeliaSim library. Cannot start the plugin.");
        return 0;
    }
    if (getSimProcAddresses(simLib) == 0)
    {
        simAddLog(info->pluginName, sim_verbosity_errors, "Your CoppeliaSim version is outdated. CoppeliaSim 4.0.0 rev1 or higher is required. Cannot start the plugin.");
        unloadSimLibrary(simLib);
        return 0;
    }

    return 13; // Return the plugin version
}

SIM_DLLEXPORT void simCleanup()
{
    if (csvFile.is_open()) {
        csvFile.close();  // Ensure file is closed
    }
    unloadSimLibrary(simLib);  // Release the library
}

SIM_DLLEXPORT void simMsg(SSimMsg* info)
{   
    if (info->msgId == sim_message_eventcallback_simulationabouttostart)
    {
        // Close any previous CSV file if open
        if (csvFile.is_open()) {
            csvFile.close();
        }

        // Create CSV filename with date and time
        std::string dateTime = getCurrentDateTime();
        std::string fileName = "simLogger_" + dateTime + ".csv";
        csvFile.open(fileName);

        // Verify if file opened successfully
        if (!csvFile.is_open()) {
            std::cerr << "Unable to open CSV file: " << fileName << std::endl;
            // simAddLog(info->pluginName, sim_verbosity_errors, "Unable to open CSV file for logging.");
        }

        // Write CSV headers
        csvFile << "Frame" << sep 
                << "Simulation Time (ms)" << sep 
                << "System Time (ms)" << sep 
                << "FPS (Hz)" << sep 
                << "Active Objects" << sep 
                << "Collision Count" << sep 
                << "Step Size (ms)" << std::endl;

        initialTime = std::chrono::high_resolution_clock::now(); // Initialize start time
        lastTime = std::chrono::high_resolution_clock::now();  // Initialize last time for FPS calculations
    }

    if ((info->msgId == sim_message_eventcallback_simulationsensing) && (info->auxData[0] == 0))
    {
        // Get current simulation time in ms
        int simTime_ms = static_cast<int>(simGetSimulationTime() * 1000);

        // Increment frame counter
        frameCount++;

        // Capture the current time for system-based measurements
        auto currentTime = std::chrono::high_resolution_clock::now();

        // Calculate elapsed time since the start of the simulation
        std::chrono::duration<float> duration = currentTime - initialTime;
        auto systemTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        // Calculate FPS if at least 1 second has passed
        std::chrono::duration<float> elapsedTime = currentTime - lastTime;
        if (elapsedTime.count() >= 1.0f) {
            fps = frameCount / elapsedTime.count();

            // Reset frame count and update lastTime
            frameCount = 0;
            lastTime = currentTime;
        }

        // Log active objects
        int sceneObjectsCtn;
        int *activeObjects = simGetObjectsInTree(sim_handle_scene, sim_handle_all, 2, &sceneObjectsCtn);
    

        // If there are no objects, return an empty JSON string
        std::string objectsData; 
        if (activeObjects == nullptr) {
            objectsData = "[]";
        }
        else{
            // Initialize the JSON string
            objectsData = "[";
            for (int i = 0; i < sceneObjectsCtn; i++) {
                int objectHandle = activeObjects[i];

                std::string alias(simGetObjectAlias(objectHandle, 0));
                objectsData += "{ \"alias\": \"" + alias + "\", ";

                double pose[7];
                int r = simGetObjectPose(objectHandle, sim_handle_world, pose);
                
                objectsData += "\"pose\": ["
                     + std::to_string(pose[0]) + ", "
                     + std::to_string(pose[1]) + ", "
                     + std::to_string(pose[2]) + ", "
                     + std::to_string(pose[3]) + ", "
                     + std::to_string(pose[4]) + ", "
                     + std::to_string(pose[5]) + ", "
                     + std::to_string(pose[6]) + "]"
                     + "}";

                if (i < sceneObjectsCtn - 1) {
                    objectsData += ", ";  // Add a comma between objects, but not after the last one
                }
            }
            objectsData += "]";
        }
        simReleaseBuffer(reinterpret_cast<char*>(activeObjects));

        // Log the current simulation time step (step size)
        double stepSize = simGetSimulationTimeStep() * 1000;  // Convert to ms

        // Example collision checking (replace with your actual objects' handles if needed)
        int collisionHandle = simGetObject("/Floor",-1,-1,0);  // You can specify the object name or ID
        int collisionCount = 0;
        if (collisionHandle != -1)
        {
            collisionCount = simCheckCollision(collisionHandle, sim_handle_all);  // Check for collisions with any object
        }

        // Write data to CSV file
        if (csvFile.is_open()) {
            auto sep = ';';
            csvFile << std::to_string(frameCount) << sep 
                    << std::to_string(simTime_ms) << sep 
                    << std::to_string(systemTime_ms) << sep 
                    << std::to_string(fps) << sep 
                    << objectsData << sep 
                    << std::to_string(collisionCount) << sep 
                    << std::to_string(stepSize) << std::endl;
        }
    }

    // Close the file when the script state is destroyed
    if (info->msgId == sim_message_eventcallback_scriptstatedestroyed)
    {
        if (csvFile.is_open()) {
            csvFile.close();
        }

        frameCount = 0;
        fps = 0.0f;
    }
}
