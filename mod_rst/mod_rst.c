/*
auth:上海宁卫信息技术有限公司
License: GPL
Description: this is a module of FreeSWITCH，and it send a udp stream to other udp server
*/
#include "switch.h"

SWITCH_MODULE_LOAD_FUNCTION(mod_rst_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_rst_shutdown);
SWITCH_MODULE_DEFINITION(mod_rst, mod_rst_load, mod_rst_shutdown, NULL);

static struct {
	switch_memory_pool_t *pool;
	switch_mutex_t *mutex;
	char* udp_server_ip;
    unsigned short udp_server_port;
	unsigned int fs_ver;
   
} globals;


struct rst_helper {
	
	int native;
	uint32_t packet_len;
	int min_sec;
	int final_timeout_ms;
	int initial_timeout_ms;
	int silence_threshold;
	int silence_timeout_ms;
	switch_time_t silence_time;
	int rready;
	int wready;
	switch_time_t last_read_time;
	switch_time_t last_write_time;
	switch_bool_t hangup_on_error;
	switch_codec_implementation_t read_impl;
	switch_codec_implementation_t write_impl;
	switch_bool_t speech_detected;
	switch_buffer_t *thread_buffer;
	switch_thread_t *thread;
	switch_mutex_t *buffer_mutex;
	int thread_ready;
	const char *completion_cause;
	
	switch_audio_resampler_t *resampler;
    switch_socket_t *socket = NULL;
	switch_sockaddr_t *addr = NULL,
};


SWITCH_DECLARE(char *) switch_uuid_str(char *buf, switch_size_t len)
{
	switch_uuid_t uuid;

	if (len < (SWITCH_UUID_FORMATTED_LENGTH + 1)) {
		switch_snprintf(buf, len, "INVALID");
	} else {
		switch_uuid_get(&uuid);
		switch_uuid_format(buf, &uuid);
	}

	return buf;
}

static switch_bool_t nway_is_silence_frame(switch_frame_t *frame, int silence_threshold, switch_codec_implementation_t *codec_impl)
{
	int16_t *fdata = (int16_t *) frame->data;
	uint32_t samples = frame->datalen / sizeof(*fdata);
	switch_bool_t is_silence = SWITCH_TRUE;
	uint32_t channel_num = 0;

	int divisor = 0;
	if (!(divisor = codec_impl->samples_per_second / 8000)) {
		divisor = 1;
	}

	/* is silence only if every channel is silent */
	for (channel_num = 0; channel_num < codec_impl->number_of_channels && is_silence; channel_num++) {
		uint32_t count = 0, j = channel_num;
		double energy = 0;
		for (count = 0; count < samples; count++) {
			energy += abs(fdata[j]);
			j += codec_impl->number_of_channels;
		}
		is_silence &= (uint32_t) ((energy / (samples / divisor)) < silence_threshold);
	}

	return is_silence;
}




