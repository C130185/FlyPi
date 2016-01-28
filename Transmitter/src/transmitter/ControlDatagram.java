package transmitter;

public class ControlDatagram {

	private static final int SIZE = 13;

	public long time;
	public byte type = 1;
	public byte throttle;
	public byte roll;
	public byte pitch;
	public byte yaw;

	public int Size() {
		return SIZE;
	}

}
