package fastfiledownload;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;


/**
 *
 * @author rkandur Deamon thread which keeps on sending hearbeats/registering
 *         with the Registration server to say that it is actively serving
 *         requests
 *
 */
public class DirectoryRegistrationThread extends Thread{
	
	Socket m_socket;
	
	public DirectoryRegistrationThread(Socket socket) {
		m_socket = socket;
	}

	@Override
	public void run() {
		
		try {
			
			if(m_socket.isClosed()){
				m_socket= new Socket(m_socket.getInetAddress(), m_socket.getPort());
			}
	
			DataOutputStream dataOutStream = new DataOutputStream(m_socket.getOutputStream());	
			DataInputStream dataInpStream = new DataInputStream(m_socket.getInputStream());
			
			String ipAddress = m_socket.getLocalAddress().toString();
			ipAddress = ipAddress.substring(ipAddress.indexOf('/')+1);
			
			String command = "addMe " + ipAddress +" 50000";
			dataOutStream.writeUTF(command);
			dataInpStream.readUTF();
			
			dataOutStream.close();
			dataInpStream.close();
			
			interrupt();
		
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
}
