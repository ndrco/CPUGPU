The CPUGPU plugin is designed for use with LCDSmartie (https://github.com/LCD-Smartie/LCDSmartie) and allows displaying CPU and Nvidia GPU monitoring data on the screen.

The plugin essentially serves as a wrapper that utilizes the NVIDIA Management Library (NVML) (https://developer.nvidia.com/management-library-nvml) to access GPU sensors and the LibreHardwareMonitor library (https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) to retrieve CPU data.

The compiled CPUGPU.dll file should be placed in the \plugins directory of the LCDSmartie program.
The LibreHardwareMonitor.dll file must be placed in the root directory of the LCDSmartie program (not in the \plugins directory!). LibreHardwareMonitor requires .NET Framework 4.7.2 to function. Since LibreHardwareMonitor uses low-level access to CPU registers via WinRing0, the LCDSmartie program must be run with administrator privileges.
The NVIDIA Management Library requires the NVIDIA GPU Driver to operate.

CPUGPU Interface:

function 1: get CPU data

param1: 
Load        	// Retrieve CPU load percentage;
Power		// Retrieve CPU power consumption;
Temp		// Retrieve CPU temperature;
Fan_RPM	// Retrieve CPU Fan speed in RPM;
Fan			// Retrieve CPU Fan speed in %;
Clock		// Retrieve CPU Clock for first core;

param2=0: Hide units;
param2=1: Show units;


function 2: get GPU data

param1: 
Load        	// Retrieve GPU load percentage;
Power		// Retrieve GPU power consumption;
Limit			// Retrieve symbol '!' if GPU power limit is reached;
Temp		// Retrieve GPU temperature;
Fan			// Retrieve GPU Fan speed in %;
Clock		// Retrieve GPU core clock;
Mem_Clock	// Retrieve GPU memory clock;
Mem_Alloc	// Retrieve GPU memory allocation;
Mem_Usage	// Retrieve GPU memory usage in %;

param2=0: Hide units;
param2=1: Show units;

By utilizing the capabilities of NVML and LibreHardwareMonitor, you can easily extend the plugin to retrieve other data you may require.
Enjoy!
