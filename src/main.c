/*
 * Copyright 2008-2010 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * $Id: main.c 1924 2013-03-25 17:03:04Z achaloyan@gmail.com $
 */

#include <stdio.h>
#include <stdlib.h>
#include <apr_getopt.h>
#include <apr_file_info.h>
#include "demo_framework.h"
#include "apt_pool.h"
#include "apt_log.h"
#include "uni_version.h"
#include "AppMessage.h"

typedef struct {
	const char   *root_dir_path;
	const char   *log_priority;
	const char   *log_output;	
} client_options_t;

static System::IntPtr CLRS(void* s){
	return System::IntPtr((void*)s);
}

/*static recogAppRunner::Main^ getCLR(System::IntPtr clr){
	return (recogAppRunner::Main^)(((System::Runtime::InteropServices::GCHandle) clr).Target);
}
*/

static void* fromCLR(System::Object^ obj){
	return reinterpret_cast<void*>(
		((System::IntPtr)System::Runtime::InteropServices::GCHandle::Alloc(obj)
		).ToPointer());
}

static apt_bool_t demo_framework_cmdline_process(demo_framework_t *framework, char *cmdline, ucf::ITestCase^ session)
{
	apt_bool_t running = TRUE;
	char *name;
	char *last;
	name = apr_strtok(cmdline, " ", &last);

	if(strcasecmp(name,"run") == 0) {
		char *app_name = apr_strtok(NULL, " ", &last);
		if(app_name) {
			char *profile_name = apr_strtok(NULL, " ", &last);
			if(!profile_name) {
				profile_name = "uni2";
			}
			demo_framework_app_run(framework,app_name,profile_name, fromCLR(session));
		}
	}
	else if(strcasecmp(name,"loglevel") == 0) {
		char *priority = apr_strtok(NULL, " ", &last);
		if(priority) {
			apt_log_priority_set((apt_log_priority_e)atol(priority));
		}
	}
	else if(strcasecmp(name,"exit") == 0 || strcmp(name,"quit") == 0) {
		running = FALSE;
	}
	else if(strcasecmp(name,"help") == 0) {
		printf("usage:\n"
		       "\n- run [app_name] [profile_name] (run demo application)\n"
			   "       app_name is one of 'synth', 'recog', 'bypass', 'discover'\n"
			   "       profile_name is one of 'uni2', 'uni1', ...\n"
			   "\n       examples: \n"
			   "           run synth\n"
			   "           run recog\n"
			   "           run synth uni1\n"
			   "           run recog uni1\n"
		       "\n- loglevel [level] (set loglevel, one of 0,1...7)\n"
		       "\n- quit, exit\n");
	}
	else {
		printf("unknown command: %s (input help for usage)\n",name);
	}
	return running;
}

bool CmdRunner::Run(ucf::ITestCase^ tc){
		const char* cmdline = mContext.marshal_as<const char*>(tc->Cmd);
		return demo_framework_cmdline_process((demo_framework_t*)obj, (char*)cmdline, tc) == TRUE;
	}


static apt_bool_t demo_framework_cmdline_run(demo_framework_t *framework)
{
#define MAX_CMD_LEN 1024
	apt_bool_t running = TRUE;

	CmdRunner^ runner = gcnew CmdRunner(framework);

	while(running){
		running = PluginsMgr::MainRunner::Run(runner);
	}	
	return TRUE;
}

static void usage()
{
	printf(
		"\n"
		" * "UNI_COPYRIGHT"\n"
		" *\n"
		UNI_LICENSE"\n"
		"\n"
		"Usage:\n"
		"\n"
		"  unimrcpclient [options]\n"
		"\n"
		"  Available options:\n"
		"\n"
		"   -r [--root-dir] path     : Set the project root directory path.\n"
		"\n"
		"   -l [--log-prio] priority : Set the log priority.\n"
		"                              (0-emergency, ..., 7-debug)\n"
		"\n"
		"   -o [--log-output] mode   : Set the log output mode.\n"
		"                              (0-none, 1-console only, 2-file only, 3-both)\n"
		"\n"
		"   -v [--version]           : Show the version.\n"
		"\n"
		"   -h [--help]              : Show the help.\n"
		"\n"
		"   -a [--auto]configuration : Run client with configuration file automatically.\n"
		"\n");
}

