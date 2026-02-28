#pragma once
// comm.h
#ifndef COMM_H
#define COMM_H

#ifdef __cplusplus
extern "C" {
#endif

	// Socket Communication

	int StartSocketSender(const char* message);
	int StartSocketReceiver();

	// Shared Memory Communication
	int StartSharedMemorySender(const char* message);
	int StartSharedMemoryReceiver();

	// Pipe Communication
	int StartPipeSender(const char* message);
	int StartPipeReceiver();

	// Mailslot Communication
	int StartMailslotSender(const char* message);
	int StartMailslotReceiver();

#ifdef __cplusplus
}
#endif

#endif // COMM_H
