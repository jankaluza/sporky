#include "Sporky.h"
#include <iostream>
#include "glib.h"
#include "purple.h"
#include "dlfcn.h"

#define READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _PurpleIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleIOClosure;

static gboolean io_invoke(GIOChannel *source,
										GIOCondition condition,
										gpointer data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;

	int tmp = 0;
	if (condition & READ_COND)
	{
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (condition & WRITE_COND)
	{
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}

	closure->function(closure->data, g_io_channel_unix_get_fd(source), purple_cond);

	return TRUE;
}

static void io_destroy(gpointer data)
{
	g_free(data);
}

static guint input_add(gint fd,
								PurpleInputCondition condition,
								PurpleInputFunction function,
								gpointer data)
{
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;
	closure->function = function;
	closure->data = data;

	int tmp = 0;
	if (condition & PURPLE_INPUT_READ)
	{
		tmp |= READ_COND;
		cond = (GIOCondition)tmp;
	}
	if (condition & PURPLE_INPUT_WRITE)
	{
		tmp |= WRITE_COND;
		cond = (GIOCondition)tmp;
	}

#ifdef WIN32
	channel = wpurple_g_io_channel_win32_new_socket(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
	io_invoke, closure, io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}

static PurpleEventLoopUiOps eventLoopOps =
{
	g_timeout_add,
	g_source_remove,
	input_add,
	g_source_remove,
	NULL,
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif

	NULL,
	NULL,
	NULL
};

static PurpleEventLoopUiOps * getEventLoopUiOps(void){
	return &eventLoopOps;
}


/***** Core Ui Ops *****/
static void
spectrum_glib_log_handler(const gchar *domain,
                          GLogLevelFlags flags,
                          const gchar *message,
                          gpointer user_data)
{
	const char *level;
	char *new_msg = NULL;
	char *new_domain = NULL;

	if ((flags & G_LOG_LEVEL_ERROR) == G_LOG_LEVEL_ERROR)
		level = "ERROR";
	else if ((flags & G_LOG_LEVEL_CRITICAL) == G_LOG_LEVEL_CRITICAL)
		level = "CRITICAL";
	else if ((flags & G_LOG_LEVEL_WARNING) == G_LOG_LEVEL_WARNING)
		level = "WARNING";
	else if ((flags & G_LOG_LEVEL_MESSAGE) == G_LOG_LEVEL_MESSAGE)
		level = "MESSAGE";
	else if ((flags & G_LOG_LEVEL_INFO) == G_LOG_LEVEL_INFO)
		level = "INFO";
	else if ((flags & G_LOG_LEVEL_DEBUG) == G_LOG_LEVEL_DEBUG)
		level = "DEBUG";
	else {
		std::cout << "glib" << "Unknown glib logging level in " << (guint)flags;
		level = "UNKNOWN"; /* This will never happen. */
	}

	if (message != NULL)
		new_msg = purple_utf8_try_convert(message);

	if (domain != NULL)
		new_domain = purple_utf8_try_convert(domain);

	if (new_msg != NULL) {
		std::string area("glib");
		area.push_back('/');
		area.append(level);

		std::string message(new_domain ? new_domain : "g_log");
		message.push_back(' ');
		message.append(new_msg);

		std::cout << area << " | " << message;
		g_free(new_msg);
	}

	g_free(new_domain);
}

static void
debug_init(void)
{
#define REGISTER_G_LOG_HANDLER(name) \
	g_log_set_handler((name), \
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL \
		                                  | G_LOG_FLAG_RECURSION), \
	                  spectrum_glib_log_handler, NULL)

	REGISTER_G_LOG_HANDLER(NULL);
	REGISTER_G_LOG_HANDLER("GLib");
	REGISTER_G_LOG_HANDLER("GModule");
	REGISTER_G_LOG_HANDLER("GLib-GObject");
	REGISTER_G_LOG_HANDLER("GThread");

#undef REGISTER_G_LOD_HANDLER
}

static void printDebug(PurpleDebugLevel level, const char *category, const char *arg_s) {
	std::string c("libpurple");

	if (category) {
		c.push_back('/');
		c.append(category);
	}

	std::cout << c << " | " << arg_s;
}

/*
 * Ops....
 */
static PurpleDebugUiOps debugUiOps =
{
	printDebug,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void transport_core_ui_init(void)
{
// 	purple_blist_set_ui_ops(&blistUiOps);
// 	purple_accounts_set_ui_ops(&accountUiOps);
// 	purple_notify_set_ui_ops(&notifyUiOps);
// 	purple_request_set_ui_ops(&requestUiOps);
// 	purple_xfers_set_ui_ops(getXferUiOps());
// 	purple_connections_set_ui_ops(&conn_ui_ops);
// 	purple_conversations_set_ui_ops(&conversation_ui_ops);
// #ifndef WIN32
// 	purple_dnsquery_set_ui_ops(getDNSUiOps());
// #endif
}

static PurpleCoreUiOps coreUiOps =
{
	NULL,
	debug_init,
	transport_core_ui_init,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int initPurple() {
	dlopen("libpurple.so",RTLD_NOW|RTLD_GLOBAL);

	purple_util_set_user_dir("/tmp");

	purple_debug_set_ui_ops(&debugUiOps);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	int ret = purple_core_init("sporky");
	if (ret) {
		static int conversation_handle;
		static int conn_handle;
		static int blist_handle;

		purple_set_blist(purple_blist_new());
		purple_blist_load();

		purple_prefs_load();
	}
	return ret;
}

static bool initialized;

// JNIEXPORT void JNICALL 
// Java_Callbacks_nativeMethod(JNIEnv *env, jobject obj, jint depth)
// {
//   jclass cls = (*env)->GetObjectClass(env, obj);
//   jmethodID mid = (*env)->GetMethodID(env, cls, "callback", "(I)V");
//   if (mid == 0)
//     return;
//   printf("In C, depth = %d, about to enter Java\n", depth);
//   (*env)->CallVoidMethod(env, obj, mid, depth);
//   printf("In C, depth = %d, back from Java\n", depth);
// }

JNIEXPORT jobjectArray JNICALL Java_Sporky_getContacts (JNIEnv *env, jclass, jstring _name, jobject, jstring _password) {
	int i;

	if (!initialized) {
		initialized = initPurple();
	}

	if (!initialized) {
		// TODO: exception
	}

	const char *name = env->GetStringUTFChars(_name, 0);
	const char *password = env->GetStringUTFChars(_password, 0);

	PurpleAccount *account = purple_account_new(name, "prpl-msn");
	purple_account_set_password(account, password);
	purple_account_set_enabled(account, "sporky", TRUE);
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	jobjectArray ret = (jobjectArray) env->NewObjectArray(5, env->FindClass("java/lang/String"), env->NewStringUTF(""));

// 	for (i = 0; i < 5; i++) {
// 		env->SetObjectArrayElement(ret, i, env->NewStringUTF(message[i]));
// 	}

	env->ReleaseStringUTFChars(_name, name);
	env->ReleaseStringUTFChars(_password, password);
	return ret;

}

JNIEXPORT void JNICALL Java_Sporky_deliveryChatMessage (JNIEnv *, jclass, jstring, jobject, jstring, jobjectArray) {
}
