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

#include "stdafx.h"
#include ".\sharedmailslot.h"

typedef enum _SE_OBJECT_TYPE
{
	SE_UNKNOWN_OBJECT_TYPE = 0,
	SE_FILE_OBJECT,
	SE_SERVICE,
	SE_PRINTER,
	SE_REGISTRY_KEY,
	SE_LMSHARE,
	SE_KERNEL_OBJECT,
	SE_WINDOW_OBJECT,
	SE_DS_OBJECT,
	SE_DS_OBJECT_ALL,
	SE_PROVIDER_DEFINED_OBJECT,
	SE_WMIGUID_OBJECT,
	SE_REGISTRY_WOW64_32KEY
} SE_OBJECT_TYPE;

typedef DWORD (STDMETHODCALLTYPE FAR* LPSETNAMEDSECURITYINFO) (LPTSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION, PSID, PSID, PACL, PACL);

CSharedMailslot::CSharedMailslot(void)
{
	mailslotSlaveName=(char *)malloc(CSHAREDMAILSLOT_CHARS_MAX);
	mailslotName=(char *)malloc(CSHAREDMAILSLOT_CHARS_MAX);
	ipcMutexName=(char *)malloc(CSHAREDMAILSLOT_CHARS_MAX);
	ipcName=(char *)malloc(CSHAREDMAILSLOT_CHARS_MAX);
	mailslotSlaveHandle=INVALID_HANDLE_VALUE;
	mailslotHandle=INVALID_HANDLE_VALUE;
	ipcFileMapping=NULL;
	libADVAPI32=NULL;
	isBufferUsed=false;
	isMailslotServer=false;
}

CSharedMailslot::~CSharedMailslot(void)
{
	Close();
	free(ipcName);
	free(ipcMutexName);
	free(mailslotName);
	free(mailslotSlaveName);

	if (libADVAPI32!=NULL)
		FreeLibrary(libADVAPI32);
}

bool CSharedMailslot::isOpen(void)
{
	return ( (mailslotHandle != INVALID_HANDLE_VALUE) || (isShared()&&!isInstanceOwner()) );
}

bool CSharedMailslot::isShared(void)
{
	return (ipcFileMapping != NULL);
}

bool CSharedMailslot::isServer(void)
{
	return (isMailslotServer);
}

bool CSharedMailslot::isInstanceOwner(void)
{
	if (ipcFileMapping==NULL)
		return false;

	return (ipcInstancesView->instanceOwner==instanceIndex);
};

bool CSharedMailslot::Close(void)
{
	if (isShared())
	{

		if (WaitForSingleObject(ipcMutex, CSHAREDMAILSLOT_IPC_WAIT)==WAIT_TIMEOUT)
			return false;

		if (isInstanceOwner())
		{
			DWORD bufferSizeUsed, messagesWaiting=-1;
			char *dummy;

			// flushes pending messages to the other slave mailslots
			while (messagesWaiting!=0)
				Read(dummy, bufferSizeUsed, messagesWaiting, true);

			ipcInstancesView->instanceOwner=0;
		}
		ipcInstancesView->instances[instanceIndex]=0;
		instanceIndex=0;

		ReleaseMutex(ipcMutex);

		UnmapViewOfFile(ipcInstancesView);
		CloseHandle(mailslotSlaveHandle);
		CloseHandle(ipcFileMapping);
		CloseHandle(ipcMutex);
		ipcFileMapping=NULL;
		mailslotSlaveHandle=INVALID_HANDLE_VALUE;
	}

	if (mailslotHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mailslotHandle);
		mailslotHandle=INVALID_HANDLE_VALUE;
		isMailslotServer=false;
	}

	if (isBufferUsed)
	{
		free(mailslotBuffer);
		isBufferUsed=false;
	}

	return true;
}

char *CSharedMailslot::GetMailslotName(bool full)
{
	// this method returns either the given mailslot name (full=false) or the
	// full mailslot name (full=true) constructed by the BuildSharedNames() method
	if (isOpen())
	{
		if (full)
			return mailslotName;
		else
			return strrchr(mailslotName, '\\')+1;
	}

	return NULL;
}

