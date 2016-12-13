/*

CSharedMailslot build 1.006

code by matro
rome, italy, 2004
matro$realpopup%it

07.sep.2004 - 0.001 - first build for RealPopup build 149: IPC not implemented yet
09.nov.2004 - 1.004 - first public release (included in RealPopup build 155)
02.dec.2004 - 1.006 - released for Code Project

*warning* please update CSHAREDMAILSLOT_BUILD accordingly.

this code may be used in compiled form in any way you desire. you are not 
allowed to redistribute your version of CSharedMailslot: please change project 
name (this is to maintain a single distribution point for the source code). if 
you use this code, it would be nice the presence of an author acknowledgement. 
this code is released "as is": use it at your *own* risk and *expect bugs*. 
write me for comments, suggestions or to tell your story.

matro

*/

#pragma once

#define CSHAREDMAILSLOT_BUILD			"1.006"
#define CSHAREDMAILSLOT_IPC_WAIT		30000
#define CSHAREDMAILSLOT_SLOTS_MAX		255
#define CSHAREDMAILSLOT_CHARS_MAX		127
#define CSHAREDMAILSLOT_SYSREQ_KO		0
#define CSHAREDMAILSLOT_SYSREQ_OK		1
#define CSHAREDMAILSLOT_SYSREQ_OKNOWTS	2

class CSharedMailslot
{
public:
	CSharedMailslot(void);
	~CSharedMailslot(void);

	bool isServer(void);
	bool isOpen(void);
	bool isShared(void);
	bool isInstanceOwner(void);
	bool Close(void);
	bool Open(bool isMailslotServer, char *mailslotName, char *destinationName=NULL, bool shared=false);
	bool Write(char *text, int textLength=NULL, char *mailslotName=NULL, char *destinationName=NULL);
	bool Read(char *&buffer, DWORD &bufferSize, DWORD &messagesWaiting, bool isThreadSafe=false);
	bool Forward(char *buffer, DWORD bufferSize);
	char *GetMailslotName(bool full=false);

protected:
	struct ipcInstances
	{
		DWORD instanceOwner;
		DWORD instances[CSHAREDMAILSLOT_SLOTS_MAX]; // position [0] is not used
	};

	int  isSharedAllowed();
	void BuildSharedNames(char *mailslotName);
	void BuildSharedMailslotName(char *nameBuffer, DWORD id, bool owned);
	bool ReadMailslot(HANDLE mailslotHandle, char *&buffer, DWORD &bufferSize, DWORD &messagesWaiting, bool messagesWaitingOnly=false);

	HMODULE libADVAPI32;

	bool isBufferUsed;
	bool isMailslotServer;

	HANDLE mailslotHandle;
	char *mailslotName;
	char *mailslotBuffer;

	HANDLE ipcMutex;
	char *ipcMutexName;

	HANDLE ipcFileMapping;
	char *ipcName;
	ipcInstances *ipcInstancesView;
	int instanceIndex;

	HANDLE mailslotSlaveHandle;
	char *mailslotSlaveName;
};
