package fastfiledownload;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * 
 * @author rkandur {@link ClientDownloaderThread} is a Deamon Thread which runs
 *         along with the main client thread to serve the Other clients Download
 *         Requests.
 *
 */
public class ClientDownloaderThread extends Thread {

	// socket to identify the client who has requested for a fastfiledownload
	ServerSocket m_fastFileSocket;

	public ClientDownloaderThread(ServerSocket ffs) {
		m_fastFileSocket = ffs;
	}

	// run method which keeps serving other clients indefinitely
	public void run() {
		
		super.run();

		while(true) {
			
			Socket clientSocket = null;
			try {
				
				clientSocket = m_fastFileSocket.accept();
				DataOutputStream outStream = new DataOutputStream(clientSocket.getOutputStream());
				DataInputStream inStream = new DataInputStream(clientSocket.getInputStream());
				handleClientRequest(inStream, outStream, clientSocket);
				
			} catch (IOException e) {
				System.err.println("Could not accept Client Connection !!!");
				System.exit(1);
			}
		}

	}

	private void handleClientRequest(DataInputStream ins, DataOutputStream outs, Socket clientSocket) {

		String request = new String();
		try {
			request = ins.readUTF();
		} catch (IOException e) {
			e.printStackTrace();
		}

		String[] reqToks = request.split(" "); //fastfiledownload file part totalparts 
		if(reqToks.length != 4) {
			try {
				outs.writeUTF("405 Invalid Arguments");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		try {
			
			File file = new File("./shared_dir/" + reqToks[1]);
			long fileLength = file.length();
			
			byte[] bytes = new byte[(int) fileLength];
			
			FileInputStream fileInpStream = new FileInputStream(file);
			BufferedInputStream buffInpStream = new BufferedInputStream(fileInpStream);
			BufferedOutputStream buffOutStream = new BufferedOutputStream(clientSocket.getOutputStream());
			
			if(Integer.parseInt(reqToks[2]) == -1 || Integer.parseInt(reqToks[3]) == 1) {

				if (fileLength > Integer.MAX_VALUE) {
					System.out.println("File is too large.");
				}
				
				int readSize;
				while ((readSize = buffInpStream.read(bytes)) > 0) {
					buffOutStream.write(bytes, 0, readSize);					
				}
				
				buffInpStream.close();
				buffOutStream.close();
				fileInpStream.close();
				
				ins.close();
				outs.close();
				clientSocket.close();				

				return;
				
			} else {
				
				int totalSize = buffInpStream.read(bytes);
				int numberOfClients = Integer.parseInt(reqToks[3]);
				int filePart = Integer.parseInt(reqToks[2]);
				
				int bytesToDownload = totalSize / numberOfClients;
				int startIndex = (filePart-1) * bytesToDownload;

				int endIndex;
				if(filePart == numberOfClients){
					endIndex = totalSize-startIndex;
				}else 
					endIndex = bytesToDownload;
				if (totalSize  > 0) {
					buffOutStream.write(bytes, startIndex, endIndex);					
				}
				
				buffInpStream.close();
				buffOutStream.close();
				fileInpStream.close();
				
				outs.flush();
				ins.close();
				outs.close();
				
				clientSocket.close();

			}
			
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}

}
