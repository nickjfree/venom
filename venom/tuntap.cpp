#include "tuntap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NETWORK_ADAPTERS L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#define TAP_CONTROL_CODE(request,method) CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_SET_MEDIA_STATUS   TAP_CONTROL_CODE(6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN         TAP_CONTROL_CODE(10, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ   TAP_CONTROL_CODE (7, METHOD_BUFFERED)

static char * reg_query(LPCWSTR key_name, wchar_t * result) {
	HKEY adapters, adapter;
	DWORD i, ret, len;
	char *deviceid = NULL;
	DWORD sub_keys = 0;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &adapters);
	if (ret != ERROR_SUCCESS) {
		printf("error open %s\n", (char*)key_name);
		return NULL;
	}
	ret = RegQueryInfoKey(adapters, NULL, NULL, NULL, &sub_keys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	if (ret != ERROR_SUCCESS) {
		printf("error open sub key\n");
		return NULL;

	}
	if (sub_keys <= 0) {
		printf("Wrong registry key");
		return NULL;
	}

	/* Walk througt all adapters */
	for (i = 0; i < sub_keys; i++) {
		WCHAR new_key[MAX_KEY_LENGTH];
		char data[256];
		TCHAR key[MAX_KEY_LENGTH];
		DWORD keylen = MAX_KEY_LENGTH;
		/* Get the adapter key name */
		ret = RegEnumKeyEx(adapters, i, key, &keylen, NULL, NULL, NULL, NULL);
		if (ret != ERROR_SUCCESS) {
			continue;
		}
		/* Append it to NETWORK_ADAPTERS and open it */
		wsprintf(new_key, L"%s\\%s", key_name, key);
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, new_key, 0, KEY_READ, &adapter);
		if (ret != ERROR_SUCCESS) {
			continue;
		}
		/* Check its values */
		len = sizeof(data);
		ret = RegQueryValueEx(adapter, L"ComponentId", NULL, NULL, (LPBYTE)data, &len);

		if (ret != ERROR_SUCCESS) {
			/* This value doesn't exist in this adaptater tree */
			goto clean;
		}

		/* If its a tap adapter, its all good */

		if (wcscmp((wchar_t*)data, L"tap0901") == 0) {

			DWORD type;
			len = sizeof data;
			ret = RegQueryValueEx(adapter, L"NetCfgInstanceId", NULL, &type, (LPBYTE)data, &len);
			if (ret != ERROR_SUCCESS) {
				// tuntap_log(TUNTAP_LOG_INFO, (const char *)formated_error(L"%1", ret));
				printf("error query %s\n", "NetCfgInstanceId");
				goto clean;
			}
			wcscpy_s(result, 256, (wchar_t *)data);
			break;
		}
	clean:
		RegCloseKey(adapter);
	}
	RegCloseKey(adapters);
	return deviceid;
}

int tuntap_start(device * dev, int mode, int tun) {
	wchar_t deviceid[256];
	wchar_t filename[256];
	reg_query(NETWORK_ADAPTERS, deviceid);
	// create file
	wsprintf(filename, L"\\\\.\\Global\\%s.tap", deviceid);
	HANDLE tun_fd = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM| FILE_FLAG_OVERLAPPED, 0);

	if (tun_fd == (HANDLE)TUNFD_INVALID_VALUE) {

		int errcode = GetLastError();
		printf("create device failed %d\n", errcode);
		return -1;
	}
	dev->tun_fd = tun_fd;

	//setup device to up
	DWORD flag = 1;
	DWORD len;
	if (DeviceIoControl(dev->tun_fd, TAP_IOCTL_SET_MEDIA_STATUS, &flag, sizeof(flag), &flag, sizeof(flag), &len, NULL) == 0) {
		int errcode = GetLastError();
		return -1;
	}
	// setup tun ip address 192.168.0.1 192.168.0.0 255.255.255.0
	unsigned char data[12] = {192, 168, 0, 1, 192, 168, 0, 0, 255, 255, 255, 0};
	unsigned int  ep[4] = { 0, 0, 0, 0};
	inet_pton(AF_INET, "192.168.0.1", &ep[0]);
	inet_pton(AF_INET, "255.255.255.0", &ep[1]);
	ep[2] = 0xFE00000A;
	ep[3] = 0x00FFFFFF;
	if (DeviceIoControl(dev->tun_fd, TAP_IOCTL_CONFIG_DHCP_MASQ, &ep, sizeof(ep), &ep, sizeof(ep), &len, NULL) == 0) {

		int errcode = GetLastError();
		return -1;
	}
	if (DeviceIoControl(dev->tun_fd, TAP_IOCTL_CONFIG_TUN, &data, sizeof(data), &data, sizeof(data), &len, NULL) == 0) {
		int errcode = GetLastError();
		return -1;
	}
	return 0;
}

int tuntap_read(device * dev, void * buff, size_t size) {
	DWORD nread;
	if (!ReadFile(dev->tun_fd, buff, size, &nread, NULL)) {
		printf("read tun error %d\n", GetLastError());
	}
	printf("tun read %d bytes\n", nread);
	return nread;
}

int tuntap_write(device * dev, void * buff, size_t size) {
	DWORD nread;
	if (!WriteFile(dev->tun_fd, buff, size, &nread, NULL)) {
		printf("write tun error %d\n", GetLastError());
	}
	printf("tun write %d bytes\n", nread);
	return nread;
}