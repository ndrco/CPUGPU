// This program is a custom plugin for LCDSmartie https://github.com/LCD-Smartie/LCDSmartie designed to monitor
// and retrieve hardware metrics for CPU and GPU.
// It uses LibreHardwareMonitor and NVIDIA Management Library (NVML) to access sensor data.
// Make sure to add a LibreHardwareMonitor.dll in root LCDSmartie directory and compile with the /clr option.


#define WIN32_LEAN_AND_MEAN	// Reduce the inclusion of rarely used Windows headers to speed up compilation.
#include <windows.h>
#include <string>
#include <nvml.h>

#using "LibreHardwareMonitorLib.dll"

using namespace LibreHardwareMonitor::Hardware;


#define DLLEXPORT __declspec(dllexport)

// NVML clock throttle reasons constants.
#define NVML_CLK_THROTTLE_REASON_THERMAL_LIMIT 0x0000000000000002LL
#define NVML_CLK_THROTTLE_REASON_RELIABILITY 0x0000000000000004LL
#define NVML_CLK_THROTTLE_REASON_SW_POWER_CAP 0x0000000000000008LL


static bool nvmlInitialized = false;
static const int CPU_FAN = 2;       // Index of the CPU fan, based on motherboard specifications. For me it is a #2 on Nuvoton NCT6796D-R chip
static const int CPU_SPEED = 1800;  // Maximum CPU fan speed in RPM, based on CPU cooler specs
static const int MIN_INTERVAL = 300; // Minimum refresh interval in milliseconds


// Class for monitoring CPU
public ref class HardwareMonitor abstract sealed {
public:
    static Computer^ computer = nullptr;
    // Initialize the hardware monitor
    static void Initialize() {
        if (computer == nullptr) {
            computer = gcnew Computer();
            computer->IsCpuEnabled = true;
            computer->IsMotherboardEnabled = true,
            computer->Open();
        }
    }
    // Close the hardware monitor
    static void Close() {
        if (computer != nullptr) {
            computer->Close();
            computer = nullptr;
        }
    }
};


// Check if the program is running with administrative privileges
bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
        &ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) 
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}


// Check if NVML (NVIDIA Management Library) is initialized
bool checkNvmlInitialized(char* errorMsg, size_t bufSize) {
    if (!nvmlInitialized) {
        snprintf(errorMsg, bufSize, "NVML not initialized");
        return false;
    }
    return true;
}


// Get the current CPU load in percentage
int GetCpuLoad() {
    HardwareMonitor::Initialize();

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            hardware->Update();

            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Load && sensor->Name == "CPU Total") {
                    return static_cast<int>(sensor->Value.GetValueOrDefault(0.0f));
                }
            }
        }
    }
    return -1; // Return -1 if the sensor is not found or the value is unavailable
}


// Get the current CPU power consumption in watts
int GetCpuPower() {
    HardwareMonitor::Initialize();

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            hardware->Update();
            
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Power && sensor->Name->Contains("Package")) {
                    return static_cast<int>(sensor->Value.GetValueOrDefault(0.0f));
                }
            }
        }
    }
    return -1; // Return -1 if the sensor is not found or the value is unavailable
}


// Get the current CPU temperature in degrees Celsius
int GetCpuTemperature() {
    HardwareMonitor::Initialize();

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            hardware->Update();

            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature && sensor->Name == "CPU Package") {
                    // Получаем значение температуры и округляем
                    return static_cast<int>(sensor->Value.GetValueOrDefault(0.0f));
                }
            }
        }
    }
    return -1; // Return -1 if the sensor is not found or the value is unavailable
}


// Get the current CPU fan speed as a percentage of the maximum speed
int GetCpuFanSpeed(int fanIndex, int maxFanSpeed) {
    HardwareMonitor::Initialize();
    int currentFanIndex = 0;

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        hardware->Update();
        // Check subcomponents (e.g., additional sensors on the motherboard)
        for each (IHardware ^ subHardware in hardware->SubHardware) {
            subHardware->Update();

            for each (ISensor ^ subSensor in subHardware->Sensors) {
                if (subSensor->SensorType == SensorType::Fan) {
                    if (currentFanIndex == fanIndex) {
                        float currentSpeed = subSensor->Value.GetValueOrDefault(0.0f);
                        if (currentSpeed == 0.0f) return 0; // Return 0 if the fan is not spinning.
                        // Convert speed to percentage
                        return static_cast<int>((currentSpeed / maxFanSpeed) * 100.0f);
                    }
                    currentFanIndex++;
                }
            }
        }
    }
    return -1; // Return -1 if the sensor is not found or the value is unavailable
}


