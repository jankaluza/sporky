#include "Sporky.h"
#include "Session.h"
#include <iostream>
#include "glib.h"
#include "purple.h"
#include "dlfcn.h"
#include "string.h"

#define READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

static GMainLoop *loop;
static jobject mainObj;
static JNIEnv *mainEnv;

enum {
	TYPE_JABBER,
	TYPE_ICQ,
	TYPE_MSN,
	TYPE_AIM,
};

typedef struct _PurpleIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleIOClosure;

static gboolean io_invoke(GIOChannel *source, GIOCondition condition, gpointer data) {
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;

	int tmp = 0;
	if (condition & READ_COND) {
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}

	if (condition & WRITE_COND) {
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}

	closure->function(closure->data, g_io_channel_unix_get_fd(source), purple_cond);

	return TRUE;
}

static void io_destroy(gpointer data) {
	g_free(data);
}

static guint input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function, gpointer data) {
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;
	closure->function = function;
	closure->data = data;

	int tmp = 0;
	if (condition & PURPLE_INPUT_READ) {
		tmp |= READ_COND;
		cond = (GIOCondition)tmp;
	}
	if (condition & PURPLE_INPUT_WRITE) {
		tmp |= WRITE_COND;
		cond = (GIOCondition)tmp;
	}

	channel = g_io_channel_unix_new(fd);
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond, io_invoke, closure, io_destroy);
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

static PurpleEventLoopUiOps * getEventLoopUiOps() {
	return &eventLoopOps;
}


static void
spectrum_glib_log_handler(const gchar *domain, GLogLevelFlags flags, const gchar *message, gpointer user_data) {
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

static int callJavaMethod(jobject object, const char *method, const char *signature, ...) {
	if (!mainEnv)
		return -1;
	jclass cls = mainEnv->GetObjectClass(object);
	jmethodID mid = mainEnv->GetMethodID(cls, method, signature);
	if (mid == 0)
		return -1;

	va_list va;
	va_start(va, signature);
	int ret = 1;
	if (signature[strlen(signature) - 1] == 'V')
		mainEnv->CallVoidMethodV(object, mid, va);
	else
		ret = (int) mainEnv->CallIntMethodV(object, mid, va);
	va_end(va);

	return ret;
}

static jobject buddy_new(PurpleBuddy *buddy) {
	jfieldID fid;
	jclass cls = mainEnv->FindClass("Buddy");
	jmethodID mid = mainEnv->GetMethodID (cls, "<init>", "()V");
	jobject object = mainEnv->NewObject(cls, mid);

	jstring alias;
	if (purple_buddy_get_server_alias(buddy))
		alias = mainEnv->NewStringUTF(purple_buddy_get_server_alias(buddy));
	else
		alias = mainEnv->NewStringUTF(purple_buddy_get_alias(buddy));
	fid = mainEnv->GetFieldID(cls, "alias", "Ljava/lang/String;");
	mainEnv->SetObjectField(object, fid, alias);

	jstring name = mainEnv->NewStringUTF(purple_buddy_get_name(buddy));
	fid = mainEnv->GetFieldID(cls, "name", "Ljava/lang/String;");
	mainEnv->SetObjectField(object, fid, name);

	fid = mainEnv->GetFieldID(cls, "handle", "J");
	mainEnv->SetLongField(object, fid, (jlong) buddy); 

// 	buddy->node.ui_data = mainEnv->NewGlobalRef(object);

	return object;
}

struct BuddiesCount {
	PurpleAccount *account;
	int last_count;
};

static gboolean check_buddies_count(void *data) {
	BuddiesCount *count = (BuddiesCount *) data;
	int current_count = purple_account_get_int(count->account, "buddies_count", 0);
	std::cout << "CHECK BUDDIES COUNT " << current_count << " " << count->last_count << "\n";
	if (current_count == count->last_count) {
		GSList *buddies = purple_find_buddies(count->account, NULL);
		jobjectArray array = (jobjectArray) mainEnv->NewObjectArray(g_slist_length(buddies), mainEnv->FindClass("Buddy"), 0);
		int i = 0;
		while(buddies) {
			PurpleBuddy *b = (PurpleBuddy *) buddies->data;
			jobject jBuddy = buddy_new(b);
			mainEnv->SetObjectArrayElement(array, i++, jBuddy);
			buddies = g_slist_delete_link(buddies, buddies);
		}
		callJavaMethod((jobject) count->account->ui_data, "onContactsReceived", "([LBuddy;)V", array);
		delete count;
		return FALSE;
	}
	count->last_count = current_count;
	return TRUE;
}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node) || !mainEnv)
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *account = purple_buddy_get_account(buddy);
	callJavaMethod((jobject) account->ui_data, "onBuddyCreated", "(LBuddy;)V", buddy_new(buddy));

	int current_count = purple_account_get_int(account, "buddies_count", 0);
	if (current_count == 0) {
		BuddiesCount *count = new BuddiesCount;
		count->account = account;
		count->last_count = -1;
		int t = purple_timeout_add_seconds(1, &check_buddies_count, count); // TODO: remove me on disconnect
		purple_account_set_int(account, "buddies_timer", t);
	}
	purple_account_set_int(account, "buddies_count", current_count + 1);
	
}

