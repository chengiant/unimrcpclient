#include "mrcp_application.h"
#include "mrcp_message.h"
#include "mrcp_client_session.h"
#include "AppMessage.h"


using namespace System::Runtime::InteropServices;
using ucf::MrcpConst;

static String^ getCLRS2(apt_str_t* pAptStr){
	if(pAptStr->length <= 0){
		return nullptr;
	}
	array<Byte>^ bytes = gcnew array<Byte>(pAptStr->length);
	Marshal::Copy(IntPtr((void*)pAptStr->buf), bytes, 0, pAptStr->length);
	return System::Text::Encoding::UTF8->GetString(bytes);
}

ucf::IMrcpChannel^ MrcpChannel::GetChannel(mrcp_channel_t *channel){
	IntPtr key((void*)channel);
	return MrcpChannel::CHANNELS[key];
}

String^ MrcpChannel::GetChannelId(){
	return getCLRS2(&mChannel->control_channel->identifier);
}
void MrcpChannel::AddNewChannel(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, apt_dir_layout_t *dir_layout){
	MrcpChannel^ c = gcnew MrcpChannel(application, session, channel, dir_layout);
	IntPtr key((void*)channel);
	MrcpChannel::CHANNELS[key] = c;
}
MrcpChannel^ MrcpChannel::RemoveChannel(mrcp_channel_t *channel){
	IntPtr key((void*)channel);
	MrcpChannel^ ret;
	MrcpChannel::CHANNELS->TryRemove(key, ret);
	return ret;
}
void MrcpChannel::SendMessage(ucf::IMrcpMessage^ msg){
	MrcpMessage^ message = (MrcpMessage^)msg;
	mrcp_application_message_send(mSession, mChannel, message->GetNativeMessage());
}
ucf::IMrcpMessage^ MrcpChannel::CreateMessage(int type){
	int nativeType = -1;
	switch(type){
	case (int)MrcpConst::RECOGNIZER_RECOGNIZE:
		nativeType = RECOGNIZER_RECOGNIZE;
		break;
	}
	mrcp_message_t *mrcp_message = mrcp_application_message_create(mSession, mChannel, nativeType);
	MrcpMessage^ msg = gcnew MrcpMessage(mrcp_message, mDir);
	return msg;
}

void MrcpMessage::SetHearder(int type, String^ content){
	mrcp_generic_header_t *generic_header = mrcp_generic_header_prepare(mMessage);
	mrcp_recog_header_t *recog_header;
	
	array<Byte>^ encodedBytes = System::Text::Encoding::UTF8->GetBytes(content);
	pin_ptr<Byte> pinnedBytes = &encodedBytes[0];
	char* pUtf8 = reinterpret_cast<char*>(pinnedBytes);

	switch(type){
	case MrcpConst::GENERIC_HEADER_CONTENT_TYPE:
		apt_string_assign(&generic_header->content_type, pUtf8, mMessage->pool);
		mrcp_generic_header_property_add(mMessage,GENERIC_HEADER_CONTENT_TYPE);
		break;
	case MrcpConst::GENERIC_HEADER_CONTENT_ID:
		apt_string_assign(&generic_header->content_id, pUtf8, mMessage->pool);
		mrcp_generic_header_property_add(mMessage,GENERIC_HEADER_CONTENT_ID);
		break;
	case MrcpConst::RECOGNIZER_HEADER_CANCEL_IF_QUEUE:
		recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(mMessage);
		recog_header->cancel_if_queue = Boolean::Parse(content) ? TRUE : FALSE;
		mrcp_resource_header_property_add(mMessage,RECOGNIZER_HEADER_CANCEL_IF_QUEUE);
		break;
	case MrcpConst::RECOGNIZER_HEADER_NO_INPUT_TIMEOUT:
		recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(mMessage);
		recog_header->no_input_timeout = Int32::Parse(content);
		mrcp_resource_header_property_add(mMessage,RECOGNIZER_HEADER_NO_INPUT_TIMEOUT);
		break;
	case MrcpConst::RECOGNIZER_HEADER_RECOGNITION_TIMEOUT:
		recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(mMessage);
		recog_header->recognition_timeout = Int32::Parse(content);
		mrcp_resource_header_property_add(mMessage,RECOGNIZER_HEADER_RECOGNITION_TIMEOUT);
		break;
	case MrcpConst::RECOGNIZER_HEADER_START_INPUT_TIMERS:
		recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(mMessage);
		recog_header->start_input_timers = Boolean::Parse(content);
		mrcp_resource_header_property_add(mMessage,RECOGNIZER_HEADER_START_INPUT_TIMERS);
		break;
	case MrcpConst::RECOGNIZER_HEADER_CONFIDENCE_THRESHOLD:
		recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(mMessage);
		recog_header->confidence_threshold = Single::Parse(content);
		mrcp_resource_header_property_add(mMessage,RECOGNIZER_HEADER_CONFIDENCE_THRESHOLD);
		break;	
	}
}
void MrcpMessage::SetBody(String^ content){	
	array<Byte>^ encodedBytes = System::Text::Encoding::UTF8->GetBytes(content);
	pin_ptr<Byte> pinnedBytes = &encodedBytes[0];
	char* pUtf8 = reinterpret_cast<char*>(pinnedBytes);
	
	apt_string_assign_n(&mMessage->body, pUtf8, encodedBytes->Length, mMessage->pool);
}
String^ MrcpMessage::GetHeader(int type){

	mrcp_generic_header_t *generic_header = mrcp_generic_header_prepare(mMessage);

	switch(type){
	case MrcpConst::GENERIC_HEADER_CONTENT_TYPE:
		return getCLRS2(&generic_header->content_type);
		
	case MrcpConst::GENERIC_HEADER_CONTENT_ID:
		return getCLRS2(&generic_header->content_id);		
	case MrcpConst::GENERIC_HEADER_CHANNEL_ID:
		return getCLRS2(&mMessage->channel_id.session_id) + "@" + getCLRS2(&mMessage->channel_id.resource_name);
	}	
	
	return nullptr;
}
Dictionary<Int32, Object^>^ MrcpMessage::GetFirstLine(){

	Dictionary<Int32, Object^>^ ret = gcnew Dictionary<Int32, Object^>();
	ret[(int)MrcpConst::FIRST_LINE_length] = UInt32(mMessage->start_line.length);
	ret[(int)MrcpConst::FIRST_LINE_version] = UInt32(mMessage->start_line.version);
	ret[(int)MrcpConst::FIRST_LINE_type] = UInt32(mMessage->start_line.message_type);
	ret[(int)MrcpConst::FIRST_LINE_request_id] = UInt32(mMessage->start_line.request_id);
	ret[(int)MrcpConst::FIRST_LINE_method_name] = getCLRS2(&mMessage->start_line.method_name);
	ret[(int)MrcpConst::FIRST_LINE_method_id] = Int32(mMessage->start_line.method_id);	
	ret[(int)MrcpConst::FIRST_LINE_status_code] = UInt32(mMessage->start_line.status_code);
	ret[(int)MrcpConst::FIRST_LINE_request_state] = UInt32(mMessage->start_line.request_state);
	return ret;
}

String^ MrcpMessage::GetBody(){  
	return getCLRS2(&mMessage->body);
}