void CSharedMailslot::BuildSharedNames(char *mailslotName)
{
	// NT4 does not permit Global session name space prefix 
	if (isSharedAllowed()==CSHAREDMAILSLOT_SYSREQ_OKNOWTS)
		strcpy(ipcName, "CSharedMailslot");
	else
		strcpy(ipcName, "Global\\CSharedMailslot");

	strcat(ipcName, mailslotName);
	strcpy(ipcMutexName, ipcName);
	strcat(ipcMutexName, "Mutex");

	BuildSharedMailslotName(mailslotSlaveName, GetCurrentThreadId(), true);
}

void CSharedMailslot::BuildSharedMailslotName(char *nameBuffer, DWORD id, bool owned)
{
	char mailslotPrefix[CSHAREDMAILSLOT_CHARS_MAX], buffer[CSHAREDMAILSLOT_CHARS_MAX], threadId[8];
	int k=0, d, buflen;

	sprintf(mailslotPrefix, "\\\\%s\\mailslot\\", (owned?".":"."));
	itoa(id, threadId, 16);
	strcpy(nameBuffer, mailslotPrefix);
	strcpy(buffer, ipcName);
	strcat(buffer, threadId);
	buflen=strlen(buffer);
	d=strlen(mailslotPrefix);

	// looks tricky, and it is :-)
	while(k<buflen)
	{
		// basically, the loop adds a '\' char every 8 characters, converting
		// a long name (not supported on 9x platforms) to a folder path
		// (which is supported instead)
		strncpy(&nameBuffer[d], &buffer[k], 8);
		d+=(strlen(&buffer[k])>7?8:strlen(&buffer[k])); k+=8;
		strcpy(&nameBuffer[d++], "\\");
	}
	nameBuffer[--d]=0;
}

bool CSharedMailslot::Open(bool isMailslotServer, char *mailslotName, char *destinationName, bool shared)
{
	this->isMailslotServer=isMailslotServer;

	if (isMailslotServer)
	{
		LPSETNAMEDSECURITYINFO SetNamedSecurityInfo;

		if (shared)
		{
			// the shared feature is supported on NT-based kernels only,
			// but since this class can act as a normal mailslots wrapper,
			// the SetNamedSecurityInfo() is loaded dynamically to permit
			// execution on win9x platforms
			if (isSharedAllowed()!=CSHAREDMAILSLOT_SYSREQ_KO)
			{
				if (libADVAPI32==NULL)
				{
					libADVAPI32 = LoadLibrary( _T("advapi32.dll") );
					if (!libADVAPI32)
						return false;

					SetNamedSecurityInfo = (LPSETNAMEDSECURITYINFO) GetProcAddress( libADVAPI32, "SetNamedSecurityInfoA" ); 
					if (!SetNamedSecurityInfo)
						return false;
				}
			}
			else
				shared=false;
		}

		if (shared)
		{
			// if this is a server (ie. listen for incoming messages),
			// all required shared structs are defined and opened

			int counter=1;

			BuildSharedNames(mailslotName);

			ipcMutex=CreateMutex(NULL, FALSE, ipcMutexName);
			if (ipcMutex==NULL)
				return false;

			ipcFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ipcInstances), ipcName);
			if (ipcFileMapping==NULL)
			{
				CloseHandle(ipcMutex);
				return false;
			}

			SetNamedSecurityInfo(ipcName, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL) NULL, NULL);

			ipcInstancesView=(ipcInstances *)MapViewOfFile(ipcFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			if (ipcInstancesView==NULL)
			{
				CloseHandle(ipcFileMapping);
				CloseHandle(ipcMutex);
				ipcFileMapping=NULL;
				return false;
			}

			// this is the 'slave mailslot', ie. the CSharedMailslot internal mailslot which
			// listens for incoming messages from the instanceOwner
			mailslotSlaveHandle = CreateMailslot(mailslotSlaveName, 0, MAILSLOT_WAIT_FOREVER, NULL);
			if (mailslotSlaveHandle==INVALID_HANDLE_VALUE)
			{
				UnmapViewOfFile(ipcInstancesView);
				CloseHandle(ipcFileMapping);
				CloseHandle(ipcMutex);
				ipcFileMapping=NULL;
				return false;
			}

			// the CSHAREDMAILSLOT_IPC_WAIT is an arbitrary timeout value
			if (WaitForSingleObject(ipcMutex, CSHAREDMAILSLOT_IPC_WAIT)==WAIT_TIMEOUT)
			{
				UnmapViewOfFile(ipcInstancesView);
				CloseHandle(ipcFileMapping);
				CloseHandle(ipcMutex);
				ipcFileMapping=NULL;
				return false;
			}

			while (counter<CSHAREDMAILSLOT_SLOTS_MAX)
			{
				if (ipcInstancesView->instances[counter]==0)
				{
					ipcInstancesView->instances[counter]=GetCurrentThreadId();
					instanceIndex=counter;

					if (ipcInstancesView->instanceOwner==0)
						ipcInstancesView->instanceOwner=counter;
					break;
				}

				counter++;
			}

			ReleaseMutex(ipcMutex);

			if (counter==CSHAREDMAILSLOT_SLOTS_MAX)
			{
				// no more available shared slots
				UnmapViewOfFile(ipcInstancesView);
				CloseHandle(ipcFileMapping);
				CloseHandle(ipcMutex);
				ipcFileMapping=NULL;
				return false;
			}
		}

		sprintf(this->mailslotName, "\\\\.\\mailslot\\%s", mailslotName);
		if ( !shared || (shared && isInstanceOwner()) )
		{
			// this is the main mailslot, ie. the one which listens for messages coming
			// from other processes\workstations. if this is a shared server, then this
			// mailslot is created only if this is the instanceOwner
			this->mailslotHandle = CreateMailslot(this->mailslotName, 0, MAILSLOT_WAIT_FOREVER,	NULL);

			if (!isOpen())
			{
				if (shared)
				{
					UnmapViewOfFile(ipcInstancesView);
					CloseHandle(mailslotSlaveHandle);
					CloseHandle(ipcFileMapping);
					CloseHandle(ipcMutex);
					ipcFileMapping=NULL;
				}
				
				return false;
			}
		}		
		
		return true;
	}

	if (destinationName==NULL)
		return false;

	// this is the client mailslot (ie. the one used to send messages)
	sprintf(this->mailslotName, "\\\\%s\\mailslot\\%s", destinationName, mailslotName);
	this->mailslotHandle = CreateFile(this->mailslotName, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,	NULL);

	return (isOpen());
}

