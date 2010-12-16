class Sporky {
	protected native int init(String purpleDir);

	protected native void connect(String accountName,
			Integer transport, String password);

	protected native int start();
	protected native void stop();
	protected native void sendMessage(String accountName,
			String to, String message);

	protected void onConnected(String accountName) {
		System.out.println("onConnected");
		System.out.println(accountName);
		sendMessage("hanzz.k@gmail.com", "seb.marion@gmail.com" ,"ahoj :) -- Sent by Sporky, don't answer here...");
		stop();
	}

	protected void onBuddyCreated(String buddy) {
		System.out.println("onBuddyCreated");
		System.out.println(buddy);
	}

	protected void onContactsReceived(String[] contacts) {
		System.out.println("onContactsReceived");
		stop();
	}

	public static void main(String[] args) {
		Sporky s = new Sporky();

		s.init("/tmp");
		s.connect(args[0], 0, args[1]);
		s.start();
	}
	static {
		System.loadLibrary("sporky");
	}
}