static void buddyRemoved(PurpleBuddy *buddy, gpointer null) {
	if (buddy->node.ui_data)
		mainEnv->DeleteGlobalRef((jobject) buddy->node.ui_data);
}

static void signed_on(PurpleConnection *gc,gpointer unused) {
	PurpleAccount *account = purple_connection_get_account(gc);
	callJavaMethod((jobject) account->ui_data, "onConnected", "()V");
	int current_count = purple_account_get_int(account, "buddies_count", 0);
	if (current_count == 0) {
		BuddiesCount *count = new BuddiesCount;
		count->account = account;
		count->last_count = -1;
		int t = purple_timeout_add_seconds(1, &check_buddies_count, count); // TODO: remove me on disconnect
		purple_account_set_int(account, "buddies_timer", t);
	}
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

static void connection_report_disconnect(PurpleConnection *gc, PurpleConnectionError reason, const char *text) {
	PurpleAccount *account = purple_connection_get_account(gc);
	jstring error = mainEnv->NewStringUTF(text ? text : "");
	callJavaMethod((jobject) account->ui_data, "onConnectionError", "(ILjava/lang/String;)V", (int) reason, error);
	mainEnv->DeleteGlobalRef((jobject) account->ui_data);
	purple_timeout_remove(purple_account_get_int(account, "buddies_timer", 0));
}

static PurpleConnectionUiOps conn_ui_ops =
{
	NULL,
	NULL,
	NULL,//connection_disconnected,
	NULL,
	NULL,
	NULL,
	NULL,
	connection_report_disconnect,
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
	purple_connections_set_ui_ops(&conn_ui_ops);
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

static jobject session_new(PurpleAccount *account, int type) {
	jfieldID fid;
	jclass cls = mainEnv->FindClass("Session");
	jmethodID mid = mainEnv->GetMethodID (cls, "<init>", "()V");
	jobject object = mainEnv->NewObject(cls, mid);

	fid = mainEnv->GetFieldID(cls, "type", "I");
	mainEnv->SetIntField(object, fid, type); 

	jstring name = mainEnv->NewStringUTF(purple_account_get_username(account));
	fid = mainEnv->GetFieldID(cls, "name", "Ljava/lang/String;");
	mainEnv->SetObjectField(object, fid, name);

	fid = mainEnv->GetFieldID(cls, "handle", "J");
	mainEnv->SetLongField(object, fid, (jlong) account); 

	fid = mainEnv->GetFieldID(cls, "sporky", "LSporky;");
	mainEnv->SetObjectField(object, fid, (jobject) mainObj); 

	account->ui_data = mainEnv->NewGlobalRef(object);

	return object;
}

static PurpleAccount *session_get_account(jobject object) {
	jclass cls = mainEnv->GetObjectClass(object);
	jfieldID fid  = mainEnv->GetFieldID(cls, "handle", "J");
	jlong handle = mainEnv->GetLongField(object, fid); 
	return (PurpleAccount *) handle;
}


JNIEXPORT jint JNICALL Java_Sporky_init(JNIEnv *env, jobject obj, jstring _dir) {
	dlopen("libpurple.so", RTLD_NOW|RTLD_GLOBAL);

	const char *dir = env->GetStringUTFChars(_dir, 0);
	purple_util_set_user_dir(dir);
	env->ReleaseStringUTFChars(_dir, dir);

	purple_debug_set_ui_ops(&debugUiOps);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	int ret = purple_core_init("sporky");
	if (ret) {
		static int conversation_handle;
		static int conn_handle;
		static int blist_handle;

		purple_set_blist(purple_blist_new());
// 		purple_blist_load();

		purple_prefs_load();
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
	}
	mainObj = env->NewGlobalRef(obj);
	mainEnv = env;
	return ret;
}

JNIEXPORT jobject JNICALL Java_Sporky_connect (JNIEnv *env, jobject sporky, jstring _name, jint type, jstring _password) {
	const char *name = env->GetStringUTFChars(_name, 0);
	const char *password = env->GetStringUTFChars(_password, 0);

	static char prpl[30];
	switch(type) {
		case TYPE_JABBER:
			strcpy(prpl, "prpl-jabber");
			break;
		case TYPE_ICQ:
			strcpy(prpl, "prpl-icq");
			break;
		case TYPE_MSN:
			strcpy(prpl, "prpl-msn");
			break;
		case TYPE_AIM:
			strcpy(prpl, "prpl-aim");
			break;
	}

	PurpleAccount *account = purple_accounts_find(name, prpl);
	if (account) {
		purple_account_set_int(account, "buddies_count", 0);
		GList *iter;
		for (iter = purple_get_conversations(); iter; ) {
			PurpleConversation *conv = (PurpleConversation*) iter->data;
			iter = iter->next;
			if (purple_conversation_get_account(conv) == account)
				purple_conversation_destroy(conv);
		}
// 		purple_accounts_delete(account);
	}
	account = purple_account_new(name, prpl);
	purple_account_set_password(account, password);
	purple_account_set_enabled(account, "sporky", TRUE);
	purple_accounts_add(account);

	env->ReleaseStringUTFChars(_name, name);
	env->ReleaseStringUTFChars(_password, password);
	
	return session_new(account, type);
}

JNIEXPORT void JNICALL Java_Session_disconnect (JNIEnv *env, jobject ses) {
	PurpleAccount *account = session_get_account(ses);
	mainEnv->DeleteGlobalRef((jobject) account->ui_data);
	purple_account_set_enabled(account, "sporky", FALSE);
}

JNIEXPORT jint JNICALL Java_Sporky_start (JNIEnv *env, jobject obj) {
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

JNIEXPORT void JNICALL Java_Session_sendMessage (JNIEnv *env, jobject ses, jstring _to, jstring _message) {
	const char *to = env->GetStringUTFChars(_to, 0);
	const char *message = env->GetStringUTFChars(_message, 0);
	PurpleAccount *account = session_get_account(ses);
	
	if (account) {
		PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, to, account);
		if (!conv)
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, to);

		gchar *_markup = purple_markup_escape_text(message, -1);
		purple_conv_im_send(PURPLE_CONV_IM(conv), _markup);
	}
	
	env->ReleaseStringUTFChars(_to, to);
	env->ReleaseStringUTFChars(_message, message);
}

struct timerCallback {
	jobject obj;
	char *cb;
};

static gboolean _timer_callback(void *data) {
	timerCallback *d  = (timerCallback *) data;
	int ret = callJavaMethod(d->obj, d->cb, "()I");
	if (ret == 0) {
		mainEnv->DeleteGlobalRef(d->obj);
		g_free(d->cb);
		delete d;
	}
	return ret;
}

JNIEXPORT jint JNICALL Java_Sporky_addTimer (JNIEnv *env, jobject, jobject obj, jstring callback, jint ms) {
	int handle;
	const char *cb = env->GetStringUTFChars(callback, 0);
	timerCallback *d = new timerCallback;
	d->obj = env->NewGlobalRef(obj);
	d->cb = g_strdup(cb);
	if (ms >= 1000)
		handle = purple_timeout_add_seconds(ms / 1000, _timer_callback, d);
	else
		handle = purple_timeout_add(ms, _timer_callback, d);
	env->ReleaseStringUTFChars(callback, cb);
	return handle;
}

JNIEXPORT void JNICALL Java_Sporky_removeTimer (JNIEnv *, jobject, jint handle) {
	purple_timeout_remove(handle);
}
  