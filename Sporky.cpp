#include "Sporky.h"
#include <iostream>
#include "glib.h"
#include "purple.h"
#include "dlfcn.h"

#define READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

GMainLoop *loop;
static jobject mainObj;
static JNIEnv *mainEnv;

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

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node) || !mainEnv)
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	jclass cls = mainEnv->GetObjectClass(mainObj);
	jmethodID mid = mainEnv->GetMethodID(cls, "onBuddyCreated", "(Ljava/lang/String;)V");
	if (mid == 0)
		return;
	mainEnv->CallVoidMethod(mainObj, mid, mainEnv->NewStringUTF(purple_buddy_get_name(buddy)));
}

static PurpleBlistUiOps blistUiOps =
{
	NULL,
	buddyListNewNode,
	NULL,
	NULL, // buddyListUpdate,
	NULL, //NodeRemoved,
	NULL,
	NULL,
	NULL, // buddyListAddBuddy,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
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

static void signed_on(PurpleConnection *gc,gpointer unused) {
  jclass cls = mainEnv->GetObjectClass(mainObj);
  jmethodID mid = mainEnv->GetMethodID(cls, "onConnected", "(Ljava/lang/String;)V");
  if (mid == 0)
    return;
  mainEnv->CallVoidMethod(mainObj, mid, mainEnv->NewStringUTF(purple_account_get_username(purple_connection_get_account(gc))));
}

JNIEXPORT jint JNICALL Java_Sporky_init
  (JNIEnv *, jobject, jstring) {
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
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
	}
	return ret;
}

JNIEXPORT void JNICALL Java_Sporky_connect (JNIEnv *env, jobject, jstring _name, jobject, jstring _password) {
	const char *name = env->GetStringUTFChars(_name, 0);
	const char *password = env->GetStringUTFChars(_password, 0);

	PurpleAccount *account = purple_account_new(name, "prpl-jabber");
	purple_account_set_password(account, password);
	purple_account_set_enabled(account, "sporky", TRUE);
	purple_accounts_add(account);

	env->ReleaseStringUTFChars(_name, name);
	env->ReleaseStringUTFChars(_password, password);
}

JNIEXPORT jint JNICALL Java_Sporky_start (JNIEnv *env, jobject obj) {
	mainObj = env->NewGlobalRef(obj);
	mainEnv = env;
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	
	env->DeleteGlobalRef(mainObj);
	return 0;
}

static gboolean stop_libpurple(void *data) {
	purple_blist_uninit();
	purple_core_quit();
	
	if (loop) {
		g_main_loop_quit(loop);
		g_main_loop_unref(loop);
	}
	return FALSE;
}

JNIEXPORT void JNICALL Java_Sporky_stop (JNIEnv *env, jobject) {
	purple_timeout_add_seconds(0, &stop_libpurple, NULL);
}

JNIEXPORT void JNICALL Java_Sporky_sendMessage (JNIEnv *env, jobject, jstring _name, jstring _to, jstring _message) {
	const char *name = env->GetStringUTFChars(_name, 0);
	const char *to = env->GetStringUTFChars(_to, 0);
	const char *message = env->GetStringUTFChars(_message, 0);
	PurpleAccount *account = purple_accounts_find(name, "prpl-jabber");
	
	if (account) {
		PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
										to,
										account);
		if (!conv)
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, to);

		gchar *_markup = purple_markup_escape_text(message, -1);
		purple_conv_im_send(PURPLE_CONV_IM(conv), _markup);
	}
	
	env->ReleaseStringUTFChars(_name, name);
	env->ReleaseStringUTFChars(_to, to);
	env->ReleaseStringUTFChars(_message, message);
}
  