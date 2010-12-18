class Buddy {
	protected String name;
	protected String alias;
	protected long handle;
}

class Session {
	protected int type;
	protected String name;
	protected long handle;
}

class Sporky {
	protected native int init(String purpleDir);
	protected native int start();
	protected native void stop();
	protected native int addTimer(String callback, int ms);
	protected native void removeTimer(int handle);
	protected native Session connect(String accountName, int transport, String password);
	protected native void disconnect(Session ses);
	protected native void sendMessage(Session ses, String to, String message);

	protected int onConnectTimeout() {
		s.connect(name, 0, password);
		return 0;
	}

	protected void onConnected(Session ses) {
		System.out.println("onConnected");
// 		sendMessage(ses, "hanzz@njs.netlab.cz" ,"ahoj :) -- Sent by Sporky, don't answer here...");
	}

	protected void onConnectionError(Session ses, int error, String message) {
		System.out.println("onConnectionError");
		System.out.println(message);
		stop();
	}

	protected void onBuddyCreated(Session ses, Buddy buddy) {
		System.out.println("onBuddyCreated");
		System.out.println(buddy.alias);
		System.out.println(buddy.name);
	}

	protected void onContactsReceived(Session ses, Buddy[] buddies) {
		System.out.println("onContactsReceived");
		System.out.println(buddies[0].name);
		disconnect(ses);
		addTimer("onConnectTimeout", 1000);
	}

	public static void main(String[] args) {
		name = args[0];
		password = args[1];
		s = new Sporky();
		s.init("/tmp");
		Session ses = s.connect(name, 0, password);
		s.start();
	}
	public static Sporky s;
	public static String name;
	public static String password;
	static {
		System.loadLibrary("sporky");
	}
}