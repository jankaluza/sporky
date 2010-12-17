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

	protected native Session connect(String accountName,
			int transport, String password);

	protected native int start();
	protected native void stop();
	protected native void sendMessage(Session ses,
			String to, String message);

	protected void onConnected(Session ses) {
		System.out.println("onConnected");
		sendMessage(ses, "hanzz@njs.netlab.cz" ,"ahoj :) -- Sent by Sporky, don't answer here...");
	}

	protected void onBuddyCreated(Session ses, Buddy buddy) {
		System.out.println("onBuddyCreated");
		System.out.println(buddy.alias);
		System.out.println(buddy.name);
	}

	protected void onContactsReceived(Session ses, Buddy[] buddies) {
		System.out.println("onContactsReceived");
		System.out.println(buddies[0].name);
		stop();
	}

	public static void main(String[] args) {
		Sporky s = new Sporky();

		s.init("/tmp");
		Session ses = s.connect(args[0], 0, args[1]);
		s.start();
	}
	static {
		System.loadLibrary("sporky");
	}
}