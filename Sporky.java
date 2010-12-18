class Buddy {
	protected String name;
	protected String alias;
	protected long handle;
}

class Session {
	protected native void disconnect();
	protected native void sendMessage(String to, String message);

	protected int TimeoutCallbackTest() {
		disconnect();
		sporky.stop();
		return 0;
	}

	protected void onConnected() {
		System.out.println("onConnected");
		sendMessage("hanzz@njs.netlab.cz" ,"ahoj :) -- Sent by Sporky, don't answer here...");
	}

	protected void onConnectionError(int error, String message) {
		System.out.println("onConnectionError");
		System.out.println(message);
		sporky.stop();
	}

	protected void onBuddyCreated(Buddy buddy) {
		System.out.println("onBuddyCreated");
		System.out.println(buddy.alias);
		System.out.println(buddy.name);
	}

	protected void onContactsReceived(Buddy[] buddies) {
		System.out.println("onContactsReceived");
		System.out.println(buddies[0].name);
		sporky.addTimer(this, "TimeoutCallbackTest", 4000);
	}

	protected int type;
	protected String name;
	protected long handle;
	protected Sporky sporky;
}

class Sporky {
	protected native int init(String purpleDir);
	protected native int start();
	protected native void stop();
	protected native int addTimer(Object obj, String callback, int ms);
	protected native void removeTimer(int handle);
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