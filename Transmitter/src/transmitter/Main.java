package transmitter;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.EventQueue;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.KeyEventDispatcher;
import java.awt.KeyboardFocusManager;
import java.awt.event.KeyEvent;
import java.io.IOException;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.SwingWorker;
import javax.swing.UIManager;
import javax.swing.border.LineBorder;
import javax.swing.border.TitledBorder;

public class Main implements KeyEventDispatcher {

	private static final String HOST = "192.168.2.1";
	private static final int PORT = 43123;

	private JFrame frame;
	private WifiConnection conn;
	private ControlDatagram controlDatagram;
	private CommandDatagram commandDatagram;
	private JLabel lblRollStatus;
	private JLabel lblPitchStatus;
	private JLabel lblYawStatus;
	private JLabel lblThrottleStatus;
	private JTextArea txtrInstructions;
	private JPanel panelInstructions;

	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Throwable e) {
			e.printStackTrace();
		}
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					Main window = new Main();
					window.frame.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	/**
	 * Create the application.
	 */
	public Main() {
		conn = new WifiConnection(HOST, PORT);
		controlDatagram = new ControlDatagram();
		controlDatagram.time = System.currentTimeMillis();
		controlDatagram.roll = 0;
		controlDatagram.pitch = 0;
		controlDatagram.yaw = 0;
		commandDatagram = new CommandDatagram();
		commandDatagram.time = System.currentTimeMillis();
		commandDatagram.cmd = 0;
		initialize();
	}

	/**
	 * Initialize the contents of the frame.
	 */
	private void initialize() {
		frame = new JFrame();
		frame.setBounds(100, 100, 450, 300);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.getContentPane().setLayout(new BorderLayout(0, 0));
		
		panelInstructions = new JPanel();
		frame.getContentPane().add(panelInstructions, BorderLayout.CENTER);
		GridBagLayout gbl_panelInstructions = new GridBagLayout();
		gbl_panelInstructions.columnWidths = new int[]{434, 0};
		gbl_panelInstructions.rowHeights = new int[]{226, 0};
		gbl_panelInstructions.columnWeights = new double[]{0.0, Double.MIN_VALUE};
		gbl_panelInstructions.rowWeights = new double[]{0.0, Double.MIN_VALUE};
		panelInstructions.setLayout(gbl_panelInstructions);
		
		txtrInstructions = new JTextArea();
		GridBagConstraints gbc_txtrInstructions = new GridBagConstraints();
		gbc_txtrInstructions.gridx = 0;
		gbc_txtrInstructions.gridy = 0;
		panelInstructions.add(txtrInstructions, gbc_txtrInstructions);
		txtrInstructions.setFont(UIManager.getFont("Label.font"));
		txtrInstructions.setText("R/F: Throttle Up/Down\r\nW/S: Pitch Up/Down\r\nA/D: Roll Up/Down\r\nQ/E: Yaw Up/Down\r\nSPACE: Start/Stop");
		txtrInstructions.setEditable(false);
		txtrInstructions.setCursor(null);
		txtrInstructions.setOpaque(false);
		txtrInstructions.setFocusable(false);

		JPanel panelStatus = new JPanel();
		panelStatus.setBorder(new TitledBorder(new LineBorder(new Color(0, 0, 0)), "Status", TitledBorder.LEADING,
				TitledBorder.TOP, null, new Color(0, 0, 0)));
		frame.getContentPane().add(panelStatus, BorderLayout.SOUTH);
		panelStatus.setLayout(new GridLayout(0, 4, 0, 0));

		lblThrottleStatus = new JLabel("Throttle: 0");
		panelStatus.add(lblThrottleStatus);

		lblRollStatus = new JLabel("Roll: 0");
		panelStatus.add(lblRollStatus);

		lblPitchStatus = new JLabel("Pitch: 0");
		panelStatus.add(lblPitchStatus);

		lblYawStatus = new JLabel("Yaw: 0");
		panelStatus.add(lblYawStatus);

		KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(this);

		SwingWorker<Integer, Integer> worker = new SwingWorker<Integer, Integer>() {
			@Override
			protected Integer doInBackground() throws Exception {
				try {
					conn.start();
					while (!isCancelled()) {
						controlDatagram.time = System.currentTimeMillis();
						conn.send(controlDatagram);
						Thread.sleep(10);
					}
				} catch (IOException e) {
					e.printStackTrace();
					done();
					return -1;
				}

				done();
				return 0;
			}
		};

		worker.execute();
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent e) {
		if (e.getID() == KeyEvent.KEY_PRESSED) {
			switch (e.getKeyCode()) {
			case KeyEvent.VK_W:
				controlDatagram.pitch += 10;
				break;
			case KeyEvent.VK_S:
				controlDatagram.pitch -= 10;
				break;
			case KeyEvent.VK_A:
				controlDatagram.roll -= 10;
				break;
			case KeyEvent.VK_D:
				controlDatagram.roll += 10;
				break;
			case KeyEvent.VK_Q:
				controlDatagram.yaw += 10;
				break;
			case KeyEvent.VK_E:
				controlDatagram.yaw -= 10;
				break;
			case KeyEvent.VK_R:
				controlDatagram.throttle += 10;
				break;
			case KeyEvent.VK_F:
				controlDatagram.throttle -= 10;
				break;
			case KeyEvent.VK_SPACE:
				if (commandDatagram.cmd == 1) {
					commandDatagram.cmd = 2;
				} else {
					commandDatagram.cmd = 1;
				}
				try {
					commandDatagram.time = System.currentTimeMillis();
					conn.send(commandDatagram);
				} catch (IOException e1) {
					e1.printStackTrace();
				}
				break;
			}
			lblThrottleStatus.setText("Throttle: " + controlDatagram.throttle);
			lblRollStatus.setText("Roll: " + controlDatagram.roll);
			lblPitchStatus.setText("Pitch: " + controlDatagram.pitch);
			lblYawStatus.setText("Yaw: " + controlDatagram.yaw);
		}
		return false;
	}

	public JLabel getLblRollStatus() {
		return lblRollStatus;
	}

	public JLabel getLblPitchStatus() {
		return lblPitchStatus;
	}

	public JLabel getLblYawStatus() {
		return lblYawStatus;
	}

	public JLabel getLblThrottleStatus() {
		return lblThrottleStatus;
	}
}
