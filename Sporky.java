class Buddy {
	protected String name;
	protected String alias;
	protected long handle;
}

class Session {
	// Disconnects this session from legacy network.
	protected native void disconnect();

	// Sends message to buddy in legacy network
	// to - username of recipient
	// message - message
	protected native void sendMessage(String to, String message);

	// Called when this session connects the legacy network
	protected void onConnected() {
		System.out.println("onConnected");
		sendMessage("hanzz@njs.netlab.cz" ,"ahoj :) -- Sent by Sporky, don't answer here...");
	}

	// Called when this session coulnd't connect legacy network.
	// Session is disconnected after this signal.
	// error - http://developer.pidgin.im/doxygen/dev/html/connection_8h.html#d073b7b1d65488a3b3e39fc382324c4d
	// message - error message
	protected void onConnectionError(int error, String message) {
		System.out.println("onConnectionError");
		System.out.println(message);
		sporky.stop();
	}

	// Called when there's new buddy created and pushed into buddy list.
	// NOTE: This is called for each buddy just once, so if you reconnect the session,
	// it won't be called again.
	protected void onBuddyCreated(Buddy buddy) {
		System.out.println("onBuddyCreated");
		System.out.println(buddy.alias);
		System.out.println(buddy.name);
	}

	// Called when all contacts are received from legacy network. This is called
	// on every login.
	protected void onContactsReceived(Buddy[] buddies) {
		System.out.println("onContactsReceived");
		System.out.println(buddies[0].name);
		sporky.addTimer(this, "TimeoutCallbackTest", 10000);
	}

	// Called when new message is received.
	// from - username of sender
	// message - message
	// flags - mainly future use - http://developer.pidgin.im/doxygen/dev/html/conversation_8h.html#66e44dfdc0de2953d73f03fb806bf6f5
	// timestamp - time_t - for delayed message it's time when the message has been sent
	protected void onMessageReceived(String from, String message, int flags, long timestamp) {
		System.out.println("onMessageReceived");
		System.out.println(message);
	}

	protected int TimeoutCallbackTest() {
		disconnect();
		sporky.stop();
		return 0;
	}

	protected int type;
	protected String name;
	protected long handle;
	protected Sporky sporky;
}

class Sporky {
	// Initializes Sporky.
	// purpleDir - directory where libpurple stored cached avatars/buddies/certificates
	// return value - 1 if initialization was successfull, otherwise 0
	protected native int init(String purpleDir);

	// Starts main event loop. This call blocks. You have to stop Sporky in some callback
	// using stop(); method.
	protected native void start();

	// Stops main event loop.
	protected native void stop();

	// Adds new timer.
	// obj - object whichs function will be called as timeout callback
	// callback - name of callback function
	// ms - timeout in miliseconds.
	// return value - handle which can be passed into removeTimer to stop timer
	// NOTE: Callback function has to be defined like:
	// protected int TimeoutCallbackTest();
	// If it returns 0, timer will be automatically removed.
	// If it returns 1, callback will be called again after timeout.
	protected native int addTimer(Object obj, String callback, int ms);

	// Removes timer.
	// handle - handle from addTimer method.
	protected native void removeTimer(int handle);

	// Adds new socket notifier.
	// obj - object whichs function will be called when new data arrives on this socket
	// callback - name of callback function
	// source - Socket/SocketImpl/FileDescriptor
	// return value - handle which can be passed into removeSocketNotifier to stop socket notifier
	// NOTE: Callback function has to be defined like:
	// protected int NotifierCallbackTest(int source);
	// If it returns 0, notifier will be automatically removed.
	// If it returns 1, callback will be called again on new data.
	protected native int addSocketNotifier(Object obj, String callback, Object source);

	// Removes socket notifier.
	// handle - handle from addSocketNotifier method.
	protected native void removeSocketNotifier(int handle);

	// Connects to legacy network and creates Session.
	// accountName - username used for login
	// transport - enum {
	// 	TYPE_JABBER = 0,
	// 	TYPE_ICQ,
	// 	TYPE_MSN,
	// 	TYPE_AIM,
	// };
	// password - password
	// return value - Session associated with this account
	protected native Session connect(String accountName, int transport, String password);

	public static void main(String[] args) {
		String name = args[0];
		String password = args[1];
		Sporky s = new Sporky();
		s.init("/tmp");
		Session ses = s.connect(name, 0, password);
		s.start();
	}
	static {
		System.loadLibrary("sporky");
	}
}