// Get the current CPU fan speed in RPM
int GetCpuFanSpeedRPM(int fanIndex, int maxFanSpeed) {
    HardwareMonitor::Initialize();
    int currentFanIndex = 0;

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        hardware->Update();
        // Check subcomponents (e.g., additional sensors on the motherboard)
        for each (IHardware ^ subHardware in hardware->SubHardware) {
            subHardware->Update();

            for each (ISensor ^ subSensor in subHardware->Sensors) {
                if (subSensor->SensorType == SensorType::Fan) {
                    if (currentFanIndex == fanIndex) {
                        return static_cast<int>(subSensor->Value.GetValueOrDefault(0.0f));
                    }
                    currentFanIndex++;
                }
            }
        }
    }
    return -1; // Return -1 if the sensor is not found or the value is unavailable
}


// Get the current CPU clock frequency in MHz
float GetCpuFrequency() {
    HardwareMonitor::Initialize();

    for each (IHardware ^ hardware in HardwareMonitor::computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            hardware->Update();

            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Clock && sensor->Name == "CPU Core #1") {
                    return sensor->Value.GetValueOrDefault(0.0f);
                }
            }
        }
    }
    return -1;  // Return -1 if the sensor is not found or the value is unavailable
}



/*********************************************************
 *         SmartieInit                                   *
 *********************************************************/
 // Initializes the Smartie plugin. Checks for administrative privileges,
 // initializes the NVML library for GPU monitoring, and sets up the hardware monitor

extern "C" DLLEXPORT void __stdcall SmartieInit() {
    
    if (!IsRunningAsAdmin()) {
        MessageBoxA(0, "Administrative privileges required for this plugin", "Error", MB_OK);
    }

    if (!nvmlInitialized) {
        // Attempt to initialize NVML (NVIDIA Management Library) for GPU monitoring
        nvmlReturn_t result = nvmlInit();
        if (result == NVML_SUCCESS) {
            nvmlInitialized = true;
        }
        else {
            MessageBoxA(0, nvmlErrorString(result), "NVML Init Failed", MB_OK);
        }
    }

    try {
        // Initialize the CPU hardware monitor
        HardwareMonitor::Initialize();
    }
    catch (System::Exception^ ex) {
        MessageBoxA(0, (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(ex->Message)).ToPointer(),
            "Initialization Error", MB_OK);
    }
}

/*********************************************************
 *         SmartieFini                                   *
 *********************************************************/
 // Cleans up and shuts down the Smartie plugin. Releases resources and closes NVML and hardware monitor

extern "C" DLLEXPORT void __stdcall SmartieFini() {
    if (nvmlInitialized) {
        // Shut down NVML
        nvmlShutdown();
        nvmlInitialized = false;
        
        // Close the hardware monitor
        HardwareMonitor::Close();
    }
}

/*********************************************************
 *         GetMinRefreshInterval                         *
 *********************************************************/
 // Returns the minimum refresh interval in milliseconds for sensor data updates

extern "C" DLLEXPORT int __stdcall GetMinRefreshInterval() {
    return MIN_INTERVAL;
}


/*********************************************************
 *         Function 1                                    *
 *  Returns CPU sensors data                             *
 *********************************************************/
 // Function to retrieve various CPU sensor data (load, power, temperature, fan speed, etc.)
 // based on the parameter provided by the user

extern "C" __declspec(dllexport) char* __stdcall function1(char* param1, char* param2) {
    static char tempStr[256];
    memset(tempStr, 0, sizeof(tempStr));

    bool showUnits = (strcmp(param2, "1") == 0);

    if (strcmp(param1, "Load") == 0) {
        // Retrieve CPU load percentage
        int load = GetCpuLoad();
        if (load < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading CPU Load");
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u%%" : "%u", load);
        }
        return tempStr;
    }

    else if (strcmp(param1, "Power") == 0) {
        // Retrieve CPU power consumption
        int tdp = GetCpuPower();
        if (tdp < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading CPU Power");
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%uW" : "%u", tdp);
        }
        return tempStr;
    }

    else if (strcmp(param1, "Temp") == 0) {
        // Retrieve CPU temperature
        int temp = GetCpuTemperature();
        if (temp < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading CPU Temp");
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u°C" : "%u", temp);
        }
        return tempStr;
    }
   
    else if (strcmp(param1, "Fan_RPM") == 0) {
        // Retrieve CPU Fan speed in RPM
        int fanSpeedRPM = GetCpuFanSpeedRPM(CPU_FAN, CPU_SPEED);
        if (fanSpeedRPM < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading Fan Speed");
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%uRPM" : "%u", fanSpeedRPM);
        }
        return tempStr;
    }

    else if (strcmp(param1, "Fan") == 0) {
        // Retrieve CPU Fan speed in %
        int fanSpeed = GetCpuFanSpeed(CPU_FAN, CPU_SPEED);
        if (fanSpeed < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading Fan Speed");
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u%%" : "%u", fanSpeed);
        }
        return tempStr;
    }

    else if (strcmp(param1, "Clock") == 0) {
        // Retrieve CPU Clock for first core
        float clock = GetCpuFrequency();
        if (clock < 0) {
            snprintf(tempStr, sizeof(tempStr), "Error reading CPU clock");
        }
        else {
            float clockGHz = static_cast<float>(clock) / 1000.0f;
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%.2fGHz" : "%.2f", clockGHz);
        }
        return tempStr;
    }

    snprintf(tempStr, sizeof(tempStr), "Invalid parameter");
    return tempStr;
}