bool CSharedMailslot::Write(char *text, int textLength, char *mailslotName, char *destinationName)
{
	DWORD writtenLength;
	int towriteLength;

	if (text==NULL || isMailslotServer)
		return false;

	if (textLength==NULL)
		towriteLength=sizeof(text);
	else
		towriteLength=textLength;

	if (isOpen())
		return (WriteFile(mailslotHandle, text, towriteLength, &writtenLength, NULL));
	else
	{
		if (mailslotName==NULL || destinationName==NULL)
			return false;

		// if the mailslot is not open, then it is automatically opened then closed
		// (user can choose to call the Write() function only)
		if (Open(false, mailslotName, destinationName))
			if (WriteFile(mailslotHandle, text, towriteLength, &writtenLength, NULL))
			{
				Close();
				return true;
			}
	}

	return false;
}

bool CSharedMailslot::Read(char *&buffer, DWORD &bufferSize, DWORD &messagesWaiting, bool isThreadSafe)
{
	DWORD bufferSizeUsed, otherMessagesWaiting;
	char *dummy;

	if (!isOpen() || !isMailslotServer)
		return false;

	if (isShared())
	{
		if (isInstanceOwner())
		{
			// even if this is the instanceOwner, maybe there are some msgs pending on the slave mailslot
			if (!ReadMailslot(mailslotSlaveHandle, buffer, bufferSize, messagesWaiting))
				return false;

			if (bufferSize>0)
			{
				if (!ReadMailslot(mailslotHandle, dummy, bufferSizeUsed, otherMessagesWaiting, true))
					return false;

				messagesWaiting+=otherMessagesWaiting;
				return true;
			}

			// pending messages on main mailslot (usual condition if this is the instanceOwner)
			if (!ReadMailslot(mailslotHandle, buffer, bufferSize, messagesWaiting))
				return false;

			// forward message to all non-instanceOwner instances; comment this call to
			// behave as the messenger service does (only this instance will receive the message)
			if (bufferSize>0)
				Forward(buffer, bufferSize);

			return true;
		}
		else
		{
			// pending messages on slave mailslot (usual condition if this is a slave instance)
			if (!ReadMailslot(mailslotSlaveHandle, buffer, bufferSize, messagesWaiting))
				return false;

			if (!isThreadSafe)
				if (WaitForSingleObject(ipcMutex, CSHAREDMAILSLOT_IPC_WAIT)==WAIT_TIMEOUT)
					return false;

			// try to become instanceOwner (in the case the instanceOwner thread exited or crashed)
			HANDLE gainMailslotHandle=CreateMailslot(this->mailslotName, 0, MAILSLOT_WAIT_FOREVER, NULL);
			if (gainMailslotHandle!=INVALID_HANDLE_VALUE)
			{
				mailslotHandle=gainMailslotHandle;
				ipcInstancesView->instances[ipcInstancesView->instanceOwner]=0;
				ipcInstancesView->instanceOwner=instanceIndex;
			}

			if (!isThreadSafe)
				ReleaseMutex(ipcMutex);

			return true;
		}
	}
	else
	{
		if (ReadMailslot(mailslotHandle, buffer, bufferSize, messagesWaiting))
			return true;
	}

	return false;
}

