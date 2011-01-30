enum StatusType {
	UNSET,
	OFFLINE,
	AVAILABLE,
	UNAVAILABLE,
	INVISIBLE,
	AWAY,
	EXTENDED_AWAY
}

enum TransportType {
	JABBER,
	ICQ,
	MSN,
	AIM
}

enum ConnectionErrorType {
	CONNECTION_ERROR_NETWORK_ERROR,
	CONNECTION_ERROR_INVALID_USERNAME,
	CONNECTION_ERROR_AUTHENTICATION_FAILED,
	CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
	CONNECTION_ERROR_NO_SSL_SUPPORT,
	CONNECTION_ERROR_ENCRYPTION_ERROR,
	CONNECTION_ERROR_NAME_IN_USE,
	CONNECTION_ERROR_INVALID_SETTINGS,
	CONNECTION_ERROR_CERT_NOT_PROVIDED,
	CONNECTION_ERROR_CERT_UNTRUSTED,
	CONNECTION_ERROR_CERT_EXPIRED,
	CONNECTION_ERROR_CERT_NOT_ACTIVATED,
	CONNECTION_ERROR_CERT_HOSTNAME_MISMATCH,
	CONNECTION_ERROR_CERT_FINGERPRINT_MISMATCH,
	CONNECTION_ERROR_CERT_SELF_SIGNED,
	CONNECTION_ERROR_CERT_OTHER_ERROR,
	CONNECTION_ERROR_OTHER_ERROR
}

class Buddy {
	// Removes buddy from legacy network contact list.
	protected native void remove();

	// Called when buddy signs off.
	protected void onSignedOff() {
		System.out.println("onBuddySignedOff");
		System.out.println(alias);
		System.out.println(name);
	}

	// Called when buddy signs on.
	protected void onSignedOn() {
		System.out.println("onBuddySignedOn");
		System.out.println(alias);
		System.out.println(name);
		System.out.println(status);
	}

	// Called when buddy's status changes.
	protected void onStatusChanged() {
		System.out.println("onBuddyStatusChanged");
		System.out.println(alias);
		System.out.println(name);
		System.out.println(status + " " + statusMessage);
	}

	protected void onIconChanged() {
		System.out.println("onIconChanged " + icon);
	}

	protected String name;
	protected String alias;
	protected StatusType status;
	protected String statusMessage;
	protected byte[] icon;
	protected long handle;
	protected Session session;
}

class Session {
	// Disconnects this session from legacy network.
	protected native void disconnect();

	// Sends message to buddy in legacy network
	// to - username of recipient
	// message - message
	protected native void sendMessage(String to, String message);

	// Sets status and status message.
	// status - status
	// message - message
	protected native void setStatus(StatusType type, String message);

	// Adds buddy into legacy network contact list.
	protected native Buddy addBuddy(String name, String alias);

	protected native void setIcon(byte[] icon);

	// Called when this session connects the legacy network
	protected void onConnected() {
		System.out.println("onConnected");
		setStatus(StatusType.EXTENDED_AWAY, "testing message! :)");
		sendMessage("hanzz@njs.netlab.cz" ,"ahoj :) -- Sent by Sporky, don't answer here...");
	}

	// Called when this session coulnd't connect legacy network.
	// Session is disconnected after this signal.
	// error - http://developer.pidgin.im/doxygen/dev/html/connection_8h.html#d073b7b1d65488a3b3e39fc382324c4d
	// message - error message
	protected void onConnectionError(ConnectionErrorType error, String message) {
		System.out.println("onConnectionError " + error);
		System.out.println(message);
// 		sporky.stop();
		sporky.addTimer(this, "TimeoutCallbackTest", 10000);
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
		System.out.println(buddies[0].name + " " + buddies[0].status);
		sporky.addTimer(this, "TimeoutCallbackTest", 1000);
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

	protected native void setDebugEnabled(int enabled);

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
	// transport - type
	// password - password
	// return value - Session associated with this account
	protected native Session connect(String accountName, TransportType type, String password);

	public static void main(String[] args) {
		String name = args[0];
		String password = args[1];
		Sporky s = new Sporky();
		s.init("/tmp");
		s.setDebugEnabled(0);
		Session ses = s.connect(name, TransportType.JABBER, password);
		s.start();
		ses = s.connect(name, TransportType.JABBER, password);
		s.start();
	}
	static {
		System.loadLibrary("sporky");
	}
}