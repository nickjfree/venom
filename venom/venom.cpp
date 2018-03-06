// venom.cpp : Defines the entry point for the console application.
//
#include "tuntap.h"
#include <stdio.h>
//#include <Windows.h>


HANDLE hCompletionPort;

typedef struct Context {
	int net_fd;
	int remote_len;
	int sent;
	HANDLE tun_fd;
	sockaddr_in remote;
}Context;


int worker_thread() {
	while (1) {
		int ret = 0;
		PipeOperation * Operation = NULL;
		DWORD Transferred = 0;
		Context *context;
		ret = GetQueuedCompletionStatus(hCompletionPort, &Transferred, (PULONG_PTR)&context, (LPOVERLAPPED *)&Operation, INFINITE);
		if (!ret) {
			int code = GetLastError();
			printf("something wrong with %d \n", code);
		}
		else {
			if (context && Transferred) {
				// process data
				DWORD flags = 0;
				WSABUF Buffer = {};
				int ret;
				switch (Operation->Type) {
				case PipeOperation::RECV_TUN:
					printf("recv tun %d\n", Transferred);
					memset(&Operation->overlapped, 0, sizeof(OVERLAPPED));
					Operation->transferred = 0;
					Operation->Type = PipeOperation::SEND_NET;
					Buffer.buf = Operation->buffer;
					Buffer.len = Transferred;
					ret = WSASendTo(context->net_fd, &Buffer, 1, &Operation->transferred, 0, (sockaddr*)&context->remote, sizeof(context->remote), &Operation->overlapped, NULL);
					break;
				case PipeOperation::RECV_NET:
					printf("recv net %d\n", Transferred);
					memset(&Operation->overlapped, 0, sizeof(OVERLAPPED));
					Operation->transferred = 0;
					Operation->Type = PipeOperation::SEND_TUN;
					WriteFile(context->tun_fd, Operation->buffer, Transferred, &Operation->transferred, &Operation->overlapped);
					break;
				case PipeOperation::SEND_TUN:
					printf("sent tun %d\n", Transferred);
					memset(&Operation->overlapped, 0, sizeof(OVERLAPPED));
					Operation->transferred = 0;
					Operation->Type = PipeOperation::RECV_NET;
					Buffer.buf = Operation->buffer;
					Buffer.len = 2048;
					WSARecvFrom(context->net_fd, &Buffer, 1, 0, &flags, (sockaddr*)&context->remote, &context->remote_len, &Operation->overlapped, NULL);
					// recvfrom(context->net_fd, Operation->buffer, 2048, 0, (sockaddr*)&context->remote, &context->remote_len);
					break;
				case PipeOperation::SEND_NET:
					printf("sent net %d\n", Transferred);
					memset(&Operation->overlapped, 0, sizeof(OVERLAPPED));
					Operation->transferred = 0;
					Operation->Type = PipeOperation::RECV_TUN;
					ret = ReadFile(context->tun_fd, Operation->buffer, 2048, &Operation->transferred, &Operation->overlapped);
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}


int main(int argc, char **argv) {
	// setup tun
	device dev;
	tuntap_start(&dev, 0, 0);
	// setup socket
	WSADATA wsadata = {};
	WSAStartup(0x201, &wsadata);
	SOCKET net_fd = socket(AF_INET, SOCK_DGRAM, 0);

	// setup context
	Context context;
	context.net_fd = net_fd;
	context.tun_fd = dev.tun_fd;
	context.remote.sin_family = AF_INET;
	context.remote_len = sizeof(sockaddr_in);
	inet_pton(AF_INET, argv[1], &context.remote.sin_addr);
	context.remote.sin_port = htons(55555);
	// add all fd to iocp
	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 2);
	HANDLE hPort = CreateIoCompletionPort((HANDLE)net_fd, hCompletionPort, (ULONG_PTR)&context, 0);
	hPort = CreateIoCompletionPort((HANDLE)dev.tun_fd, hCompletionPort, (ULONG_PTR)&context, 0);

	for (int i = 0; i < 2; i++) {
		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)worker_thread, 0, 0, 0);
	}
	// triger the operations
	PipeOperation net_read, tun_read;
	memset(&net_read, 0, sizeof(PipeOperation));
	memset(&tun_read, 0, sizeof(PipeOperation));
	tun_read.Type = PipeOperation::RECV_TUN;
	BOOL ret = ReadFile(dev.tun_fd, tun_read.buffer, 2048, &tun_read.transferred, &tun_read.overlapped);

	WSABUF Buffer = {};
	DWORD flags = 0;
	net_read.Type = PipeOperation::RECV_NET;
	Buffer.buf = net_read.buffer;
	Buffer.len = 2048;
	int sent = sendto(context.net_fd, net_read.buffer, 1, 0, (sockaddr*)&context.remote, sizeof(context.remote));
	int recv_ret = WSARecvFrom(context.net_fd, &Buffer, 1, &net_read.transferred, &flags, (sockaddr*)&context.remote, &context.remote_len, &net_read.overlapped, NULL);
	printf("first rev from %d\n", GetLastError());
	while (1) {
		Sleep(10000);
	}
    return 0;
}

