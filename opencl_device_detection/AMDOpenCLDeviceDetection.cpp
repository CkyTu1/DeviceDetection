#include "AMDOpenCLDeviceDetection.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "json_helpers.h"

using namespace std;
using namespace cl;

AMDOpenCLDeviceDetection::AMDOpenCLDeviceDetection()
{
}

AMDOpenCLDeviceDetection::~AMDOpenCLDeviceDetection()
{
}

vector<Platform> AMDOpenCLDeviceDetection::getPlatforms() {
	vector<Platform> platforms;
	try {
		Platform::get(&platforms);
	} catch (Error const& err) {
#if defined(CL_PLATFORM_NOT_FOUND_KHR)
		if (err.err() == CL_PLATFORM_NOT_FOUND_KHR)
			throw exception("No OpenCL platforms found");
		else
#endif
			throw err;
	}
	return platforms;
}

vector<Device> AMDOpenCLDeviceDetection::getDevices(vector<Platform> const& _platforms, unsigned _platformId) {
	vector<Device> devices;
	try {
		_platforms[_platformId].getDevices(/*CL_DEVICE_TYPE_CPU| */CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, &devices);
	} catch (Error const& err) {
		// if simply no devices found return empty vector
		if (err.err() != CL_DEVICE_NOT_FOUND)
			throw err;
	}
	return devices;
}

string AMDOpenCLDeviceDetection::StringnNullTerminatorFix(const string& str) {
	return string(str.c_str(), strlen(str.c_str()));
}

bool AMDOpenCLDeviceDetection::QueryDevices() {
	try {
		// get platforms
		auto platforms = getPlatforms();
		if (platforms.empty()) {
			_statusString = "No OpenCL platforms found";
			cout << "No OpenCL platforms found" << endl;
			return false;
		}
		else {
			for (auto i_pId = 0u; i_pId < platforms.size(); ++i_pId) {
				string platformName = StringnNullTerminatorFix(platforms[i_pId].getInfo<CL_PLATFORM_NAME>());
				if (std::find(_platformNames.begin(), _platformNames.end(), platformName) == _platformNames.end()) {
					JsonLog current;
					_platformNames.push_back(platformName);
					// new
					current.PlatformName = platformName;
					current.PlatformNum = i_pId;

					// not the best way but it should work
					bool isAMD = platformName.find("AMD", 0) != string::npos;
					auto clDevs = getDevices(platforms, i_pId);
					for (auto i_devId = 0u; i_devId < clDevs.size(); ++i_devId) {
						OpenCLDevice curDevice;
						curDevice.DeviceID = i_devId;
						curDevice._CL_DEVICE_NAME = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_NAME>());
						switch (clDevs[i_devId].getInfo<CL_DEVICE_TYPE>()) {
						case CL_DEVICE_TYPE_CPU:
							curDevice._CL_DEVICE_TYPE = "CPU";
							break;
						case CL_DEVICE_TYPE_GPU:
							curDevice._CL_DEVICE_TYPE = "GPU";
							break;
						case CL_DEVICE_TYPE_ACCELERATOR:
							curDevice._CL_DEVICE_TYPE = "ACCELERATOR";
							break;
						default:
							curDevice._CL_DEVICE_TYPE = "DEFAULT";
							break;
						}

						curDevice._CL_DEVICE_GLOBAL_MEM_SIZE = clDevs[i_devId].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
						curDevice._CL_DEVICE_VENDOR = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_VENDOR>());
						curDevice._CL_DEVICE_VERSION = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_VERSION>());
						curDevice._CL_DRIVER_VERSION = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DRIVER_VERSION>());

						// AMD topology get Bus No
						if (isAMD) {
							cl_device_topology_amd topology = clDevs[i_devId].getInfo<CL_DEVICE_TOPOLOGY_AMD>();
							if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
								curDevice.AMD_BUS_ID = (int)topology.pcie.bus;
							}
                            curDevice._CL_DEVICE_BOARD_NAME_AMD = clDevs[i_devId].getInfo<CL_DEVICE_BOARD_NAME_AMD>();
						}

						current.Devices.push_back(curDevice);
					}
					_devicesPlatformsDevices.push_back(current);
				}
			}
		}
	}
	catch (exception &ex) {
		_errorString = ex.what();
		_statusString = "Error";
		return false;
	}

	return true;
}

string AMDOpenCLDeviceDetection::GetDevicesJsonString(bool prettyPrint) {
	return json_helpers::GetPlatformDevicesJsonString(_devicesPlatformsDevices, _statusString, _errorString, prettyPrint);
}

string AMDOpenCLDeviceDetection::GetErrorString() {
	return _errorString;
}