static void *SWITCH_THREAD_FUNC rts_thread(switch_thread_t *thread, void *obj)
{
	switch_media_bug_t *bug = (switch_media_bug_t *) obj;
	switch_core_session_t *session = switch_core_media_bug_get_session(bug);
	switch_channel_t *channel = switch_core_session_get_channel(session);
	struct rts_helper *rh;
	switch_size_t bsize = SWITCH_RECOMMENDED_BUFFER_SIZE, samples = 0, inuse = 0;
	unsigned char *data;
	int channels = 1;

	if (switch_core_session_read_lock(session) != SWITCH_STATUS_SUCCESS) {
		return NULL;
	}

	rh = switch_core_media_bug_get_user_data(bug);
	switch_buffer_create_dynamic(&rh->thread_buffer, 1024 * 512, 1024 * 64, 0);
	rh->thread_ready = 1;

	channels = switch_core_media_bug_test_flag(bug, SMBF_STEREO) ? 2 : rh->read_impl.number_of_channels;
	data = switch_core_session_alloc(session, bsize);

	while(switch_test_flag(rh->fh, SWITCH_FILE_OPEN)) {
		switch_mutex_lock(rh->buffer_mutex);
		inuse = switch_buffer_inuse(rh->thread_buffer);

		if (rh->thread_ready && switch_channel_up_nosig(channel) && inuse < bsize) {
			switch_mutex_unlock(rh->buffer_mutex);
			switch_yield(20000);
			continue;
		} else if ((!rh->thread_ready || switch_channel_down_nosig(channel)) && !inuse) {
			switch_mutex_unlock(rh->buffer_mutex);
			break;
		}
		
		samples = switch_buffer_read(rh->thread_buffer, data, bsize) / 2 / channels;
		switch_mutex_unlock(rh->buffer_mutex);

		if (switch_core_file_write(rh->fh, data, &samples) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Error writing %s\n", rh->file);
			/* File write failed */
			set_completion_cause(rh, "uri-failure");
			if (rh->hangup_on_error) {
				switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
				switch_core_session_reset(session, SWITCH_TRUE, SWITCH_TRUE);
			}
		}
	}

	switch_core_session_rwunlock(session);

	return NULL;
}
static switch_bool_t nway_send_to(struct rst_helper* rh,char *data,int len)
{
    if (socket){
        if (switch_socket_sendto(rh->socket, rh->remote_addr, 0, (void *) data, &len) != SWITCH_STATUS_SUCCESS) {
						switch_ivr_deactivate_unicast(session);
				}
    }
}
static switch_bool_t nway_rts_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type)
{
	switch_core_session_t *session = switch_core_media_bug_get_session(bug);
	switch_channel_t *channel = switch_core_session_get_channel(session);
	struct rst_helper *rh = (struct rst_helper *) user_data;
	switch_event_t *event;
	switch_frame_t *nframe;
	switch_size_t len = 0;
	 
	switch_codec_t* raw_codec = NULL;
	switch_codec_implementation_t read_impl = { 0 };
	int mask = switch_core_media_bug_test_flag(bug, SMBF_MASK);
	unsigned char null_data[SWITCH_RECOMMENDED_BUFFER_SIZE] = {0};

	int16_t *read_data;
	int read_samples;

	

	switch_core_session_get_read_impl(session, &read_impl);

	int channels = read_impl.number_of_channels;
	
    switch_rtp_engine_t *a_engine;
	switch_media_handle_t *smh;

	switch_assert(session);
	
	if (!(smh = session->media_handle)) {
		return SWITCH_STATUS_FALSE;
	}

	a_engine = &smh->engines[SWITCH_MEDIA_TYPE_AUDIO];
	 


	if (switch_channel_down(session->channel)) {
		return SWITCH_STATUS_FALSE;
	}

	
 
	 
	switch (type) {
	case SWITCH_ABC_TYPE_INIT:
		{
			//get caller,callee,uuid,caller_port,callee_port
            switch_caller_profile_t *caller_profile;
			caller_profile = switch_channel_get_caller_profile(channel);
             
            if (a_engine->rtp_session){
                 
                char cmd[1024]={0};
                sprintf(cmd,"INV:%s:%s:%s:%d:%d\0",caller_profile->caller_id_number,caller_profile->destination_number,switch_channel_get_uuid(channel),a_engine->local_sdp_port,a_engine->cur_payload_map->remote_sdp_port);
                nway_send_to(rh,cmd,strlen(cmd));
            }
            
		}
		break;
	case SWITCH_ABC_TYPE_TAP_NATIVE_READ:
		{
			 
			
		}
		break;
	case SWITCH_ABC_TYPE_TAP_NATIVE_WRITE:
		{
			 
			
		}
		break;
	case SWITCH_ABC_TYPE_CLOSE:
		{
		
            if (a_engine->rtp_session){
                 
                char cmd[1024]={0};
                sprintf(cmd,"BYE:%s\0",switch_channel_get_uuid(channel));
                nway_send_to(rh,cmd,strlen(cmd));
            }
		}
		break;
	case SWITCH_ABC_TYPE_READ_PING:
	
		break;
	case SWITCH_ABC_TYPE_WRITE_REPLACE:
		{
			wframe = switch_core_media_bug_get_write_replace_frame(bug);
            //send local rtp port and data
            char cmd[1024]={0};
            sprintf(cmd,"DATA:%d:0:\0",a_engine->local_sdp_port);
            int len=strlen(cmd);
            memcpy(cmd+len,wframe->data,wframe->datalen);
            len += wframe->datalen;
            nway_send_to(rh,cmd,len);
			switch_core_media_bug_set_write_replace_frame(bug, wframe);
		}
		break;
	case SWITCH_ABC_TYPE_READ_REPLACE:
		
			rframe = switch_core_media_bug_get_read_replace_frame(bug);
			//send remote rtp port and data
			char cmd[1024]={0};
            sprintf(cmd,"DATA:%d:0:\0",a_engine->cur_payload_map->remote_sdp_port);
            int len=strlen(cmd);
            memcpy(cmd+len,rframe->data,rframe->datalen);
            len += rframe->datalen;
            nway_send_to(rh,cmd,len);
			switch_core_media_bug_set_read_replace_frame(bug, rframe);
		   
	   }
		break;
	 
	default:
		break;
	}
 
	return SWITCH_TRUE;

}