/*********************************************************
 *         Function 2                                    *
 *  Returns GPU sensors data                             *
 *********************************************************/
 // Function to retrieve various GPU sensor data (temperature, fan speed, power usage, memory usage, etc.)
 // using the NVML library

extern "C" DLLEXPORT char* __stdcall function2(char* param1, char* param2) {
    static char tempStr[256];
    memset(tempStr, 0, sizeof(tempStr));

    if (!checkNvmlInitialized(tempStr, sizeof(tempStr))) {
        // Return an error message if NVML is not initialized
        return tempStr;
    }

    nvmlDevice_t device;
    nvmlReturn_t result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        // Return an error message if the GPU handle cannot be obtained
        snprintf(tempStr, sizeof(tempStr), "GPU handle error: %s", nvmlErrorString(result));
        return tempStr;
    }

    bool showUnits = (strcmp(param2, "1") == 0);

    if (strcmp(param1, "Temp") == 0) {
        // Retrieve GPU temperature
        unsigned int temp;
        result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting temp: %s", nvmlErrorString(result));
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u°C" : "%u", temp);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Limit") == 0) {
        // Retrieve symbol '!' if GPU power limit is reached
        unsigned long long throttleReasons;
        result = nvmlDeviceGetCurrentClocksThrottleReasons(device, &throttleReasons);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting throttle reasons: %s", nvmlErrorString(result));
        }
        else {
            snprintf(tempStr, sizeof(tempStr), throttleReasons & NVML_CLK_THROTTLE_REASON_RELIABILITY ? "!" : " ");
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Fan") == 0) {
        // Retrieve GPU Fan speed in %
        unsigned int fanSpeed;
        result = nvmlDeviceGetFanSpeed(device, &fanSpeed);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting fan speed: %s", nvmlErrorString(result));
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u%%" : "%u", fanSpeed);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Power") == 0) {
        // Retrieve GPU power consumption
        unsigned int power;
        result = nvmlDeviceGetPowerUsage(device, &power);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting power usage: %s", nvmlErrorString(result));
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%uW" : "%u", (unsigned int)(power / 1000.0 + 0.5));
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Clock") == 0) {
        // Retrieve GPU core clock
        unsigned int clock;
        result = nvmlDeviceGetClock(device, NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_CURRENT, &clock);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting GPU clock: %s", nvmlErrorString(result));
        }
        else {
            float clockGHz = static_cast<float>(clock) / 1000.0f;
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%.2fGHz" : "%.2f", clockGHz);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Mem_Clock") == 0) {
        // Retrieve GPU memory clock
        unsigned int memClock;
        result = nvmlDeviceGetClock(device, NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &memClock);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting Memory clock: %s", nvmlErrorString(result));
        }
        else {
            float memClockGHz = static_cast<float>(memClock) / 1000.0f;
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%.2fGHz" : "%.2f", memClockGHz);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Mem_Alloc") == 0) {
        // Retrieve GPU memory allocation
        nvmlMemory_t memInfo;
        result = nvmlDeviceGetMemoryInfo(device, &memInfo);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting memory usage: %s", nvmlErrorString(result));
        }
        else {
            float memUsage = (float)memInfo.used / (1024 * 1024 * 1024);
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%.1fGb" : "%.1f", memUsage);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Mem_Usage") == 0) {
        // Retrieve GPU memory usage in %
        nvmlMemory_t memInfo;
        result = nvmlDeviceGetMemoryInfo(device, &memInfo);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting memory usage: %s", nvmlErrorString(result));
        }
        else {
            unsigned int memUsage = (unsigned int)((memInfo.used * 100) / memInfo.total);
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u%%" : "%u", memUsage);
        }
        return tempStr;
    }
    
    else if (strcmp(param1, "Load") == 0) {
        // Retrieve GPU load percentage
        nvmlUtilization_t utilization;
        result = nvmlDeviceGetUtilizationRates(device, &utilization);
        if (result != NVML_SUCCESS) {
            snprintf(tempStr, sizeof(tempStr), "Error getting GPU load: %s", nvmlErrorString(result));
        }
        else {
            snprintf(tempStr, sizeof(tempStr), showUnits ? "%u%%" : "%u", utilization.gpu);
        }
        return tempStr;
    }
    
    snprintf(tempStr, sizeof(tempStr), "Invalid parameter");
    return tempStr;
}