bool CSharedMailslot::ReadMailslot(HANDLE mailslotHandle, char *&buffer, DWORD &bufferSize, DWORD &messagesWaiting, bool messagesWaitingOnly)
{
	DWORD bufferSizeUsed;

	if (GetMailslotInfo(mailslotHandle, NULL, &bufferSize, &messagesWaiting, NULL))
	{
		// the following check for nonsense could detect a mailslot abnormal condition
		if ( (messagesWaiting>0 && (bufferSize<0 || bufferSize>65535)) || messagesWaiting>255)
			return false;

		if (bufferSize == MAILSLOT_NO_MESSAGE || messagesWaitingOnly)
		{
			bufferSize=0;
			return true;
		}

		if (isBufferUsed)
			free(mailslotBuffer);

		isBufferUsed=true;
		mailslotBuffer=(char *)calloc(bufferSize, 1);
		if (ReadFile(mailslotHandle, mailslotBuffer, bufferSize, &bufferSizeUsed, NULL))
		{
			buffer=mailslotBuffer;
			messagesWaiting--;
			return true;
		}
		else
		{
			free(mailslotBuffer);
			isBufferUsed=false;
			return false;
		}
	}

	return false;
}

bool CSharedMailslot::Forward(char *buffer, DWORD bufferSize)
{
	// forward message to all non-instanceOwner instances

	// even if this is normally used in Read() method, it can be called
	// from the external too: for example, it is called from the NetBios
	// listener in RealPopup whenever a msg is received from it

	char ipcMailslotSlaveName[CSHAREDMAILSLOT_CHARS_MAX];
	HANDLE ipcMailslotSlaveHandle;
	int towriteLength, counter=1;
	DWORD writtenLength;

	towriteLength=bufferSize;

	if (WaitForSingleObject(ipcMutex, CSHAREDMAILSLOT_IPC_WAIT)==WAIT_TIMEOUT)
		return false;

	while (counter<CSHAREDMAILSLOT_SLOTS_MAX)
	{
		if (counter!=instanceIndex)
		{
			if (ipcInstancesView->instances[counter]!=0)
			{
				BuildSharedMailslotName(ipcMailslotSlaveName, ipcInstancesView->instances[counter], false);

				ipcMailslotSlaveHandle = CreateFile(ipcMailslotSlaveName, GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,	NULL);

				WriteFile(ipcMailslotSlaveHandle, buffer, towriteLength, &writtenLength, NULL);
				CloseHandle(ipcMailslotSlaveHandle);
			}
		}
		counter++;
	}

	ReleaseMutex(ipcMutex);

	return true;
}

int CSharedMailslot::isSharedAllowed()
{
	// shared feature is allowed on NT kernel based platforms;
	// NT4 does not permit Global session name space prefix 

	OSVERSIONINFO vi = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&vi);

	if (vi.dwPlatformId!=VER_PLATFORM_WIN32_NT || vi.dwMajorVersion<4)
		return CSHAREDMAILSLOT_SYSREQ_KO;

	if (vi.dwMajorVersion==4)
		return CSHAREDMAILSLOT_SYSREQ_OKNOWTS;

	return (CSHAREDMAILSLOT_SYSREQ_OK);
}