SWITCH_DECLARE(switch_status_t) nway_rst_session(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "concurrent is:%d,18621575908 \n",switch_core_session_count());
	switch_channel_t *channel = switch_core_session_get_channel(session);
	const char *p;
	const char *vval;
	switch_media_bug_t *bug;
	switch_status_t status;
	time_t to = 0;
	switch_media_bug_flag_t flags = SMBF_WRITE_REPLACE | SMBF_READ_REPLACE|SMBF_READ_PING|SMBF_READ_STREAM | SMBF_WRITE_STREAM;
	//SMBF_READ_STREAM | SMBF_WRITE_STREAM | SMBF_READ_PING;// SMBF_TAP_NATIVE_READ | SMBF_TAP_NATIVE_WRITE  ;
	// SMBF_READ_STREAM | SMBF_WRITE_STREAM |SMBF_STEREO| SMBF_READ_PING |SMBF_STEREO;SMBF_READ_STREAM | SMBF_WRITE_STREAM | SMBF_READ_PING ;//
	uint8_t channels;
	switch_codec_implementation_t read_impl = { 0 };
	struct record_helper *rh = NULL;
	int file_flags = SWITCH_FILE_FLAG_WRITE | SWITCH_FILE_DATA_SHORT;
	switch_bool_t hangup_on_error = SWITCH_FALSE;	
	switch_codec_t raw_codec = { 0 };
	if (!switch_channel_media_up(channel) || !switch_core_session_get_read_codec(session)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Can not rst session.  Media not enabled on channel\n");
		return SWITCH_STATUS_FALSE;
	}

	switch_core_session_get_read_impl(session, &read_impl);
	channels = read_impl.number_of_channels;	
	rh = switch_core_session_alloc(session, sizeof(*rh));
	if (read_impl.actual_samples_per_second != 8000) {

		if (switch_resample_create(&rh->resampler,
			read_impl.actual_samples_per_second,
			8000,
			read_impl.samples_per_packet, SWITCH_RESAMPLE_QUALITY, 1) != SWITCH_STATUS_SUCCESS) {

				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unable to create resampler!\n");

				goto end;

		}
	}
    if (switch_socket_create(&rh->socket, AF_INET, SOCK_DGRAM, 0, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Socket Error 1\n");
		goto end;
	}

	if (switch_socket_opt_set(rh->socket, SWITCH_SO_REUSEADDR, 1) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Option Error\n");
		goto end;
	}
    if (switch_sockaddr_info_get(&rh->addr, globals.udp_server_ip, SWITCH_UNSPEC,
								 globals.udp_server_port, 0, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Socket Error 3\n");
		goto end;
	}
	if ((status = switch_core_media_bug_add(session, "rst", file,
											nway_rts_callback, rh, to, flags, &bug)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Error adding media bug  \n");
		 
		return status;
	} 
end:
   	if (rh->resampler) {
		switch_resample_destroy(&rh->resampler);
	}
	if (rh->socket)
        switch_socket_close(rh->socket);
    switch_core_session_reset(session, SWITCH_FALSE, SWITCH_TRUE);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t load_config(void)
{
	char *cf = "rst.conf";
	switch_xml_t cfg, xml = NULL, param, settings;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
		status = SWITCH_STATUS_FALSE;
		goto done;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");
			if (!strcasecmp(var, "udp-server-ip")) {
				if (!zstr(val) ) {
					globals.udp_server_ip = switch_core_strdup(globals.pool, val);
				}
			}
			if (!strcasecmp(var, "udp-server-port")) {
				if (!zstr(val) ) {
					globals.udp_server_port = atoi(val);
				}
			}
		}
	}
  done:
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, globals.recording_dir);
	if (xml) {
		switch_xml_free(xml);
	}

	return status;
}
SWITCH_STANDARD_APP(stop_rst_session_function)
{
	switch_ivr_stop_record_session(session, data);
}
SWITCH_DECLARE(switch_status_t) switch_ivr_stop_rts_session(switch_core_session_t *session, const char *file)
{
	switch_media_bug_t *bug;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	return switch_core_media_bug_remove_callback(session, nway_rst_callback);
	 
}

