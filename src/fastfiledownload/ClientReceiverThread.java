package fastfiledownload;
import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;


/**
 * 
 * @author rkandur
 * 
 *         Client Helper Class to achieve FastFileDownload by splitting a single
 *         file download to multiple clients who host the same file. Once all
 *         the parts are executed successfully, an aggregation of the parts of
 *         files is done to get the original file.
 *
 */

public class ClientReceiverThread extends Thread {

	Socket m_clientSocket;
	String m_fileName;
	
	int m_currentPart;
	int m_totalParts;
	
	public ClientReceiverThread(Socket clientSocket, String filename, int totalParts, int filePart) {
		
		m_clientSocket = clientSocket;
		m_fileName = filename;
		
		m_currentPart = filePart;
		m_totalParts = totalParts;
		
	}

	// this is the deamon thread which creates a TCP connection to multiple
	// other clients for fast file download
	// then reports back to the main client thread with the status
	public void run() {

		super.run();
		
		try {
			
			System.out.println("Downloading " + m_fileName + ", " + m_currentPart + ", " + m_totalParts);
			
			InputStream inpStream = m_clientSocket.getInputStream();
			FileOutputStream fileOutStream = new FileOutputStream(new StringBuffer().append("./shared_dir/part").append(m_currentPart).toString());
		    BufferedOutputStream buffOutStream = new BufferedOutputStream(fileOutStream);
		    
		    int bufferSize = m_clientSocket.getReceiveBufferSize();
		    
			DataOutputStream outStream = new DataOutputStream(m_clientSocket.getOutputStream());
			outStream.writeUTF(new StringBuffer().append("fastfiledownload ")
					.append(m_fileName).append(" ").append(m_currentPart)
					.append(" ").append(m_totalParts).toString());						

			byte[] bytes = new byte[bufferSize];
			int readSize;
		    while ((readSize = inpStream.read(bytes)) > 0) {
		        buffOutStream.write(bytes, 0, readSize);
		    }
		    buffOutStream.flush();
		    
		    buffOutStream.close();
		    inpStream.close();
		    outStream.close();
		    m_clientSocket.close();
		    
			System.out.println("Finished downloading " + m_fileName +", " + m_currentPart + ", " + m_totalParts);
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
}
