package transmitter;

public class CommandDatagram {
	
	private static final int SIZE = 10;
	
	public long time;
	public byte type = 2;
	public byte cmd = 2;
	
	public int Size() {
		return SIZE;
	}
	
}
