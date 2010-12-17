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
		stop();
		
	}

	protected void onBuddyCreated(Session ses, Buddy buddy) {
		System.out.println("onBuddyCreated");
	}

	protected void onContactsReceived(String[] contacts) {
		System.out.println("onContactsReceived");
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