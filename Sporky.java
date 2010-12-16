public class Sporky {
	protected static native String[] getContacts(String accountName,
			Integer transport, String password);

	protected static native void deliveryChatMessage(String accountName,
			Integer transport, String password, String[] targets);

	public static void main(String[] args) {
	    System.loadLibrary("sporky");
	//     int r = nativeMethod(123);
		getContacts("hanzzik", 0, "xxxxxxx");
		System.out.println("The native method has returned ");
	}
}