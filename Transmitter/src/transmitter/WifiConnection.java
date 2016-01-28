package transmitter;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class WifiConnection {

	private static final int BUFFER_SIZE = 256;

	private DatagramSocket sock;
	private DatagramPacket packet;
	private ByteBuffer buf;

	public WifiConnection(String host, int port) {
		buf = ByteBuffer.allocate(256);
		buf.order(ByteOrder.LITTLE_ENDIAN);
		packet = new DatagramPacket(buf.array(), BUFFER_SIZE);
		packet.setSocketAddress(new InetSocketAddress(host, port));
	}

	public void start() throws SocketException {
		sock = new DatagramSocket();
	}

	public void stop() throws IOException {
		sock.close();
	}

	public void send(ControlDatagram d) throws IOException {
		buf.clear();
		buf.putLong(d.time);
		buf.put(d.type);
		buf.put(d.throttle);
		buf.put(d.roll);
		buf.put(d.pitch);
		buf.put(d.yaw);
		packet.setData(buf.array(), 0, d.Size());
		sock.send(packet);
	}

	public void send(CommandDatagram d) throws IOException {
		buf.clear();
		buf.putLong(d.time);
		buf.put(d.type);
		buf.put(d.cmd);
		packet.setData(buf.array(), 0, d.Size());
		sock.send(packet);
	}

	@Override
	protected void finalize() throws Throwable {
		try {
			stop();
		} catch (Exception e) {
			e.printStackTrace();
		}
		super.finalize();
	}

}