SWITCH_STANDARD_APP(rst_session_function)
{
	char *path = NULL;
	char *path_end;
	uint32_t limit = 0;

	if (zstr(data)) {
		return;
	}
	nway_rst_session(session);
}

#define SESSIOS_RST_SYNTAX "<uuid>"
SWITCH_STANDARD_API(session_rst_function)
{
	switch_core_session_t *rsession = NULL;
	 
	char *uuid = NULL;
	int argc = 0;
	uint32_t limit = 0;

	if (zstr(cmd)) {
		goto usage;
	}

	uuid = cmd;
	if (!(rsession = switch_core_session_locate(uuid))) {
		stream->write_function(stream, "-ERR Cannot locate session!\n");
		goto done;
	}

	goto done;

  usage:
	stream->write_function(stream, "-USAGE: %s\n", SESSIOS_RST_SYNTAX);

  done:
	if (rsession) {
		switch_core_session_rwunlock(rsession);
	}
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_rst_load)
{
	switch_application_interface_t *app_interface;
	//globals.pool = pool;
	memset(&globals, 0, sizeof(globals));
	globals.pool = pool;
	//switch_api_interface_t *commands_api_interface;
	switch_api_interface_t *api_interface;
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, globals.pool);
	unsigned int major = atoi(switch_version_major());
	unsigned int minor = atoi(switch_version_minor());
	unsigned int micro = atoi(switch_version_micro());

	globals.fs_ver = major << 16;
	globals.fs_ver |= minor << 8;
	globals.fs_ver |= micro << 4;
	load_config();
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
		
	SWITCH_ADD_APP(app_interface, "rst", "realtime stream transfer", "rst", rst_session_function, "", SAF_MEDIA_TAP);
	SWITCH_ADD_APP(app_interface, "rst_stop", "realtime stream transfer", "rst_stop", switch_ivr_stop_rts_session, "", SAF_MEDIA_TAP);
	SWITCH_ADD_API(api_interface, "uuid_rst", "realtime stream transfer api", session_rst_function, SESSIOS_RST_SYNTAX);
 
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " rst loaded\n");
	return SWITCH_STATUS_SUCCESS;
END:
    return SWITCH_STATUS_FALSE;
   
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_rst_shutdown)
{
	 
	switch_mutex_destroy(globals.mutex);	 
	return SWITCH_STATUS_SUCCESS;
}