static apt_bool_t demo_framework_options_load(client_options_t *options, int argc, const char * const *argv, apr_pool_t *pool)
{
	apr_status_t rv;
	apr_getopt_t *opt = NULL;
	int optch;
	const char *optarg;

	const apr_getopt_option_t opt_option[] = {
		/* long-option, short-option, has-arg flag, description */
		{ "root-dir",    'r', TRUE,  "path to root dir" },  /* -r arg or --root-dir arg */
		{ "log-prio",    'l', TRUE,  "log priority" },      /* -l arg or --log-prio arg */
		{ "log-output",  'o', TRUE,  "log output mode" },   /* -o arg or --log-output arg */
		{ "version",     'v', FALSE, "show version" },      /* -v or --version */
		{ "help",        'h', FALSE, "show help" },         /* -h or --help */
		{ "auto",        'a', TRUE, "automatically run"},
		{ NULL, 0, 0, NULL },                               /* end */
	};

	rv = apr_getopt_init(&opt, pool , argc, argv);
	if(rv != APR_SUCCESS) {
		return FALSE;
	}

	while((rv = apr_getopt_long(opt, opt_option, &optch, &optarg)) == APR_SUCCESS) {
		switch(optch) {
			case 'r':
				options->root_dir_path = optarg;
				break;
			case 'l':
				options->log_priority = optarg;
				break;
			case 'o':
				options->log_output = optarg;
				break;
			case 'v':
				printf(UNI_VERSION_STRING);
				return FALSE;
			case 'h':
				usage();
				return FALSE;
			case 'a':
				CmdRunner::APP = PluginsMgr::MainRunner::Init(MrcpChannelMgr::getInstance(), CLRS((void*)optarg));					
				break;
		}
	}

	if(rv != APR_EOF) {
		usage();
		return FALSE;
	}

	return TRUE;
}

int main(int argc, const char * const *argv)
{
	apr_pool_t *pool = NULL;
	client_options_t options;
	apt_dir_layout_t *dir_layout;
	const char *log_conf_path;
	demo_framework_t *framework;

	/* APR global initialization */
	if(apr_initialize() != APR_SUCCESS) {
		apr_terminate();
		return 0;
	}

	/* create APR pool */
	pool = apt_pool_create();
	if(!pool) {
		apr_terminate();
		return 0;
	}

	/* set the default options */
	options.root_dir_path = "../";
	options.log_priority = NULL;
	options.log_output = NULL;

	/* load options */
	if(demo_framework_options_load(&options,argc,argv,pool) != TRUE) {
		apr_pool_destroy(pool);
		apr_terminate();
		return 0;
	}

	/* create the structure of default directories layout */
	dir_layout = apt_default_dir_layout_create(options.root_dir_path,pool);

	/* get path to logger configuration file */
	log_conf_path = apt_confdir_filepath_get(dir_layout,"logger.xml",pool);
	/* create and load singleton logger */
	apt_log_instance_load(log_conf_path,pool);

	if(options.log_priority) {
		/* override the log priority, if specified in command line */
		apt_log_priority_set((apt_log_priority_e)atoi(options.log_priority));
	}
	if(options.log_output) {
		/* override the log output mode, if specified in command line */
		apt_log_output_mode_set((apt_log_output_e)atoi(options.log_output));
	}

	if(apt_log_output_mode_check(APT_LOG_OUTPUT_FILE) == TRUE) {
		/* open the log file */
		apt_log_file_open(dir_layout->log_dir_path,"unimrcpclient",MAX_LOG_FILE_SIZE,MAX_LOG_FILE_COUNT,FALSE,pool);
	}

	/* create demo framework */
	framework = demo_framework_create(dir_layout, NULL);
	if(framework) {
		/* run command line  */
		demo_framework_cmdline_run(framework);
		/* destroy demo framework */
		demo_framework_destroy(framework);
		if(CmdRunner::APP != nullptr){
			CmdRunner::APP->OnDestory();
		}
	}

	/* destroy singleton logger */
	apt_log_instance_destroy();
	/* destroy APR pool */
	apr_pool_destroy(pool);
	/* APR global termination */
	apr_terminate();
	return 0;
}
