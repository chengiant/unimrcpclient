#ifndef _APPMESSAGE_H_
#define _APPMESSAGE_H_

#include "mrcp_generic_header.h"
#include "mrcp_recog_header.h"
#include "mrcp_recog_resource.h"
#include "mrcp_client.h"


using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections::Concurrent;

ref class MrcpMessage : ucf::IMrcpMessage{
private:
	mrcp_message_t *mMessage;
	const apt_dir_layout_t *mDir;

public:
	MrcpMessage(mrcp_message_t *mrcp_message, const apt_dir_layout_t *dir_layout){
		mMessage = mrcp_message;
		mDir = dir_layout;
	}

	mrcp_message_t* GetNativeMessage(){
		return mMessage;
	}

	virtual void SetHearder(int type, String^ content);
	virtual void SetBody(String^ content);
	virtual String^ GetHeader(int type);
	virtual String^ GetBody();
	virtual Dictionary<Int32, Object^>^ GetFirstLine();
	~MrcpMessage(){
		//message is allocated in channel pool, no need to free
	}
};

ref class MrcpChannel : ucf::IMrcpChannel{
	mrcp_application_t *mApplication;
	mrcp_session_t *mSession;
	mrcp_channel_t *mChannel;
	apt_dir_layout_t *mDir;	

private:
	static ConcurrentDictionary<IntPtr, ucf::IMrcpChannel^>^ CHANNELS = gcnew ConcurrentDictionary<IntPtr, ucf::IMrcpChannel^>();

	MrcpChannel(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, apt_dir_layout_t *dir_layout){
		mApplication = application;
		mSession = session;
		mChannel = channel;
		mDir = mDir;
	}	
public:
	static ucf::IMrcpChannelMgr^ MGR;
	static int ChannelCount(){
		#pragma warning (disable: 4538)
		return MrcpChannel::CHANNELS->Count;
	};
	static ICollection<ucf::IMrcpChannel^>^ GetChannels(){
		return MrcpChannel::CHANNELS->Values;
	}
	static ucf::IMrcpChannel^ GetChannel(mrcp_channel_t *channel);
	static void AddNewChannel(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, apt_dir_layout_t *dir_layout);
	static MrcpChannel^ RemoveChannel(mrcp_channel_t *channel);
	virtual ucf::IMrcpMessage^ CreateMessage(int type);
	virtual void SendMessage(ucf::IMrcpMessage^ msg);
	virtual String^ GetChannelId();
	~MrcpChannel(){
		//message is allocated in channel pool, no need to free
	}
	virtual ucf::IMrcpChannelMgr^ GetMgr(){return MGR;}
};

ref class MrcpChannelMgr : ucf::IMrcpChannelMgr{
private:
	MrcpChannelMgr(){}
	static MrcpChannelMgr^ THIS = nullptr;
public:
	static MrcpChannelMgr^ getInstance(){
		if(THIS == nullptr){
			THIS = gcnew MrcpChannelMgr();
			MrcpChannel::MGR = THIS;
		}
		return THIS;
	}
	virtual int Count(){
		return MrcpChannel::ChannelCount();
	}
	virtual property ICollection<ucf::IMrcpChannel^>^ Channels{
		ICollection<ucf::IMrcpChannel^>^ get(){			
			return MrcpChannel::GetChannels();
		}
	}
};

#include <msclr/marshal.h>

ref class CmdRunner : PluginsMgr::ICmdRunner{
private:
	void* obj;
	msclr::interop::marshal_context mContext;
public:
	CmdRunner(void* framework){
		obj= framework;
	}
	virtual bool Run(ucf::ITestCase^ tc);

	static ucf::ITestApp^ APP;
};

#endif